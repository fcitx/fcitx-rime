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
#ifndef FCITX_RIME_MODEL_H
#define FCITX_RIME_MODEL_H

#include "keysym.h"
#include "keysymgen.h"
#include <QFlags>
#include <QKeySequence>
#include <QVector>
#include <rime_api.h>

static constexpr int max_shortcuts = 3;
static constexpr int default_page_size = 5;

namespace fcitx_rime {
typedef QFlags<fcitx::KeyState> KeyStates;
typedef FcitxKeySym KeySym;
class FcitxKeySeq {
public:
    KeyStates states_;
    KeySym sym_;
    FcitxKeySeq();
    FcitxKeySeq(const char *keyseq);
    FcitxKeySeq(const QKeySequence qkey);
    std::string toString() const;
    std::string keySymToString(KeySym sym) const;
    KeySym keySymFromString(const char *keyString);
};

class FcitxRimeSchema {
public:
    QString path;
    QString id;
    QString name;
    int index; // index starts from 1, 0 means not enabled
    bool active;
};

class FcitxRimeConfigDataModel {
public:
    QVector<FcitxKeySeq> toggle_keys;
    QVector<FcitxKeySeq> ascii_key;
    QVector<FcitxKeySeq> trasim_key;
    QVector<FcitxKeySeq> halffull_key;
    QVector<FcitxKeySeq> pgup_key;
    QVector<FcitxKeySeq> pgdown_key;
    int candidate_per_word;
    QVector<FcitxRimeSchema> schemas_;
    void sortSchemas();
    void sortKeys();

private:
    void sortSingleKeySet(QVector<FcitxKeySeq> &keys);
};
} // namespace fcitx_rime

#endif // FCITX_RIME_MODEL_H
