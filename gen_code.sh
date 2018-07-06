#!/bin/bash
if [[ "$1" = "" ]]; then
    echo Fatal: must specify a dll.
    exit
elif [[ "$1" = "--help" ]] || [[ "$1" = "-h" ]]; then
    echo Usage: ./gen_code.sh dll_name.dll
    echo dll_name.dll should be in "${WINDIR}/System32/"
    echo Requirements: gendef
    exit
fi
dll_path="$(which "$1")"
dll_file="$(basename "${dll_path}")"
dll_name="$(echo "${dll_file}" | cut -d'.' -f1)"
if [[ "${dll_name}" = "" ]]; then
    echo Fatal: dll "$1" not found. Try specifying full dll name with extension.
    exit
fi

gen_header() {
    local count=1
    echo "#define DLL_NAME \"${dll_name}\""
    echo "#include \"hijack.h\""
    echo
    for i in $(gendef - "${dll_path}" | cut -d'=' -f1 | grep -ivP 'LIBRARY|EXPORTS|;'); do
        echo "EXPORT ${i}() NOP_FUNC($((count++)));"
    done
}

gen_header > "./${dll_name}.h"
echo "Generated ${dll_name}.h"

cat <<EOF > "./${dll_name}.c"
#include "${dll_name}.h"
#include "hook_knowndir.h"
EOF

echo "Generated ${dll_name}.c"

sed -i "/add_library(${dll_name} SHARED/d" CMakeLists.txt
echo "add_library(${dll_name} SHARED ${dll_name}.c ${dll_name}.h \${SRC} \${MinHook})" >> CMakeLists.txt
echo Added "${dll_name}" entry to CMakeLists.txt