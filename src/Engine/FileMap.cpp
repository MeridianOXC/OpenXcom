/*
 * Copyright 2010-2016 OpenXcom Developers.
 *
 * This file is part of OpenXcom.
 *
 * OpenXcom is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * OpenXcom is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with OpenXcom.  If not, see <http://www.gnu.org/licenses/>.
 */

/*
 * Rules re resource/ruleset file records
 *
 * 1. mods are either directories under standard / user/mods
 *    or zip files in those directories.
 * 2. metadata.yml is required.
 * 3. zipfiles can either contain a single mod (metadata.yml at top level) or
 *    multiple mods (metadata.yml one level down).
 * 4. in both cases zipfile [sub]directory structure is what a mod dir structure is.
 * 5. mod directory can not contain zip files.
 * 6. same goes for subdirectories.
 * 7. the loader considers actual files in a directory tree with the same modId
 *    to override the zipped files from a mod with the modId.
 * 8. no more than a single .zip or a single directory is allowed for a modId.
 * 9. zipfile name does not matter, but if a directory happens to get scanned insert_before
 *    a zipfile with the same modId, the zipfile would be ignored.
 * A. somename.zip is always scanned before somename/ directory.
 */

#include <string>
#include <sstream>
#include <istream>
#include <unordered_map>
#include <unordered_set>

#include "FileMap.h"
#include "Unicode.h"
#include "Logger.h"
#include "CrossPlatform.h"
#include "Options.h"
#include "Exception.h"

#define MINIZ_NO_STDIO
#include "../../libs/miniz/miniz.h"

extern "C"
{

int mzops_close(struct SDL_RWops *context) {
	if (context) {
		if (context->hidden.mem.base) {
			mz_free(context->hidden.mem.base);
		}
		SDL_FreeRW(context);
	}
	return 0;
}
SDL_RWops *SDL_RWFromMZ(mz_zip_archive *zip, mz_uint file_index) {
	size_t size;
	void *data = mz_zip_reader_extract_to_heap(zip, file_index, &size, 0);
	if (data == NULL) {
		SDL_SetError("miniz extract: %s", mz_zip_get_error_string(mz_zip_get_last_error(zip)));
		return NULL;
	}
	SDL_RWops *rv = SDL_RWFromConstMem(data, size);
	rv->close = mzops_close;
	return rv;
}

/* helpers that are already present in SDL2 */
#if !SDL_VERSION_ATLEAST(2,0,0)
Uint8 SDL_ReadU8(SDL_RWops *src) {
	Uint8 px = 0;
	SDL_RWread(src, &px, 1, 1);
	return px;
}
Sint64 SDL_RWsize(SDL_RWops *src) {
	auto curpos = SDL_RWtell(src);
	auto rv = SDL_RWseek(src, 0, RW_SEEK_END);
	SDL_RWseek(src, curpos, RW_SEEK_SET);
	return rv;
}
#endif
#if !SDL_VERSION_ATLEAST(2,0,6)
void *SDL_LoadFile_RW(SDL_RWops *src, size_t *datasize, int freesrc)
{
	const int FILE_CHUNK_SIZE = 1024;
	Sint64 size;
	Sint64 size_read, size_total;
	void *data = NULL, *newdata;
	if (!src) {
		SDL_SetError("SDL_LoadFile_RW(): src==NULL");
		return NULL;
	}

	size = SDL_RWsize(src);
	if (size < 0) {
		size = FILE_CHUNK_SIZE;
	}
	data = SDL_malloc((size_t)(size + 1));

	size_total = 0;
	for (;;) {
		if ((((Sint64)size_total) + FILE_CHUNK_SIZE) > size) {
			size = (size_total + FILE_CHUNK_SIZE);
			newdata = SDL_realloc(data, (size_t)(size + 1));
			if (!newdata) {
				SDL_free(data);
				data = NULL;
				SDL_OutOfMemory();
				goto done;
			}
		data = newdata;
	}

	size_read = SDL_RWread(src, (char *)data+size_total, 1, (size_t)(size-size_total));
	if (size_read == 0) {
			break;
		}
			if (size_read == -1) {
				break;
			}
		size_total += size_read;
	}

	if (datasize) {
		*datasize = size_total;
	}
	((char *)data)[size_total] = '\0';

	done:
	if (freesrc && src) {
		SDL_RWclose(src);
	}
	return data;
}
#endif

/* miniz to SDL_rwops helpers */

static size_t mz_rwops_read_func(void *vops, mz_uint64 file_ofs, void *pBuf, size_t n) {
	SDL_RWops *rwops = (SDL_RWops *)vops;
	Sint64 size_seek = SDL_RWseek(rwops, file_ofs, SEEK_SET);
	if (size_seek != (Sint64)file_ofs) { return 0; }
	int size_read = SDL_RWread(rwops, pBuf, 1, n);
	return size_read;
}
static mz_bool mz_zip_reader_init_rwops(mz_zip_archive *pZip, SDL_RWops *rwops) {
	if (!rwops) { return false; }
	Sint64 size = SDL_RWsize(rwops);
	mz_zip_zero_struct(pZip);
	pZip->m_pRead = mz_rwops_read_func;
	pZip->m_pIO_opaque = rwops;
	return mz_zip_reader_init(pZip, size, 0);
}
static mz_bool mz_zip_reader_end_rwops(mz_zip_archive *pZip) {
	if (!pZip) { return false; }
	if (!pZip->m_pIO_opaque) { return false; }
	SDL_RWclose((SDL_RWops *)(pZip->m_pIO_opaque));
	return mz_zip_reader_end(pZip);
}

}

