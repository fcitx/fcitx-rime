#ifndef FCITX_RIME_CONFIGMAIN_H
#define FCITX_RIME_CONFIGMAIN_H

#include <fcitxqtconfiguiwidget.h>
#include <fcitxqtkeysequencewidget.h>

#include "Model.h"
#include "rime-utils/FcitxRimeConfig.h"

namespace Ui {
  class MainUI;
}

namespace fcitx_rime {
  class ConfigMain : public FcitxQtConfigUIWidget {
    Q_OBJECT
  public:
    explicit ConfigMain(QWidget* parent = 0);
    QString title();
    ~ConfigMain();
    void load();
    void save();
    
    QString addon();
    QString icon();
  public slots:
    void keytoggleChanged();
    void stateChanged();
    void addIM();
    void removeIM();
    void moveUpIM();
    void moveDownIM();
    void availIMSelectionChanged();
    void activeIMSelectionChanged();
  private:
    Ui::MainUI* m_ui;
    FcitxRime* rime;
    FcitxRimeConfigDataModel* model;
    void test();
    void setFcitxQtKeySeq(char* rime_key, FcitxKeySeq& keyseq);
    void yamlToModel();
    void uiToModel();
    void modelToUi();
    void modelToYaml();
    void getAvailableSchemas();
    void updateIMList();
    void focusSelectedIM(const QString im_name);
    QList<FcitxQtKeySequenceWidget*> getKeyWidgetsFromLayout(QLayout *layout);
    void setKeySeqFromLayout(QLayout *layout, QVector<FcitxKeySeq>& model_keys);
    void setModelFromLayout(QVector<FcitxKeySeq>& model_keys, QLayout *layout);
    void setModelKeysToYaml(QVector<FcitxKeySeq>& model_keys, int type, const char* key);
  };
  
}

#endif // FCITX_RIME_CONFIGMAIN_H
