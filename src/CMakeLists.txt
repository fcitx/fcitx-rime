
include_directories(${FCITX4_FCITX_INCLUDE_DIRS})
include_directories(${FCITX4_FCITX_CONFIG_INCLUDE_DIRS})
include_directories(${FCITX4_FCITX_UTILS_INCLUDE_DIRS})

include_directories(${Rime_INCLUDE_DIR})

set(FCITX_RIME_SRC fcitx-rime.c)
fcitx_add_addon(fcitx-rime ${FCITX_RIME_SRC})
target_link_libraries(fcitx-rime ${Rime_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})

fcitx_add_addon_conf_file(fcitx-rime.conf)
fcitx_add_inputmethod_conf_file(rime.conf)
