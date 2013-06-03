set(PO_LANGS de zh_CN ja)
foreach(lang ${PO_LANGS})
  fcitx_translate_add_po_file("${lang}" "${lang}.po")
endforeach()
fcitx_translate_set_pot_target(pot fcitx-rime fcitx-rime.pot)


