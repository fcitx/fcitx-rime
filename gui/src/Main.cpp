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
//
// You should have received a copy of the GNU General Public License
// along with this program; see the file COPYING. If not,
// see <http://www.gnu.org/licenses/>.
//
#include <fcitx-utils/utils.h>
#include <qplugin.h>

#include "ConfigMain.h"
#include "Main.h"

// FcitxQtConfigUIPlugin : QObject, FcitxQtConfigUIFactoryInterface
RimeConfigParserTool::RimeConfigParserTool(QObject *parent)
    : FcitxQtConfigUIPlugin(parent) {
    if (parent == NULL) {
    }
}

FcitxQtConfigUIWidget *RimeConfigParserTool::create(const QString &key) {
    Q_UNUSED(key);
    return new fcitx_rime::ConfigMain;
}

QString RimeConfigParserTool::name() { return "rime-config-gui-tool"; }

QStringList RimeConfigParserTool::files() { return QStringList("rime/config"); }

QString RimeConfigParserTool::domain() { return "fcitx_rime"; }
