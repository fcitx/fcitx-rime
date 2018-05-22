#ifndef FCITX_RIME_MAIN_H
#define FCITX_RIME_MAIN_H

#include <fcitxqtconfiguiplugin.h>

class FcitxRimeConfigTool : public FcitxQtConfigUIPlugin {
    Q_OBJECT
public:
    Q_PLUGIN_METADATA(IID "FcitxQtConfigUIFactoryInterface_iid" FILE "fcitx-rime-config.json")
    explicit FcitxRimeConfigTool(QObject* parent = 0);
    virtual QString name();
    virtual QStringList files();
    virtual QString domain();
    virtual FcitxQtConfigUIWidget* create(const QString& key);
};

#endif // FCITX_RIME_MAIN_H
