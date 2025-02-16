/*
 * Copyright 2010-2024 OpenXcom Developers.
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
# include "Position.h"

namespace OpenXcom
{

// helper overloads for (de)serialization
bool read(ryml::ConstNodeRef const& n, Position* val)
{
	YAML::YamlNodeReader reader(n);
	val->x = reader[0].readVal<Sint16>();
	val->y = reader[1].readVal<Sint16>();
	val->z = reader[2].readVal<Sint16>();
	return true;
}
void write(ryml::NodeRef* n, Position const& val)
{
	YAML::YamlNodeWriter writer(*n);
	writer.setAsSeq();
	writer.setFlowStyle();
	writer.write(val.x);
	writer.write(val.y);
	writer.write(val.z);
}

}
