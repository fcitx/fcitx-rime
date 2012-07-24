# Author: Marguerite Su <i@marguerite.su>
# License: same as fcitx
# Description: find RIME brise schemas collection package.
# BRISE_FOUND - System has brise package
# BRISE_DIR - Brise absolute path

set(BRISE_FIND_DIR "${PROJECT_SOURCE_DIR}/../brise"
                   "${CMAKE_INSTALL_PREFIX}/share/brise"
                   "${CMAKE_INSTALL_PREFIX}/share/rime/brise"
                   "/usr/share/brise"
                   "/usr/share/rime/brise")

set(BRISE_FOUND FALSE)

foreach(_BRISE_DIR ${BRISE_FIND_DIR})
    if (IS_DIRECTORY ${_BRISE_DIR})
        set(BRISE_FOUND True)
        set(BRISE_DIR ${_BRISE_DIR})
    endif (IS_DIRECTORY ${_BRISE_DIR})
endforeach(_BRISE_DIR)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Brise DEFAULT_MSG
				  BRISE_DIR)
mark_as_advanced(BRISE_DIR)
