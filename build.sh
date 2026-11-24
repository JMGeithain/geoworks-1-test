#!/bin/sh
# Baut GWTEST.GEO ("GeoWorks 1.x Test") mit dem SDK pcgeos-1-sdk-v2
# (https://github.com/dnesbitt142/pcgeos-1-sdk-v2). Aufruf:
#   SDK=/pfad/zu/pcgeos-1-sdk-v2 ./build.sh [test]
# Ergebnis: out/GWTEST.GEO (-> GEOWORKS\WORLD)
set -e
cd "$(dirname "$0")"
HERE=$(pwd)
SDK=${SDK:-$HERE/../pcgeos-1-sdk-v2}
export SDK
(cd icon && python3 make_icon.py >/dev/null && python3 build_monikers.py >/dev/null)
python3 tools/gen_icon_inc.py
python3 tools/gen_texts.py >/dev/null
(cd "$SDK" && perl sdk.pl build --project "$HERE/app" >/dev/null)
mkdir -p out
perl tools/postbuild.pl "$SDK/build/gwtest/gwtest.geo" out/GWTEST.GEO icon 640 460
# Emulator-Test (braucht python3 mit unicorn und pillow)
if [ "$1" = "test" ]; then
  (cd test && nasm -f bin -I "$SDK/tools/ensemble12/" -I ../app/ -o g.bin wrap.nasm && python3 test_gwtest.py)
fi
