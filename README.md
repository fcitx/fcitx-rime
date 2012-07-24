## RIME support for Fcitx

RIME(中州韻輸入法引擎) is _mainly_ a Traditional Chinese input method engine.

project: https://code.google.com/p/rimeime


## Build From Source:

### special notice of RIME dependency:

RIME split its devlopment source into a few sections, here we need librime.

In librime source, there are two directory, brise and librime.

According to your distribution, you might need brise+librime package(openSUSE) or only librime which includes brise at /usr/share/brise.

If your distribution doesn't have one you need to download librime and put brise directory into /usr/share/brise.

If you're a distribution packager, ask maintainer of librime to add brise sub-package.

### special notice of Boost dependency:

Boost is a RIME dependency, so without boost >= 1.46.1, you will not ble to install librime-devel.

Generally it means, distros that are a little old like openSUSE 11.4 or Ubuntu 10.10 might not be possible to build or install.

### Dependency

*cmake

*gcc-c++

*intltool

*fcitx-devel with all three skins

 some distro like openSUSE split a fcitx-skin-classic and a fcitx-skin-dark, so you need them.

*librime-devel

*brise

*hicolor-icon-theme 

 optional, for directory ownership.

#### openSUSE: 

	sudo zypper ar -f http://download.opensuse.org/repositories/M17N/openSUSE_12.2/ M17N

	sudo zypper in cmake gcc-c++ fcitx-devel fcitx-skin-classic fcitx-skin-dark librime-devel brise hicolor-icon-theme


## Install from Distribution

### openSUSE

	sudo zypper ar -f http://download.opensuse.org/repositories/M17N/openSUSE_12.2/ M17N

	sudo zypper in fcitx-rime


