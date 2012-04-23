
if(Boost_FOUND)
  include_directories(${Boost_INCLUDE_DIRS})
  link_directories(${Boost_LIBRARY_DIRS})
endif(Boost_FOUND)
if(YamlCpp_FOUND)
  include_directories(${YamlCpp_INCLUDE_DIR})
endif(YamlCpp_FOUND)
if(KyotoCabinet_FOUND)
  include_directories(${KyotoCabinet_INCLUDE_DIR})
endif(KyotoCabinet_FOUND)
if(Opencc_FOUND)
  include_directories(${Opencc_INCLUDE_DIR})
endif(Opencc_FOUND)

include_directories(${FCITX4_FCITX_INCLUDE_DIRS})
include_directories(${FCITX4_FCITX_CONFIG_INCLUDE_DIRS})
include_directories(${FCITX4_FCITX_UTILS_INCLUDE_DIRS})

include_directories(${PROJECT_SOURCE_DIR}/../librime/include)
link_directories(${PROJECT_SOURCE_DIR}/../librime/build/lib)

set(FCITX_RIME_SRC fcitx-rime.c)
fcitx_add_addon(fcitx-rime ${FCITX_RIME_SRC})
target_link_libraries(fcitx-rime rime ${Boost_LIBRARIES} ${YamlCpp_LIBRARY} ${KyotoCabinet_LIBRARY} ${Opencc_LIBRARY} ${ZLIB_LIBRARY} ${CMAKE_THREAD_LIBS_INIT})

fcitx_add_addon_conf_file(fcitx-rime.conf)
fcitx_add_inputmethod_conf_file(rime.conf)


add_executable(build_yaml main.c)
target_link_libraries(build_yaml rime)