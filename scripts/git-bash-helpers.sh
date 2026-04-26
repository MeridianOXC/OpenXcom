#!/bin/bash

if [[ -z "$1" ]];
then
  echo "alias prefix required"
  exit 1;
fi;

git config alias.$1-clean '!'"f() { (git diff --quiet --cached --exit-code || (echo 'not commited changes'; false)) && (git diff --quiet --exit-code || (echo 'working dir dirty'; false)); }; f"
git config alias.$1-version-bump '!'"f() { vim -T dumb --noplugin -n -c 'silent! %s/\\(\"Extended \\d\\+[.,]\\d\\+\\)\"/\\=submatch(1).\".0\\\"\"/g' -c '%s/\\%\\(#define MIN_REQUIRED_RULESET_VERSION_NUMBER\\)\\@<!\\([\" ]\\d\\+[.,]\\d\\+[.,]\\)\\(\\d\\+\\)/\\=submatch(1).eval(submatch(2)+1)/g' -c '%s/(v\\d\\d\\d\\d-\\d\\d-\\d\\d)/\\=\"(v\".strftime(\"%Y-%m-%d\").\")\"/g' -c ':wq' src/version.h && git add src/version.h; }; f"
git config alias.$1-version-bump-commit '!'"f() { git $1-clean && git $1-version-bump && git commit --no-edit -m \"OXCE \$(grep -oP '(?<=Extended )\\d+\\.\\d+(\\.\\d+)?' src/version.h)\"; }; f"
git config alias.$1-version-set '!'"f() { ([[ \$# -eq 2 ]] || (echo 'wrong args num'; false)) && vim -T dumb --noplugin -n -c '%s/\\%\\(#define MIN_REQUIRED_RULESET_VERSION_NUMBER \\)\\@<!\\d\\+[.,]\\d\\+\\([.,]\\)\\d\\+/\\=\"'\$1'\".submatch(1).\"'\$2'\".submatch(1).\"0\"/g' -c '%s/(v\\d\\d\\d\\d-\\d\\d-\\d\\d)/\\=\"(v\".strftime(\"%Y-%m-%d\").\")\"/g' -c ':wq' src/version.h && git add src/version.h && git $1-version-common-set \"\$@\" 0; }; f"
git config alias.$1-version-set-commit '!'"f() { git $1-clean && git $1-version-set \"\$@\" && git commit --no-edit -m \"OXCE v\$1.\$2\"; }; f"
git config alias.$1-version-common-set '!'"f() { ([[ \$# -eq 3 ]] || (echo 'wrong args num'; false)) && vim -T dumb --noplugin -n -c '%s/\\%\\(#define MIN_REQUIRED_RULESET_VERSION_NUMBER \\)\\@<=\\d\\+,\\d\\+,\\d\\+,\\d\\+/\\=\"'\$1','\$2','\$3',0\"/g' -c ':wq' src/version.h && git add src/version.h && vim -T dumb --noplugin -n -c 'argdo %s/\\%\\(version: \\)\\@<=\\(\\d\\+.\\d\\+.\\d\\+\\)/\\=\"'\$1'.'\$2'.'\$3'\"/g | update' -c 'qa' bin/common/dont-touch.me bin/standard/xcom1/metadata.yml bin/standard/xcom2/metadata.yml && git add -f bin/common/dont-touch.me bin/standard/xcom1/metadata.yml bin/standard/xcom2/metadata.yml && git add src/version.h; }; f"
