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
