set(FRCU_SRCS
  Main.cpp
  ConfigMain.cpp
  Model.cpp
  FcitxRimeConfig.cpp)
set(FRCU_HDRS
  Main.h
  ConfigMain.h
  Model.h
  Common.h
  FcitxRimeConfig.h)

set(FRCU_UIS
  ConfigMain.ui)

fcitx_translate_add_sources(
  ${FRCU_SRCS}
  ${FRCU_HDRS}
  ${FRCU_UIS})

add_library(fcitx-rime-config-gui
  MODULE ${FRCU_SRCS})

set(AUTOGEN_BUILD_DIR "${CMAKE_BINARY_DIR}/gui/src/fcitx-rime-config-gui_autogen/include")
file(COPY "${CMAKE_SOURCE_DIR}/gui/src/Common.h" DESTINATION "${AUTOGEN_BUILD_DIR}")

set_target_properties(fcitx-rime-config-gui PROPERTIES
  LINK_FLAGS "-Wl,--no-undefined"
  AUTOMOC TRUE
  AUTOUIC TRUE
  AUTOUIC_OPTIONS "-tr=fcitx_rime::tr2fcitx;--include=Common.h")

target_link_libraries(fcitx-rime-config-gui
  Qt5::Widgets
  Qt5::Concurrent
  FcitxQt5::WidgetsAddons
  PkgConfig::FcitxQt
  PkgConfig::Rime
  ${FCITX4_FCITX_UTILS_LIBRARIES}
  ${FCITX4_FCITX_CONFIG_LIBRARIES})


install(TARGETS fcitx-rime-config-gui DESTINATION ${FCITX4_ADDON_INSTALL_DIR}/qt)
