file(GLOB PO_FILES RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}" *.po)

foreach(po_file ${PO_FILES})
  string(REPLACE ".po" "" po_lang "${po_file}")
  fcitx_translate_add_po_file("${po_lang}" "${po_file}")
endforeach()
fcitx_translate_set_pot_target(pot fcitx-rime fcitx-rime.pot)


