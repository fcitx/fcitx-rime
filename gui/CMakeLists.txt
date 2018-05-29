set(REQUIRED_QT_VERSION 5.1.0)

find_package(FcitxQt5WidgetsAddons 1.0.0 REQUIRED)
find_package(Qt5 ${REQUIRED_QT_VERSION} CONFIG REQUIRED Core Widgets Concurrent)
find_package(Rime REQUIRED)
pkg_check_modules(FcitxQt "fcitx-qt>=4.2.8" REQUIRED IMPORTED_TARGET)
pkg_check_modules(Rime "rime>=1.0.0" REQUIRED IMPORTED_TARGET)

add_subdirectory(src)
