#!/bin/bash
source ~/emsdk/emsdk_env.sh
emcc -fwasm-exceptions -Oz virtuoso.cpp ../miniz.c ../miniz_tdef.c ../miniz_tinfl.c -I../inc -I../miniz -I.. -s EXPORT_NAME="'Virtuoso'" -sEXPORTED_FUNCTIONS=_compile,_encodeJSON,_deflateJSON,_encodeData,_encodeText,_decodeData,_decodeText,_decodeJSON,_inflateJSON,_malloc,_free -o virtuoso.js