file(READ "${ICON_FILE}" ICON_HEX HEX)

string(REGEX REPLACE "(..)" "0x\\1, " ICON_BYTES "${ICON_HEX}")

file(WRITE "${GENERATED_ICON_SOURCE}"
    "#include \"platform/pc/PcGameIcon.hpp\"\n"
    "#include <cstddef>\n\n"
    "const unsigned char game_icon[] = {\n"
    "    ${ICON_BYTES}\n"
    "};\n"
    "const size_t game_icon_len = sizeof(game_icon);\n"
)
