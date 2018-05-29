# for im icon
install(FILES rime.png RENAME rime.png DESTINATION ${FCITX4_PREFIX}/share/fcitx/imicon)

# for skin
install(DIRECTORY default DESTINATION ${FCITX4_PREFIX}/share/fcitx/skin)

# with prefix
foreach(svg rime rime-deploy rime-sync)
install(FILES ${svg}.png RENAME fcitx-${svg}.png DESTINATION share/icons/hicolor/48x48/apps)
install(FILES ${svg}.svg RENAME fcitx-${svg}.svg DESTINATION share/icons/hicolor/scalable/status)
endforeach()

# without prefix
foreach(svg rime-im rime-latin rime-disable)
install(FILES ${svg}.png DESTINATION share/icons/hicolor/48x48/apps)
install(FILES ${svg}.svg DESTINATION share/icons/hicolor/scalable/status)
endforeach()

# install the config tool icon
install(FILES rime128.png RENAME fcitx-rime.png DESTINATION share/icons/hicolor/128x128/apps)