namespace OpenXcom
{
namespace FileMap
{

static inline std::string concatPaths(const std::string& basePath, const std::string& relativePath)
{
	if(basePath.size() == 0) throw Exception("Need correct basePath");
	if(relativePath.size() == 0) throw Exception("Need correct relativePath");
	if (basePath[basePath.size() - 1] == '/')
	{
		return basePath + relativePath;
	}
	else
	{
		return basePath + "/" + relativePath;
	}
}
static inline std::string concatOptionalPaths(const std::string& basePath, const std::string& relativePath)
{
	if (basePath.size() == 0)
	{
		return relativePath;
	}
	else if (relativePath.size() == 0)
	{
		return basePath;
	}
	else
	{
		return concatPaths(basePath, relativePath);
	}
}

FileRecord::FileRecord() : fullpath(""), zip(NULL), findex(0) { }

SDL_RWops *FileRecord::getRWops() const
{
	SDL_RWops *rv;
	if (zip != NULL) {
		rv = SDL_RWFromMZ((mz_zip_archive *)zip, findex);
	} else {
		rv = SDL_RWFromFile(fullpath.c_str(), "rb");
	}
	if (!rv) { Log(LOG_ERROR) << "FileRecord::getRWops(): err=" << SDL_GetError(); }
	return rv;
}

SDL_RWops *FileRecord::getRWopsReadAll() const
{
	SDL_RWops *rv;
	if (zip != NULL)
	{
		rv = SDL_RWFromMZ((mz_zip_archive *)zip, findex);
	}
	else
	{
		rv = SDL_RWFromFile(fullpath.c_str(), "rb");
		if (rv)
		{
			size_t size = 0;
			auto data = SDL_LoadFile_RW(rv, &size, SDL_TRUE);
			if (data)
			{
				rv = SDL_RWFromConstMem(data, size);

				//close callback
				rv->close = [](struct SDL_RWops *context)
				{
					if (context)
					{
						//HACK: technically speaking `hidden` is an implementation detail, but we need to use it to deallocate memory (similar to `mzops_close`)
						if (context->hidden.mem.base)
						{
							SDL_free(context->hidden.mem.base);
						}
						SDL_FreeRW(context);
					}
					return 0;
				};
			}
			else
			{
				rv = nullptr;
			}
		}
	}
	if (!rv) { Log(LOG_ERROR) << "FileRecord::getRWopsReadAll(): err=" << SDL_GetError(); }
	return rv;
}

std::unique_ptr<std::istream> FileRecord::getIStream() const
{
	if (zip != NULL) {
		size_t size;
		void *data = mz_zip_reader_extract_to_heap((mz_zip_archive *)zip, findex, &size, 0);
		if (data == NULL) {
			auto err = "FileRecord::getIStream(): failed to decompress " + fullpath + ": ";
			err += mz_zip_get_error_string(mz_zip_get_last_error((mz_zip_archive *)zip));
			Log(LOG_FATAL) << err;
			throw Exception(err);
		}
		return std::unique_ptr<std::istream>(new StreamData(RawData{data, size, mz_free}));
	} else {
		return CrossPlatform::readFile(fullpath);
	}
}

YAML::Node FileRecord::getYAML() const
{
	try
	{
		return YAML::Load(*getIStream());
	}
	catch(...)
	{
		Log(LOG_FATAL) << "Error loading file '" << fullpath << "'";
		throw;
	}
}

std::vector<YAML::Node> FileRecord::getAllYAML() const
{
	try
	{
		return YAML::LoadAll(*getIStream());
	}
	catch(...)
	{
		Log(LOG_FATAL) << "Error loading file '" << fullpath << "'";
		throw;
	}
}


/**
 * Find out somehow if it's well-formed UTF8.
 * Since we can't know the code page used at compression time, we just return false,
 * which means crash, but note the .zip filename.
 *
 * And also convert slashes to forward.
 */
static bool sanitizeZipEntryName(std::string& zefname) {
	if (!Unicode::isValidUTF8(zefname)) { return false; }
	Unicode::replace(zefname,"\\","/");
	return true;
}
static std::string hexDumpBogusData(const std::string& bogus) {
	std::vector<char> buf(bogus.size()*3 + 1, 0);
	char *p = buf.data();
	for (auto c : bogus) { p += sprintf(p, "%02hhx ", c); }
	return std::string(buf.data());
}
/* recursively list a directory */
typedef std::vector<std::pair<std::string, std::string>> dirlist_t; // <dirname, basename>
static bool ls_r(const std::string &basePath, const std::string &relPath, dirlist_t& dlist) {
	auto fullDir = concatOptionalPaths(basePath, relPath);
	auto files = CrossPlatform::getFolderContents(fullDir);
	//Log(LOG_VERBOSE) << "ls_r: listing "<<fullDir<<" count="<<files.size();
	for (auto i = files.begin(); i != files.end(); ++i) {
		if (std::get<1>(*i)) { // it's a subfolder
			auto fullpath = concatPaths(fullDir, std::get<0>(*i));
			if (CrossPlatform::folderExists(fullpath)) {
				auto nextRelPath = concatOptionalPaths(relPath, std::get<0>(*i));
				ls_r(basePath, nextRelPath, dlist);
				continue;
			}
		} else {
			dlist.push_back(std::make_pair(relPath, std::get<0>(*i)));
		}
	}
	return true;
}
static bool isRuleset(const std::string& fname) {
	if (fname.size() < 4) { return false; }
	auto last4 = fname.substr(fname.size() - 4);
	auto canext = canonicalize(last4);
	return last4 == ".rul";
}

typedef std::unordered_map<std::string, FileRecord> FileSet;
static const NameSet emptySet;
static mz_zip_archive *newZipContext(const std::string& log_ctx, SDL_RWops *rwops);

struct VFSLayer {
	std::string fullpath;				// the origin
	FileSet resources; 					// relpath -> frec.
	std::vector<FileRecord> rulesets;  	// keeps FileRecord copies ala hard links
	std::unordered_map<std::string, NameSet> vdirs;
	bool mapped;		 				// once mapped all this is immutable

	VFSLayer(const std::string& path) : fullpath(path), resources(), rulesets(), vdirs(),
										mapped(false) { }
	~VFSLayer() { }
	const FileRecord *at(const std::string& relpath) {
		auto crelpath = canonicalize(relpath);
		auto it = resources.find(crelpath);
		return  (it == resources.end()) ? NULL : &it->second;
	}
	const NameSet& ls(const std::string& relpath) {
		auto crelpath = canonicalize(relpath);
		while (crelpath.size() > 0 && crelpath[crelpath.size() - 1] == '/') {
			crelpath.resize(crelpath.size() - 1);
		}
		auto it = vdirs.find(crelpath);
		return (it == vdirs.end()) ? emptySet : it->second;
	}
	const std::vector<FileRecord>& get_rulesets() { return rulesets; }

