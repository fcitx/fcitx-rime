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
#ifndef FCITX_RIME_MAIN_H
#define FCITX_RIME_MAIN_H

#include <fcitxqtconfiguiplugin.h>

class FcitxRimeConfigTool : public FcitxQtConfigUIPlugin {
    Q_OBJECT
public:
    Q_PLUGIN_METADATA(IID "FcitxQtConfigUIFactoryInterface_iid" FILE "fcitx-rime-config.json")
    explicit FcitxRimeConfigTool(QObject* parent = 0);
    virtual QString name() override;
    virtual QStringList files() override;
    virtual QString domain() override;
    virtual FcitxQtConfigUIWidget* create(const QString& key) override;
};

#endif // FCITX_RIME_MAIN_H
