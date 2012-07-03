# Author: Marguerite Su <i@marguerite.su>
# License: same as fcitx
# Description: find RIME brise schemas collection package.
# BRISE_FOUND - System has brise package
# BRISE_DIR - Brise absolute path

if(IS_DIRECTORY /usr/share/brise)
	set(BRISE_DIR /usr/share/brise)
else(IS_DIRECTORY /usr/share/rime/brise)
	set(BRISE_DIR /usr/share/rime/brise)
else
	set(BRISE_FOUND FALSE)
endif

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Brise DEFAULT_MSG
				  BRISE_DIR)
mark_as_advanced(BRISE_DIR)