	void insert(const std::string& relpath, FileRecord frec) {
		auto crelpath = canonicalize(relpath);

		if (isRuleset(crelpath)) {
			rulesets.push_back(frec);
			return;
		}
		// shouldn't happen, but overwrite nevertheless
		auto existing = resources.find(crelpath);
		if (existing != resources.end()) { resources.erase(existing); }
		// actually insert
		resources.insert(std::make_pair(crelpath, frec));

		// update corresponding vdir
		std::string basename = crelpath;
		std::string dirname = "";
		auto const pos = crelpath.find_last_of('/');
		if (pos != crelpath.npos) {
			basename = crelpath.substr(pos + 1);
			dirname = crelpath.substr(0, pos);
		}
		auto existing_vd = vdirs.find(dirname);
		if (existing_vd == vdirs.end()) {
			vdirs.insert(std::pair<std::string, NameSet>(dirname, NameSet()));
		}
		vdirs.at(dirname).insert(basename);
	}
	/** maps a zipped moddir from filesystem
	* @param zippath - path to the .zip
	* @param prefix - prefix in the .zip in case there are multiple mods in a single .zip. Has to have the trailing slash.
	* @param ignore_ruls - skip rulesets
	* @return - did we map anything (false, i.e if failed to unzip)
	*/
	bool mapZipFile(const std::string& zippath, const std::string& prefix, bool ignore_ruls = false) {
		std::string log_ctx = "mapZipFile(" + zippath + ",  '" + prefix + "',  '" + (ignore_ruls ? "true" : "false") + "'): ";
		SDL_RWops *rwops = SDL_RWFromFile(zippath.c_str(), "r");
		if (!rwops) {
			Log(LOG_WARNING) << log_ctx << "Ignoring zip '" << zippath << "': " << SDL_GetError();
			return false;
		}
		return mapZipFileRW(rwops, zippath, prefix, ignore_ruls);
	}
	/** maps a zipped moddir from an SDL_RWops
	* @param rwops - SDL_RWops with the zip data
	* @param zipppath - the file path to associate this with
	* @param prefix - prefix in the .zip in case there are multiple mods in a single .zip. Has to have the trailing slash.
	* @param ignore_ruls - skip rulesets
	* @return - did we map anything (false, i.e if failed to unzip)
	*/
	bool mapZipFileRW(SDL_RWops *rwops, const std::string& zippath, const std::string& prefix, bool ignore_ruls = false) {
		std::string log_ctx = "mapZipFileRW(rwops, '" + zippath + "', '" + prefix + "',  '" + (ignore_ruls ? "true" : "false") + "'): ";
		mz_zip_archive *zip = newZipContext(log_ctx, rwops);
		if (!zip) { return false; }
		return mapZip(zip, zippath, prefix, ignore_ruls);
	}

	/** maps a zipped moddir from filesystem
	* @param zippath - the file path to associate this with
	* @param prefix - prefix in the .zip in case there are multiple mods in a single .zip. Has to have the trailing slash.
	* @param ignore_ruls - skip rulesets
	* @return - did we map anything (false, i.e if failed to unzip)
	*/
	bool mapZip(mz_zip_archive *zip, const std::string& zippath, const std::string& prefix, bool ignore_ruls = false) {
		std::string log_ctx = "mapZip(zip, '" + zippath + "', '" + prefix + "',  '" + (ignore_ruls ? "true" : "false") + "'): ";
		if (mapped) {
			auto err=  log_ctx + "Fatal: already mapped.";
			Log(LOG_FATAL) << err;
			throw Exception(err);
		}
		auto prefixlen = prefix.size();
		if ((prefixlen) > 0 && (prefix[prefixlen-1] != '/')) {
			auto err = log_ctx + "Bogus prefix of '" + prefix;
			Log(LOG_FATAL) << err;
			throw Exception(err);
		}
		mapped = true;
		fullpath = zippath;
		mz_uint filecount = mz_zip_reader_get_num_files(zip);

		FileRecord frec;
		frec.zip = zip;

		mz_uint mapped_count = 0;
		for (mz_uint fi = 0; fi < filecount; ++fi) {
			mz_zip_archive_file_stat fistat;
			mz_zip_reader_file_stat(zip, fi, &fistat);

			std::string fname = fistat.m_filename;
			if (!sanitizeZipEntryName(fname)) {
				Log(LOG_WARNING) << "Bogus filename " << hexDumpBogusData(fname) << " in " << zippath << ", ignoring.";
				continue;
			}
			std::string relfname = fname;
			if (fistat.m_is_encrypted || !fistat.m_is_supported) { continue; }
			if (fistat.m_is_directory) { continue; }
			if (fname.size() <= prefixlen) { continue; }
			if (prefixlen > 0) { // well, cut out the prefix to get the real relfname.
				auto tprefix = fname.substr(0, prefixlen);
				if (tprefix != prefix) { continue; }
				relfname = fname.substr(prefixlen, fname.npos);
			}
			frec.findex = fi;
			frec.fullpath = concatPaths(fullpath, fname);

			if (isRuleset(relfname) && ignore_ruls) { continue; }
			insert(relfname, frec);
			mapped_count ++;
		}
		Log(LOG_VERBOSE) << log_ctx << "mapped_count=" << mapped_count;
		return mapped_count > 1;
	}
	bool mapPlainDir(const std::string& dirpath, bool ignore_ruls = false) {
		std::string log_ctx = "mapPlainDir(" + dirpath + ", " + (ignore_ruls ? "true" : "false") + "): ";
		if (mapped) {
			auto err = log_ctx + "fatal: already mapped.";
			Log(LOG_FATAL) << err;
			throw Exception(err);
		}
		dirlist_t dlist;
		if (!ls_r(dirpath, "", dlist)) {
			return false;
		}
		fullpath = dirpath;
		FileRecord frec;
		frec.zip = NULL;
		std::string relpath;
		int mapped_count = 0;
		for (auto i = dlist.cbegin(); i != dlist.cend(); ++i) {
			relpath = concatOptionalPaths(i->first, i->second);
			frec.fullpath = concatPaths(fullpath, relpath);
			if (isRuleset(i->second) && ignore_ruls) { continue; }
			insert(relpath, frec);
			mapped_count ++;
		}
		return mapped_count > 1;
	}
	void dump(std::ostream &out, const std::string &prefix, bool verbose) {
		out << prefix << "origin: " << fullpath << "; " << resources.size()
						 << " resources, " << rulesets.size() << " rulesets";
		if (verbose) {
			for (auto l = resources.begin(); l != resources.end(); ++l) {
				out << prefix << "    " << l->first << " -> " << l->second.fullpath;
			}
		}
	}
};
struct VFSLayerStack {
	std::vector<VFSLayer *> layers;
	FileSet resources;
	std::vector<FileRecord> rulesets;
	std::unordered_map<std::string, NameSet> vdirs;

	VFSLayerStack() : layers(), resources(), rulesets(), vdirs() { }

