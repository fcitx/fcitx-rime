#include <qplugin.h>
#include <fcitx-utils/utils.h>

#include "ConfigMain.h"
#include "Main.h"

// FcitxQtConfigUIPlugin : QObject, FcitxQtConfigUIFactoryInterface
FcitxRimeConfigTool::FcitxRimeConfigTool(QObject* parent)
  : FcitxQtConfigUIPlugin(parent) {
  if (parent == NULL) {
  }
}

FcitxQtConfigUIWidget* FcitxRimeConfigTool::create(const QString& key)
{
  Q_UNUSED(key);
  return new fcitx_rime::ConfigMain;
}

QString FcitxRimeConfigTool::name() {
  return "rime-config-gui-tool";
}

QStringList FcitxRimeConfigTool::files() {
  return QStringList("rime/config");
}

QString FcitxRimeConfigTool::domain() {
  return "fcitx_rime";
}
