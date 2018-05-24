//
// Copyright (C) 2018~2018 by xuzhao9 <i@xuzhao.net>
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License,
// or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
// General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with this program; see the file COPYING. If not,
// see <http://www.gnu.org/licenses/>.
//
#ifndef FCITX_RIME_CONFIG_COMMON_H
#define FCITX_RIME_CONFIG_COMMON_H

#include <libintl.h>
#include <QString>

#define _(x) QString::fromUtf8(dgettext("fcitx-rime", x))

#define FCITX_RIME_ARRAY_SIZE(array) (sizeof(array) / sizeof(array[0]))

namespace fcitx_rime {
  inline QString tr2fcitx(const char *message, const char *comment = nullptr) {
    if (message && message[0]) {
      return QString(_(message));
    } else {
      return QString();
    }
  }
};
#endif // _FCITXRIMECONFIGCOMMON_H_