	void clear() {
		layers.clear(); 	// not deleting them, they're owned by VFSLayerSet
		resources.clear();
		rulesets.clear();
		vdirs.clear();
	}
	void _merge_vdirs(VFSLayer *src) {
		auto src_vdirs = src->vdirs;
		for (auto i = src_vdirs.begin(); i != src_vdirs.end(); ++i) {
			auto vdi = vdirs.find(i->first);
			if (vdi == vdirs.end()) {
				vdirs.insert(*i);
			} else {
				for (auto ni = i->second.begin(); ni != i->second.end(); ++ni) {
					vdi->second.insert(*ni);
				}
			}
		}
	}
	void _merge_resources(VFSLayer *src, bool reverse) {
		for (auto ri = src->resources.begin(); ri != src->resources.end(); ++ri) {
			auto crelpath = ri->first;
			auto frec = ri->second;
			auto existing = resources.find(crelpath);
			if ((!reverse) && (existing != resources.end())) { resources.erase(existing); }
			resources.insert(std::make_pair(crelpath, frec));
		}
	}
	void push_back(VFSLayer *layer) {
		layers.push_back(layer);
		auto ruls = layer->rulesets;
		for (auto i = ruls.begin(); i != ruls.end(); ++i) {
			rulesets.push_back(*i);
		}
		_merge_vdirs(layer);
		_merge_resources(layer, false);
	}
	void push_front(VFSLayer *layer) {
		layers.insert(layers.begin(), layer);
		auto ruls = layer->rulesets;
		for (auto i = ruls.rbegin(); i != ruls.rend(); ++i) {
			rulesets.insert(rulesets.begin(), *i);
		}
		_merge_vdirs(layer);
		_merge_resources(layer, true);
	}
	const FileRecord *at(const std::string& relpath) {
		auto crelpath = canonicalize(relpath);
		auto it = resources.find(crelpath);
		return (it == resources.end()) ? NULL : &it->second;
	}
	const NameSet& ls(const std::string& relpath) {
		auto crelpath = canonicalize(relpath);
		while (crelpath.size() > 0 && crelpath[crelpath.size() - 1] == '/') {
			crelpath.resize(crelpath.size() - 1);
		}
		auto it = vdirs.find(crelpath);
		return (it == vdirs.end()) ? emptySet : it->second;
	}
	const std::vector<const FileRecord *> get_slice(const std::string& relpath) {
		std::vector<const FileRecord *> rv;
		auto crelpath = canonicalize(relpath);
		for (auto si = layers.begin(); si != layers.end(); ++si) {
			rv.push_back((*si)->at(crelpath));
		}
		return rv;
	}
	const std::vector<FileRecord> &getRulesets() { return rulesets; }
	void dump(std::ostream& out, const std::string& prefix, bool verbose) {
		for (size_t i = 0; i < layers.size(); ++i) {
			std::ostringstream subprefix;
			subprefix << prefix << "    layer " << i << ": ";
			layers[i]->dump(out, subprefix.str(), verbose);
		}
	}
};
// each mod is also a (sub)set of layers
struct ModRecord {
	ModInfo modInfo;
	VFSLayerStack stack;

	ModRecord(const std::string& somepath) : modInfo(somepath), stack() { }
	void push_back(VFSLayer *layer) { stack.push_back(layer); }
	void push_front(VFSLayer *layer) { stack.push_front(layer); }
	const FileRecord *at(const std::string& relpath) { return stack.at(relpath); }
	const NameSet& ls(const std::string& relpath) { return stack.ls(relpath); }
	const std::vector<FileRecord> &getRulesets() { return stack.getRulesets(); }
	void dump(std::ostream& out, const std::string& prefix, bool verbose) {
		out << "  modId=" << modInfo.getId() << "; " << stack.layers.size() << " layers";
		stack.dump(out, prefix, verbose);
	}
};

static bool mapExtResources(ModRecord *, const std::string&, bool embeddedOnly);

// the final VFS view is a set of layers too.
struct VFS {
	std::vector<ModRecord *> mods;
	VFSLayerStack stack;
	RSOrder rsorder;

