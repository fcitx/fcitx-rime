#ifndef FCITX_RIME_MODEL_H
#define FCITX_RIME_MODEL_H

#include <rime_api.h>

#include "fcitx-utils/keysym.h"
#include "fcitx-utils/keysymgen.h"
#include <qkeysequence.h>
#include <QFlags>

#define MAX_SHORTCUTS 3
#define DEFAULT_PAGE_SIZE 5

namespace fcitx_rime {
  typedef QFlags<fcitx::KeyState> KeyStates;
  typedef FcitxKeySym KeySym;
  class FcitxKeySeq {
  public:
    KeyStates states_;
    KeySym sym_;
    FcitxKeySeq();
    FcitxKeySeq(const char* keyseq);
    FcitxKeySeq(const QKeySequence qkey);
    std::string toString() const;
    std::string keySymToString(KeySym sym) const;
    KeySym keySymFromString(const char* keyString);
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
    void sortSingleKeySet(QVector<FcitxKeySeq>& keys);
  };
}

#endif // FCITX_RIME_MODEL_H
