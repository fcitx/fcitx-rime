# for im icon
install(FILES rime.png RENAME rime.png DESTINATION ${FCITX4_PREFIX}/share/fcitx/imicon)

# for skin
foreach(png rime-sync rime-im rime-disable rime-latin rime-deploy)
install(FILES ${png}.png DESTINATION ${FCITX4_PREFIX}/share/fcitx/skin/default)
endforeach()

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