	VFS() : mods(), stack(), rsorder() { }
	const RSOrder &get_rulesets() { return rsorder; }
	const std::vector<const FileRecord *> get_slice(const std::string& relpath) { return stack.get_slice(relpath); }
	const FileRecord *at(const std::string& relpath) { return stack.at(relpath); }
	const NameSet& ls(const std::string& relpath) { return stack.ls(relpath); }
	const NameSet& lslayer(const std::string& relpath, size_t level) {
		if (level >= stack.layers.size()) { level = stack.layers.size() - 1; }
		return stack.layers[level]->ls(relpath);
	}
	void push_back(ModRecord *mod) {
		mods.push_back(mod);
		auto layers = mod->stack.layers;
		for (auto i = layers.begin(); i != layers.end(); ++i) {
			stack.push_back(*i);
		}
		auto rulesets = mod->getRulesets();
		auto modId = mod->modInfo.getId();
		// 	typedef std::vector<std::pair<std::string, std::vector<FileRecord *>>> RSOrder;
		rsorder.push_back(std::make_pair(modId, rulesets));
	}
	void map_common(bool embeddedOnly) {
		auto mrec = new ModRecord("common");
		if (!mapExtResources(mrec, "common", embeddedOnly)) {
			Log(LOG_ERROR) << "VFS::map_common(): failed to map 'common'";
			delete mrec;
			return;
		}
		for (auto layer: mrec->stack.layers) {
			stack.push_back(layer);
		}
		delete mrec;
	}
	void clear() {
		rsorder.clear();
		mods.clear();
		stack.clear();
	}
	void dump(std::ostream& out, const std::string& prefix, bool verbose) {
		stack.dump(out, prefix, verbose);
	}
};

static std::unordered_map<std::string, ModRecord *> ModsAvailable;
static std::unordered_set<VFSLayer *> MappedVFSLayers; // owned here so we can have some sense of their lifetime
												       // only the layers that get dropped on FileMap::clear()
static std::vector<mz_zip_archive *> ZipContexts;	   // zip decompression contexts shared between layers that came from
													   // the same .zip. this makes the whole thing very thread-unsafe
static VFS TheVFS;

const RSOrder &getRulesets() { return TheVFS.get_rulesets(); }

static mz_zip_archive *newZipContext(const std::string& log_ctx, SDL_RWops *rwops) {
	mz_zip_archive *zip = (mz_zip_archive *) SDL_malloc(sizeof(mz_zip_archive));
	if (!zip) {
		Log(LOG_FATAL) << log_ctx << ": " << SDL_GetError();
		throw Exception("Out of memory");
	}
	if (!mz_zip_reader_init_rwops(zip, rwops)) {
		// whoa, no opening the file
		Log(LOG_WARNING) << log_ctx << "Ignoring zip: " << mz_zip_get_error_string(mz_zip_get_last_error(zip));
		SDL_RWclose(rwops);
		SDL_free(zip);
		return NULL;
	}
	ZipContexts.push_back(zip);
	return zip;
}

void clear(bool clearOnly, bool embeddedOnly) {
	TheVFS.clear();
	for (auto i : ModsAvailable ) { delete i.second; }
	ModsAvailable.clear();
	for (auto i : MappedVFSLayers ) { delete i; }
	MappedVFSLayers.clear();
	for (auto i : ZipContexts) { mz_zip_reader_end_rwops(i); SDL_free(i); }
	ZipContexts.clear();
	if (!clearOnly)
	{
		Log(LOG_VERBOSE) << "FileMap::clear(): mapping 'common'";
		TheVFS.map_common(embeddedOnly);
		if (LOG_VERBOSE <= Logger::reportingLevel()) {
			TheVFS.dump(Logger().get(LOG_VERBOSE), "\nFileMap::clear():", Options::oxceListVFSContents);
		}
	}
}
/**
 * This maps mods and their dependencies.
 * Dependencies' existence is checked in checkModsDependencies()
 * @param active - mods that are to be loaded.
 * 		it is supposedly the list of enabled mods in the order they
 * 		appear in the options screen. and with only a single master.
 * 		which nonetheless can itself depend on another master and so on.
*/
void setup(const std::vector<const ModInfo* >& active, bool embeddedOnly)
{
	TheVFS.clear();
	TheVFS.map_common(embeddedOnly);
	std::string log_ctx = "FileMap::setup(): ";

	Log(LOG_VERBOSE) << log_ctx << "Active mods per options:";
	for (auto i = active.cbegin(); i != active.cend(); ++i) {
		Log(LOG_VERBOSE) << log_ctx << "    " << (*i)->getId();
	}
	// expand the mod list into map order
	// listing all the mod dependencies
	std::vector<std::string> map_order;
	for (auto i = active.begin(); i != active.end(); ++i) {
		std::string currentId = (*i)-> getId();
		std::string masterId;
		auto insert_before = map_order.end();
		while (true) {
			insert_before = map_order.insert(insert_before, currentId);
			masterId = ModsAvailable.find(currentId)->second->modInfo.getMaster();
			if (masterId.empty()) { break; }
			currentId = masterId;
		}
	}
	// map the mods skipping duplicates
	// this will preserve the order.
	std::unordered_set<std::string> mods_seen;
	for (auto i = map_order.begin(); i !=  map_order.end(); ++i) {
		if (mods_seen.find(*i) != mods_seen.end()) { continue; }
		mods_seen.insert(*i);
		TheVFS.push_back( ModsAvailable.at(*i));
	}
	Log(LOG_VERBOSE) << log_ctx << "Active VFS stack:";
	if (LOG_VERBOSE <= Logger::reportingLevel()) {
		TheVFS.dump(Logger().get(LOG_VERBOSE), "\n" + log_ctx, Options::oxceListVFSContents);
	}
}
[[gnu::unused]]
static void dump_mods_layers(std::ostream &out, const std::string& prefix, bool verbose) {
	out << prefix << ModsAvailable.size() << " mods mapped:";
	for (auto i = ModsAvailable.begin(); i != ModsAvailable.end(); ++i) {
		i->second->dump(out, prefix + "  ", verbose);
	}
	out << prefix << MappedVFSLayers.size() << " layers mapped:";
	for (auto i = MappedVFSLayers.begin(); i != MappedVFSLayers.end(); ++i) {
		(*i)->dump(out, prefix + "  ", verbose);
	}
}
/**
 * Attempts to map extResources and insert the resulting layers into a mod
 * @param mrec - ModRec of a mod
 * @param basename - extRes name to map (from userDir/dataDir)
 */
static bool mapExtResources(ModRecord *mrec, const std::string& basename, bool embeddedOnly) {
	auto modId = mrec->modInfo.getId();
	std::string log_ctx = "FileMap::mapExtResources(" + modId + ", " + basename + "): ";
	bool mapped_anything = false;
	std::string zipname = basename + ".zip";
	SDL_RWops *embedded_rwops = NULL;
	if (zipname == "common.zip" || zipname == "standard.zip") {
		embedded_rwops = CrossPlatform::getEmbeddedAsset(zipname);
	}
	// first try finding a directory (ass-backwards since we got to push this into front re layers.
	if (!embedded_rwops || ! embeddedOnly) {
		std::string fullname = Options::getUserFolder() + basename;
		if (!CrossPlatform::folderExists(fullname)) {
			fullname = CrossPlatform::searchDataFolder(basename);
		}
		if (CrossPlatform::folderExists(fullname)) {
			Log(LOG_VERBOSE) << log_ctx << "found dir ("<<fullname<<")";
			auto layer = new VFSLayer(fullname);
			if (layer->mapPlainDir(fullname, true)) {
				mrec->push_front(layer);
				MappedVFSLayers.insert(layer);
				mapped_anything = true;
			} else {
				delete layer;
			}
		} else {
			Log(LOG_VERBOSE) << log_ctx << "dir not found ("<<fullname<<")";
		}
	}
	// then try finding a zipfile
	if (!embedded_rwops || ! embeddedOnly) {
		std::string fullname = Options::getUserFolder() + zipname;
		if (!CrossPlatform::fileExists(fullname)) {
			fullname = CrossPlatform::searchDataFile(zipname);
		}
		if (CrossPlatform::fileExists(fullname)) {
			Log(LOG_VERBOSE) << log_ctx << "found zip ("<<fullname<<")";
			auto layer = new VFSLayer(fullname);
			if (layer->mapZipFile(fullname, basename + "/", true) || layer->mapZipFile(fullname, "", true)) {
				mrec->push_front(layer);
				MappedVFSLayers.insert(layer);
				mapped_anything = true;
			} else {
				delete layer;
			}
		} else {
			Log(LOG_VERBOSE) << log_ctx << "zip not found ("<<fullname<<")";
		}
	}
	// now try the embedded zip
	{
		if (embedded_rwops) {
			Log(LOG_VERBOSE) << log_ctx << "found embedded asset ("<<zipname<<")";
			std::string ezipname = "exe:" + zipname;
			auto layer = new VFSLayer(ezipname);
			if (layer->mapZipFileRW(embedded_rwops, ezipname, "", true)) {
				mrec->push_front(layer);
				MappedVFSLayers.insert(layer);
				mapped_anything = true;
			} else {
				delete layer;
			}
		} else {
			Log(LOG_VERBOSE) << log_ctx << "embedded asset not found ("<<zipname<<")";
		}
	}
	if (!mapped_anything) { // well, nothing found. say so.
		Log(LOG_INFO) << log_ctx << "external resources not found.";
	}
	return mapped_anything;
}
/** Map a mod in a given .zip with a given prefix
 * @param zipfname  - full path to the .zip_open
 * @param prefix    - prefix (subdir) in the .zip if any
 */
static void mapZippedMod(mz_zip_archive *zip, const std::string& zipfname, const std::string& prefix) {
	std::string log_ctx = "mapZippedMod(" + zipfname + ", '" + prefix + "'): ";
	auto layer = new VFSLayer(concatPaths(zipfname, prefix));
	if (!layer->mapZip(zip, zipfname, prefix)) {
		Log(LOG_WARNING) << log_ctx << "Failed to map, skipping.";
		delete layer;
		return;
	}
	auto frec = layer->at("metadata.yml");
	if (frec == NULL) { // whoa, no metadata
		Log(LOG_WARNING) << log_ctx << "No metadata.yml found, skipping.";
		delete layer;
		return;
	}
	auto modpath = concatOptionalPaths(zipfname, prefix);
	auto doc = frec->getYAML();
	if (!doc.IsMap()) {
		Log(LOG_WARNING) << log_ctx << "Bad metadata.yml found, skipping.";
		delete layer;
		return;
	}
	auto mrec = new ModRecord(modpath);
	mrec->modInfo.load(doc);
	auto mri = ModsAvailable.find(mrec->modInfo.getId());
	if (mri != ModsAvailable.end()) {
		Log(LOG_ERROR) << log_ctx << "modId " << mrec->modInfo.getId() << " already mapped in, skipping " << modpath;
		delete mrec;
		delete layer;
		return;
	}
	Log(LOG_VERBOSE) << log_ctx << "mapped mod '" << mrec->modInfo.getId() << "' from " << modpath
					 << " master=" << mrec->modInfo.getMaster() << " version=" << mrec->modInfo.getVersion();
	MappedVFSLayers.insert(layer);
	mrec->push_back(layer);
	ModsAvailable.insert(std::make_pair(mrec->modInfo.getId(), mrec));
}
/** now this scans a zip of mods or of a single mod
 * @param rwops - SDL_RWops to the zip data
 * @param fullpath - full path to associate with the .zip.
 */
void scanModZipRW(SDL_RWops *rwops, const std::string& fullpath) {
	std::string log_ctx = "scanModZipRW(rwops, " + fullpath + "): ";
	mz_zip_archive *mzip = newZipContext(log_ctx, rwops);

	if (!mzip) { return; }
	// check if this is maybe a zip of a single mod (metadata.yml at the top level)
	if (mz_zip_reader_locate_file_v2(mzip, "metadata.yml", NULL, 0, NULL)) {
		Log(LOG_VERBOSE) << log_ctx << "retrying as a single-mod .zip";
		// FIXME: this doesn't seem to work at all... do we support this?
		mapZippedMod(mzip, fullpath, "");
		return;
	}
	mz_uint filecount = mz_zip_reader_get_num_files(mzip);
	for (mz_uint fi = 0; fi < filecount; ++fi) {
		mz_zip_archive_file_stat fistat;
		mz_zip_reader_file_stat(mzip, fi, &fistat);
		if (fistat.m_is_encrypted || !fistat.m_is_supported) { continue; }
		if (!fistat.m_is_directory) { continue; } // skip files, we're only interested in toplevel dirs.
		std::string prefix = fistat.m_filename;
		if (!sanitizeZipEntryName(prefix)) {
			Log(LOG_WARNING) << "Bogus dirname " << hexDumpBogusData(prefix) << " in " << fullpath << ", ignoring.";
			continue;
		}
		auto slashpos = prefix.find_first_of("/"); // miniz returns dirnames with trailing slashes
		if (slashpos != prefix.size() - 1) { continue; } // not top-level: skip.
		// FIXME: if Microsoft Windows "Send to > Compressed (zipped) folder" is used to create the ZIP archive,
		// this will never be called, because the top-level directory is NOT on the file list... yes, seriously, I'm not kidding!
		// Do we want to handle this somehow or do we just call it unsupported?
		mapZippedMod(mzip, fullpath, prefix);
	}
}
/** Filesystem wrapper for scanModZipRW()
 * @param fullpath - full path to the .zip.
 */
void scanModZip(const std::string& fullpath) {
	std::string log_ctx = "scanModZip(" + fullpath + "): ";
	SDL_RWops *rwops = SDL_RWFromFile(fullpath.c_str(), "r");
	if (!rwops) {
		Log(LOG_WARNING) << log_ctx << "Ignoring zip: " << SDL_GetError();
		return;
	}
	scanModZipRW(rwops, fullpath);
}
/**
 * Extracts a single file to an ConstMem RWops object
 * @param rwops - .zip file
 * @param fullpath - what to extract, case and slash - insensitive
 */
SDL_RWops *zipGetFileByName(const std::string& zipfile, const std::string& fullpath) {
	std::string log_ctx = "zipGetFileByName(rwops, " + zipfile + ", " + fullpath + "): ";
	std::string sanpath = fullpath;
	if (!sanitizeZipEntryName(sanpath)) {
		Log(LOG_ERROR) << log_ctx << "Bogus fullpath given: '" << fullpath << "': " << hexDumpBogusData(sanpath);
		return NULL;
	}
	Unicode::lowerCase(sanpath);
	SDL_RWops *rwops = SDL_RWFromFile(zipfile.c_str(), "rb");
	if (!rwops) {
		Log(LOG_ERROR) << log_ctx << "SDL_RWFromFile(): " << SDL_GetError();
		return NULL;
	}
	mz_zip_archive *mzip = (mz_zip_archive *) SDL_malloc(sizeof(mz_zip_archive));
	if (!mzip) {
		Log(LOG_FATAL) << log_ctx << ": " << SDL_GetError();
		throw Exception("Out of memory");
	}
	if (!mz_zip_reader_init_rwops(mzip, rwops)) {
		Log(LOG_ERROR) << log_ctx << "Bad zip: " << mz_zip_get_error_string(mz_zip_get_last_error(mzip));
		SDL_RWclose(rwops);
		SDL_free(mzip);
		return NULL;
	}
	mz_uint filecount = mz_zip_reader_get_num_files(mzip);
	for (mz_uint fi = 0; fi < filecount; ++fi) {
		mz_zip_archive_file_stat fistat;
		mz_zip_reader_file_stat (mzip, fi, &fistat);
		if (fistat.m_is_encrypted || !fistat.m_is_supported) {
			continue;
		}
		if (fistat.m_is_directory) {
			continue;    // skip directories
		}
		std::string somepath = fistat.m_filename;
		if ( !sanitizeZipEntryName ( somepath ) ) {
			Log(LOG_WARNING) << "Bogus filename '" << somepath << "' "
							 << hexDumpBogusData(somepath) << " in the .zip, ignoring.";
			continue;
		}
		Unicode::lowerCase(somepath);
		if (sanpath == somepath) { // gotcha
			SDL_RWops *rv = SDL_RWFromMZ(mzip, fi);
			if (!rv) {
				Log(LOG_ERROR) << log_ctx << "Unzip failed: " << SDL_GetError();
			}
			mz_zip_reader_end(mzip);
			SDL_RWclose(rwops);
			SDL_free(mzip);
			return rv;
		}
	}
	Log ( LOG_ERROR ) << log_ctx << "File not found in the .zip";
	mz_zip_reader_end(mzip);
	SDL_RWclose(rwops);
	SDL_free(mzip);
	return NULL;
}
/**
 * this scans a mod dir.
 * which by definition is either a .zip
 * or a directory of zips and dirs.
 *
 * @param dirname - is either the datadir or the userdir
 * @param basename is then either 'standard' or 'mods'
 *
 * yes you can have mods.zip in userdir.
 * you can also have multimod zips in user/mods (and standard/mods too)
 *
 * and yes this has to parse the metadata.yml for the modId.
 *
 */
void scanModDir(const std::string& dirname, const std::string& basename, bool protectedLocation) {

	// "standard" directory is for built-in mods only! otherwise automatic updates would delete user data
	const static std::set<std::string> standardMods = {
		"Aliens_Pick_Up_Weapons",
		"Aliens_Pick_Up_Weapons_TFTD",
		"Demigod_Difficulty",
		"Demigod_Difficulty_TFTD",
		"Limit_Craft_Item_Capacities",
		"Limit_Craft_Item_Capacities_TFTD",
		"OpenXCom_Unlimited_Waypoints",
		"OpenXCom_Unlimited_Waypoints_TFTD",
		"PSX_Static_Cydonia_Map",
		"Smarter_Equip",
		"Smarter_Equip_TFTD",
		"StrategyCore_Swap_Small_USOs_TFTD",
		"TFTD_Damage",
		"UFOextender_Gun_Melee",
		"UFOextender_Gun_Melee_TFTD",
		"UFOextender_Psionic_Line_Of_Fire",
		"UFOextender_Psionic_Line_Of_Fire_TFTD",
		"UFOextender_Starting_Avalanches",
		"xcom1",
		"xcom2",
		"XcomUtil_Always_Daytime",
		"XcomUtil_Always_Daytime_TFTD",
		"XcomUtil_Always_Nighttime",
		"XcomUtil_Always_Nighttime_TFTD",
		"XcomUtil_Fighter_Transports",
		"XcomUtil_Fighter_Transports_TFTD",
		"XcomUtil_High_Explosive_Damage",
		"XcomUtil_High_Explosive_Damage_TFTD",
		"XcomUtil_Improved_Gauss",
		"XcomUtil_Improved_Ground_Tanks",
		"XcomUtil_Improved_Heavy_Laser",
		"XcomUtil_Infinite_Gauss",
		"XcomUtil_No_Psionics",
		"XcomUtil_No_Psionics_TFTD",
		"XcomUtil_Pistol_Auto_Shot",
		"XcomUtil_Pistol_Auto_Shot_TFTD",
		"XcomUtil_Skyranger_Weapon_Slot",
		"XcomUtil_Starting_Defensive_Base",
		"XcomUtil_Starting_Defensive_Base_TFTD",
		"XcomUtil_Starting_Defensive_Improved_Base",
		"XcomUtil_Starting_Defensive_Improved_Base_TFTD",
		"XcomUtil_Starting_Improved_Base",
		"XcomUtil_Starting_Improved_Base_TFTD",
		"XcomUtil_Statstrings",
		"XcomUtil_Statstrings_TFTD",
		"XcomUtil_Triton_Weapon_Slot",
		"XCOM_Damage"
	};

	std::string log_ctx = "scanModDir('" + dirname + "', '" + basename + "'): ";
 	// first check for a .zip
	std::string fullname = dirname + basename + ".zip";
	if (CrossPlatform::fileExists(fullname)) {
		Log(LOG_VERBOSE) << log_ctx << "scanning zip " << fullname;
		scanModZip(fullname);
	} else {
		Log(LOG_VERBOSE) << log_ctx << "no zip " << fullname;
	}
	// then check for a dir
	fullname = dirname  + basename;
	if (!CrossPlatform::folderExists(fullname)) {
		Log(LOG_VERBOSE) << log_ctx << "no dir " << fullname;
		return;
	}
	Log(LOG_VERBOSE) << log_ctx << "scanning dir " << fullname;

	// now this dir can contain both moddirs and modzips.
	// first scan for modzips : that is, anything but a directory
	auto contents = CrossPlatform::getFolderContents(fullname);
	std::vector<std::string> dirlist;
	for (auto zi = contents.begin(); zi != contents.end(); ++zi) {
		auto is_dir =  std::get<1>(*zi);
		if (is_dir) {
			if (protectedLocation)
			{
				if (standardMods.find(std::get<0>(*zi)) == standardMods.end())
				{
					Log(LOG_ERROR) << "Invalid standard mod '" << std::get<0>(*zi) << "', skipping.";
					continue;
				}
			}
			dirlist.push_back(std::get<0>(*zi)); // stash for later
			continue;
		}
		if (protectedLocation)
		{
			Log(LOG_ERROR) << "Invalid standard mod '" << std::get<0>(*zi) << "', skipping.";
			continue;
		}
		auto subpath = concatPaths(fullname, std::get<0>(*zi));
		scanModZip(subpath);
	}
	for (auto di = dirlist.begin(); di != dirlist.end(); ++di) {
		auto mp_basename = *di;
		auto modpath = concatPaths(fullname, mp_basename);
		// map dat dir! (if it has metadata.yml, naturally)
		auto layer = new VFSLayer(modpath);
		if (!layer->mapPlainDir(modpath)) {
			Log(LOG_WARNING) << log_ctx << "Can't scan " << mp_basename << ", skipping.";
			delete layer;
			continue;
		}
		auto frec = layer->at("metadata.yml");
		if (frec == NULL) { // whoa, no metadata
			Log(LOG_WARNING) << log_ctx << "No metadata.yml in " << mp_basename << ", skipping.";
			delete layer;
			continue;
		}
		auto doc = frec->getYAML();
		if (!doc.IsMap()) {
			Log(LOG_WARNING) << log_ctx << "Bad metadata.yml " << mp_basename << ", skipping.";
			delete layer;
			return;
		}
		auto mrec = new ModRecord(modpath);
		mrec->modInfo.load(doc);
		auto mri = ModsAvailable.find(mrec->modInfo.getId());
		if (mri != ModsAvailable.end()) {
			Log(LOG_ERROR) << log_ctx << "modId " << mrec->modInfo.getId() << " already mapped in, skipping " << mp_basename;
			delete layer;
			delete mrec;
			continue;
		}
		Log(LOG_VERBOSE) << log_ctx << "modId " << mrec->modInfo.getId() << " mapped in from " << mp_basename;
		MappedVFSLayers.insert(layer);
		mrec->push_back(layer);
		ModsAvailable.insert(std::make_pair(mrec->modInfo.getId(), mrec));
	}
}
/**
 * A helper that drops listed mods and dependent ones.
 * @param log_ctx ~
 * @param drop_list mods to drop (mutated and then cleared)
 */
static void drop_mods(const std::string& log_ctx, std::unordered_set<std::string>& drop_list) {
	bool dropped_something = true;
	while(dropped_something) {
		dropped_something = false;
		for (auto mri = ModsAvailable.begin(); mri != ModsAvailable.end(); ++mri) {
			auto modId = mri->first;
			auto mrec = mri->second;
			if (drop_list.find(modId) != drop_list.end()) { // already dropped.
				continue;
			}
			auto masterId = mrec->modInfo.getMaster();
			if (drop_list.find(masterId) != drop_list.end()) {
				Log(LOG_DEBUG) << log_ctx << "Dropping mod " << modId << ": depends " << masterId;
				drop_list.insert(modId);
				dropped_something = true;
			}
		}
	}
	for (auto ki = drop_list.begin(); ki != drop_list.end(); ++ki) {
		delete ModsAvailable.at(*ki);
		ModsAvailable.erase(*ki);
	}
	drop_list.clear();
}
/**
 * Enforces mod list sanity:
 * 	- breaks circular dependency loops
 *  - removes mods that miss their extResources or masters
 */
void checkModsDependencies() {
	std::string log_ctx = "checkModsDependencies(): ";
	Log(LOG_VERBOSE) << log_ctx << "called.";
	std::unordered_set<std::string> drop_list;

	// break circular dependency loops.
	for (auto mri = ModsAvailable.begin(); mri != ModsAvailable.end(); ++mri) {
		auto started_at = mri->first;
		auto current = started_at;
		while(true) {
			auto masterId = ModsAvailable.at(current)->modInfo.getMaster();
			if (masterId.empty()) { break; } 							// no loop here, move along.
			if (drop_list.find(masterId) != drop_list.end()) { break; } // this loop is already broken
			if ( ModsAvailable.find(masterId) == ModsAvailable.end()) {				// master missing
				Log(LOG_WARNING) << log_ctx << current << ": missing master mod " << masterId;
				drop_list.insert(started_at);
				break;
			}
			if (masterId == started_at) { 								// whoa, loop detected
				Log(LOG_WARNING) << log_ctx << current << ": dependency loop detected for " << masterId;
				drop_list.insert(started_at);
				break;
			}
			current = masterId;
		}
	}
	// at this point all loops are broken, let's drop mods involved
	// and anything that depends on them
	drop_mods(log_ctx, drop_list);

	// now let's attempt mapping extResources for all the remaining mods
	// and drop any mod where this fails.
	for (auto mri = ModsAvailable.begin(); mri != ModsAvailable.end(); ++mri) {
		auto modId = mri->first;
		auto mrec = mri->second;
		auto resdirs = mrec->modInfo.getExternalResourceDirs();
		for (auto eri = resdirs.begin(); eri != resdirs.end(); ++eri) {
			if (!mapExtResources(mrec, *eri, false)) {
				Log(LOG_DEBUG) << log_ctx << "dropping mod " << modId << ": extResources '" << *eri << "' not found.";
				drop_list.insert(modId);
				break;
			}
		}
	}
	drop_mods(log_ctx, drop_list);
}
// returns currently mapped bunch of mods.
std::map<std::string, ModInfo> getModInfos() {
	std::map<std::string, ModInfo> rv;
	for (auto mri = ModsAvailable.begin(); mri != ModsAvailable.end(); ++mri) {
		rv.insert(std::make_pair(mri->first, mri->second->modInfo));
	}
	return rv;
}

const FileRecord* getModRuleFile(const ModInfo* modInfo, const std::string& relpath)
{
	if (!relpath.empty())
	{
		auto fullPath = concatPaths(modInfo->getPath(), relpath);
		for (auto& r : ModsAvailable.at(modInfo->getId())->stack.rulesets)
		{
			if (r.fullpath == fullPath)
			{
				return &r;
			}
		}
		Log(LOG_WARNING) << "mod " << modInfo->getId() << ": unknown rulefile '" << relpath <<"'.";
	}
	return nullptr;
}

std::string canonicalize(const std::string &in)
{
	std::string ret = in;
	Unicode::lowerCase(ret);
	return ret;
}
bool fileExists(const std::string &relativeFilePath) {
	return ( TheVFS.at(relativeFilePath) != NULL);
}
const FileRecord *at(const std::string &relativeFilePath) {
	auto frec = TheVFS.at(relativeFilePath);
	if (frec == NULL) {
		std::string fail = "FileRecord::at(" + relativeFilePath + "): requested file not found.";
		Log(LOG_FATAL) << fail;
		throw Exception(fail);
	}
	return frec;
}

SDL_RWops *getRWops(const std::string &relativeFilePath)
{
	return at(relativeFilePath)->getRWops();
}
SDL_RWops *getRWopsReadAll(const std::string &relativeFilePath)
{
	return at(relativeFilePath)->getRWopsReadAll();
}

std::unique_ptr<std::istream> getIStream(const std::string &relativeFilePath) {
	return at(relativeFilePath)->getIStream();
}
YAML::Node getYAML(const std::string &relativeFilePath) {
	return at(relativeFilePath)->getYAML();
}
std::vector<YAML::Node> getAllYAML(const std::string &relativeFilePath) {
	return at(relativeFilePath)->getAllYAML();
}
const std::vector<const FileRecord *> getSlice(const std::string &relativeFilePath) {
	return TheVFS.get_slice(relativeFilePath);
}
const NameSet &getVFolderContents(const std::string &relativePath) {
	return TheVFS.ls(relativePath);
}
const NameSet &getVFolderContents(const std::string &relativePath, size_t level) {
	return TheVFS.lslayer(relativePath, level);
}
template <typename T>
NameSet _filterFiles(const T &files, const std::string &ext)
{
	NameSet ret;
	size_t extLen = ext.length() + 1; // +1 for the '.'
	std::string canonicalExt = canonicalize(ext);
	for (typename T::const_iterator i = files.begin(); i != files.end(); ++i)
	{
		// less-than not less-than-or-equal since we should have at least
		// one character in the filename that is not part of the extension
		if (extLen < i->length() && 0 == canonicalize(i->substr(i->length() - (extLen - 1))).compare(canonicalExt))
		{
			ret.insert(*i);
		}
	}
	return ret;
}

NameSet filterFiles(const std::vector<std::string> &files, const std::string &ext) { return _filterFiles(files, ext); }
NameSet filterFiles(const std::set<std::string>    &files, const std::string &ext) { return _filterFiles(files, ext); }
NameSet filterFiles(const NameSet                  &files, const std::string &ext) { return _filterFiles(files, ext); }

}

}
