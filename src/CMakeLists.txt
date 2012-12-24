
include_directories(${FCITX4_FCITX_INCLUDE_DIRS})
include_directories(${FCITX4_FCITX_CONFIG_INCLUDE_DIRS})
include_directories(${FCITX4_FCITX_UTILS_INCLUDE_DIRS})

include_directories(${Rime_INCLUDE_DIR})

set(FCITX_RIME_SRC fcitx-rime.c)

fcitx_add_addon_full(rime
  SOURCES ${FCITX_RIME_SRC}
  IM_CONFIG rime.conf
  LINK_LIBS ${Rime_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})
