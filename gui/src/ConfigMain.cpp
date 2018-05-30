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
#include <fcitx-config/xdg.h>

#include <QDialogButtonBox>
#include <QFutureWatcher>
#include <QTreeWidgetItem>
#include <QtConcurrentRun>

#include "Common.h"
#include "ConfigMain.h"
#include <QListWidgetItem>
#include <QStandardItemModel>

// TODO: when failed-read happens, disable ui
// TODO: when failed-save happens, disable ui and show reason

namespace fcitx_rime {
ConfigMain::ConfigMain(QWidget *parent)
    : FcitxQtConfigUIWidget(parent), model(new FcitxRimeConfigDataModel()) {
    // Setup UI
    setMinimumSize(680, 500);
    setupUi(this);
    verticallayout_general->setAlignment(Qt::AlignTop);
    addIMButton->setIcon(QIcon::fromTheme("go-next"));
    removeIMButton->setIcon(QIcon::fromTheme("go-previous"));
    moveUpButton->setIcon(QIcon::fromTheme("go-up"));
    moveDownButton->setIcon(QIcon::fromTheme("go-down"));
    // configureButton->setIcon(QIcon::fromTheme("help-about"));
    // listViews for currentIM and availIM
    QStandardItemModel *listModel = new QStandardItemModel();
    currentIMView->setModel(listModel);
    QStandardItemModel *availIMModel = new QStandardItemModel();
    availIMView->setModel(availIMModel);
    // tab shortcut
    connect(cand_cnt_spinbox, SIGNAL(valueChanged(int)), this,
            SLOT(stateChanged()));
    QList<FcitxQtKeySequenceWidget *> keywgts =
        general_tab->findChildren<FcitxQtKeySequenceWidget *>();
    for (size_t i = 0; i < keywgts.size(); i++) {
        connect(keywgts[i],
                SIGNAL(keySequenceChanged(QKeySequence, FcitxQtModifierSide)),
                this, SLOT(keytoggleChanged()));
    }
    // tab schemas
    connect(removeIMButton, SIGNAL(clicked(bool)), this, SLOT(removeIM()));
    connect(addIMButton, SIGNAL(clicked(bool)), this, SLOT(addIM()));
    connect(moveUpButton, SIGNAL(clicked(bool)), this, SLOT(moveUpIM()));
    connect(moveDownButton, SIGNAL(clicked(bool)), this, SLOT(moveDownIM()));
    connect(availIMView->selectionModel(),
            SIGNAL(currentChanged(QModelIndex, QModelIndex)), this,
            SLOT(availIMSelectionChanged()));
    connect(currentIMView->selectionModel(),
            SIGNAL(currentChanged(QModelIndex, QModelIndex)), this,
            SLOT(activeIMSelectionChanged()));
    rime = FcitxRimeConfigCreate();
    FcitxRimeConfigStart(rime);
    yamlToModel();
    modelToUi();
}

ConfigMain::~ConfigMain() {
    FcitxRimeDestroy(rime);
    delete model;
}

void ConfigMain::keytoggleChanged() { stateChanged(); }

// SLOTs
void ConfigMain::stateChanged() { emit changed(true); }

void ConfigMain::focusSelectedIM(const QString im_name) {
    // search enabled IM first
    int sz = currentIMView->model()->rowCount();
    for (int i = 0; i < sz; i++) {
        QModelIndex ind = currentIMView->model()->index(i, 0);
        const QString name =
            currentIMView->model()->data(ind, Qt::DisplayRole).toString();
        if (name == im_name) {
            currentIMView->setCurrentIndex(ind);
            currentIMView->setFocus();
            return;
        }
    }
    // if not found, search avali IM list
    sz = availIMView->model()->rowCount();
    for (int i = 0; i < sz; i++) {
        QModelIndex ind = availIMView->model()->index(i, 0);
        const QString name =
            availIMView->model()->data(ind, Qt::DisplayRole).toString();
        if (name == im_name) {
            availIMView->setCurrentIndex(ind);
            availIMView->setFocus();
            return;
        }
    }
}

void ConfigMain::addIM() {
    if (availIMView->currentIndex().isValid()) {
        const QString uniqueName =
            availIMView->currentIndex().data(Qt::DisplayRole).toString();
        int largest = 0;
        int find = -1;
        for (size_t i = 0; i < model->schemas_.size(); i++) {
            if (model->schemas_[i].name == uniqueName) {
                find = i;
            }
            if (model->schemas_[i].index > largest) {
                largest = model->schemas_[i].index;
            }
        }
        if (find != -1) {
            model->schemas_[find].active = true;
            model->schemas_[find].index = largest + 1;
        }

        model->sortSchemas();
        updateIMList();
        focusSelectedIM(uniqueName);
        stateChanged();
    }
}

void ConfigMain::removeIM() {
    if (currentIMView->currentIndex().isValid()) {
        const QString uniqueName =
            currentIMView->currentIndex().data(Qt::DisplayRole).toString();
        for (size_t i = 0; i < model->schemas_.size(); i++) {
            if (model->schemas_[i].name == uniqueName) {
                model->schemas_[i].active = false;
                model->schemas_[i].index = 0;
            }
        }
        model->sortSchemas();
        updateIMList();
        focusSelectedIM(uniqueName);
        stateChanged();
    }
}

void ConfigMain::moveUpIM() {
    if (currentIMView->currentIndex().isValid()) {
        const QString uniqueName =
            currentIMView->currentIndex().data(Qt::DisplayRole).toString();
        int cur_index = -1;
        for (size_t i = 0; i < model->schemas_.size(); i++) {
            if (model->schemas_[i].name == uniqueName) {
                cur_index = model->schemas_[i].index;
                Q_ASSERT(cur_index ==
                         (i + 1)); // make sure the schema is sorted
            }
        }
        // can't move up the top schema because the button should be grey
        if (cur_index == -1 || cur_index == 0) {
            return;
        }

        int temp;
        temp = model->schemas_[cur_index - 1].index;
        model->schemas_[cur_index - 1].index =
            model->schemas_[cur_index - 2].index;
        model->schemas_[cur_index - 2].index = temp;
        model->sortSchemas();
        updateIMList();
        focusSelectedIM(uniqueName);
        stateChanged();
    }
}

void ConfigMain::moveDownIM() {
    if (currentIMView->currentIndex().isValid()) {
        const QString uniqueName =
            currentIMView->currentIndex().data(Qt::DisplayRole).toString();
        int cur_index = -1;
        for (size_t i = 0; i < model->schemas_.size(); i++) {
            if (model->schemas_[i].name == uniqueName) {
                cur_index = model->schemas_[i].index;
                Q_ASSERT(cur_index ==
                         (i + 1)); // make sure the schema is sorted
            }
        }
        // can't move down the bottom schema because the button should be grey
        if (cur_index == -1 || cur_index == 0) {
            return;
        }
        int temp;
        temp = model->schemas_[cur_index - 1].index;
        model->schemas_[cur_index - 1].index = model->schemas_[cur_index].index;
        model->schemas_[cur_index].index = temp;
        model->sortSchemas();
        updateIMList();
        focusSelectedIM(uniqueName);
        stateChanged();
    }
}

void ConfigMain::availIMSelectionChanged() {
    if (!availIMView->currentIndex().isValid()) {
        addIMButton->setEnabled(false);
    } else {
        addIMButton->setEnabled(true);
    }
}

void ConfigMain::activeIMSelectionChanged() {
    if (!currentIMView->currentIndex().isValid()) {
        removeIMButton->setEnabled(false);
        moveUpButton->setEnabled(false);
        moveDownButton->setEnabled(false);
        // configureButton->setEnabled(false);
    } else {
        removeIMButton->setEnabled(true);
        // configureButton->setEnabled(true);
        if (currentIMView->currentIndex().row() == 0) {
            moveUpButton->setEnabled(false);
        } else {
            moveUpButton->setEnabled(true);
        }
        if (currentIMView->currentIndex().row() ==
            currentIMView->model()->rowCount() - 1) {
            moveDownButton->setEnabled(false);
        } else {
            moveDownButton->setEnabled(true);
        }
    }
}
// end of SLOTs

QString ConfigMain::icon() { return "fcitx-rime"; }

QString ConfigMain::addon() { return "fcitx-rime"; }

QString ConfigMain::title() { return _("Fcitx Rime Config GUI Tool"); }

void ConfigMain::load() {}

void ConfigMain::setModelFromLayout(QVector<FcitxKeySeq> &model_keys,
                                    QLayout *layout) {
    QList<FcitxQtKeySequenceWidget *> keys = getKeyWidgetsFromLayout(layout);
    model_keys.clear();
    for (int i = 0; i < keys.size(); i++) {
        if (!keys[i]->keySequence().isEmpty()) {
            model_keys.push_back(keys[i]->keySequence());
        }
    }
}

void ConfigMain::uiToModel() {
    model->candidate_per_word = cand_cnt_spinbox->value();
    setModelFromLayout(model->toggle_keys, horizontallayout_toggle);
    setModelFromLayout(model->ascii_key, horizontallayout_ascii);
    setModelFromLayout(model->pgdown_key, horizontallayout_pagedown);
    setModelFromLayout(model->pgup_key, horizontallayout_pageup);
    setModelFromLayout(model->trasim_key, horizontallayout_trasim);
    setModelFromLayout(model->halffull_key, horizontallayout_hfshape);

    // clear cuurent model and save from the ui
    for (int i = 0; i < model->schemas_.size(); i++) {
        model->schemas_[i].index = 0;
        model->schemas_[i].active = false;
    }
    QStandardItemModel *qmodel =
        static_cast<QStandardItemModel *>(currentIMView->model());
    QModelIndex parent;
    int seqno = 1;
    for (int r = 0; r < qmodel->rowCount(parent); ++r) {
        QModelIndex index = qmodel->index(r, 0, parent);
        QVariant name = qmodel->data(index);
        for (int i = 0; i < model->schemas_.size(); i++) {
            if (model->schemas_[i].name == name) {
                model->schemas_[i].index = seqno++;
                model->schemas_[i].active = true;
            }
        }
    }
    model->sortSchemas();
}

void ConfigMain::save() {
    uiToModel();
    QFutureWatcher<void> *futureWatcher = new QFutureWatcher<void>(this);
    futureWatcher->setFuture(
        QtConcurrent::run<void>(this, &ConfigMain::modelToYaml));
    connect(futureWatcher, SIGNAL(finished()), this, SIGNAL(saveFinished()));
}

QList<FcitxQtKeySequenceWidget *>
ConfigMain::getKeyWidgetsFromLayout(QLayout *layout) {
    int count = layout->count();
    QList<FcitxQtKeySequenceWidget *> out;
    for (int i = 0; i < count; i++) {
        FcitxQtKeySequenceWidget *widget =
            qobject_cast<FcitxQtKeySequenceWidget *>(
                layout->itemAt(i)->widget());
        if (widget != NULL) {
            out.push_back(widget);
        }
    }
    return out;
}

void ConfigMain::setKeySeqFromLayout(QLayout *layout,
                                     QVector<FcitxKeySeq> &model_keys) {
    QList<FcitxQtKeySequenceWidget *> keywidgets =
        getKeyWidgetsFromLayout(layout);
    Q_ASSERT(keywidgets.size() >= model_keys.size());
    for (int i = 0; i < model_keys.size(); i++) {
        keywidgets[i]->setKeySequence(
            QKeySequence(FcitxQtKeySequenceWidget::keyFcitxToQt(
                model_keys[i].sym_, model_keys[i].states_)));
    }
    return;
}

void ConfigMain::modelToUi() {
    cand_cnt_spinbox->setValue(model->candidate_per_word);
    // set shortcut keys
    setKeySeqFromLayout(horizontallayout_toggle, model->toggle_keys);
    setKeySeqFromLayout(horizontallayout_pagedown, model->pgdown_key);
    setKeySeqFromLayout(horizontallayout_pageup, model->pgup_key);
    setKeySeqFromLayout(horizontallayout_ascii, model->ascii_key);
    setKeySeqFromLayout(horizontallayout_trasim, model->trasim_key);
    setKeySeqFromLayout(horizontallayout_hfshape, model->halffull_key);

    // set available and enabled input methods
    for (size_t i = 0; i < model->schemas_.size(); i++) {
        auto &schema = model->schemas_[i];
        if (schema.active) {
            QStandardItem *active_schema = new QStandardItem(schema.name);
            active_schema->setEditable(false);
            auto qmodel =
                static_cast<QStandardItemModel *>(currentIMView->model());
            qmodel->appendRow(active_schema);
        } else {
            QStandardItem *inactive_schema = new QStandardItem(schema.name);
            inactive_schema->setEditable(false);
            auto qmodel =
                static_cast<QStandardItemModel *>(availIMView->model());
            qmodel->appendRow(inactive_schema);
        }
    }
}

void ConfigMain::updateIMList() {
    auto avail_IMmodel =
        static_cast<QStandardItemModel *>(availIMView->model());
    auto active_IMmodel =
        static_cast<QStandardItemModel *>(currentIMView->model());
    avail_IMmodel->removeRows(0, avail_IMmodel->rowCount());
    active_IMmodel->removeRows(0, active_IMmodel->rowCount());
    for (size_t i = 0; i < model->schemas_.size(); i++) {
        auto &schema = model->schemas_[i];
        if (schema.active) {
            QStandardItem *active_schema = new QStandardItem(schema.name);
            active_schema->setEditable(false);
            active_IMmodel->appendRow(active_schema);
        } else {
            QStandardItem *inactive_schema = new QStandardItem(schema.name);
            inactive_schema->setEditable(false);
            avail_IMmodel->appendRow(inactive_schema);
        }
    }
}

// type: toggle 0, send 1
void ConfigMain::setModelKeysToYaml(QVector<FcitxKeySeq> &model_keys, int type,
                                    const char *key) {
    char **keys =
        (char **)fcitx_utils_malloc0(sizeof(char *) * model_keys.size());
    for (int i = 0; i < model_keys.size(); i++) {
        std::string s = model_keys[i].toString();
        keys[i] = (char *)fcitx_utils_malloc0(s.length() + 1);
        memcpy(keys[i], s.c_str(), s.length());
    }
    FcitxRimeConfigSetKeyBindingSet(rime->default_conf, type, key,
                                    (const char **)keys, model_keys.size());
    for (int i = 0; i < model_keys.size(); i++) {
        fcitx_utils_free(keys[i]);
    }
    fcitx_utils_free(keys);
}

void ConfigMain::modelToYaml() {
    rime->api->config_set_int(rime->default_conf, "menu/page_size",
                              model->candidate_per_word);
    char **tkptrs = (char **)fcitx_utils_malloc0(model->toggle_keys.size());
    for (size_t i = 0; i < model->toggle_keys.size(); i++) {
        std::string s = model->toggle_keys[i].toString();
        tkptrs[i] = (char *)fcitx_utils_malloc0(s.length() + 1);
        memset(tkptrs[i], 0, s.length() + 1);
        memcpy(tkptrs[i], s.c_str(), s.length());
    }
    FcitxRimeConfigSetToggleKeys(rime, rime->default_conf,
                                 (const char **)tkptrs,
                                 model->toggle_keys.size());
    for (size_t i = 0; i < model->toggle_keys.size(); i++) {
        fcitx_utils_free(tkptrs[i]);
    }
    fcitx_utils_free(tkptrs);

    setModelKeysToYaml(model->ascii_key, 0, "ascii_mode");
    setModelKeysToYaml(model->trasim_key, 0, "simplification");
    setModelKeysToYaml(model->halffull_key, 0, "full_shape");
    setModelKeysToYaml(model->pgup_key, 1, "Page_Up");
    setModelKeysToYaml(model->pgdown_key, 1, "Page_Down");

    // set active schema list
    int active = 0;
    char **schema_names =
        (char **)fcitx_utils_malloc0(sizeof(char *) * model->schemas_.size());
    for (int i = 0; i < model->schemas_.size(); i++) {
        if (model->schemas_[i].index == 0) {
            break;
        } else {
            std::string schema_id = model->schemas_[i].id.toStdString();
            size_t len = schema_id.length();
            schema_names[active] = (char *)fcitx_utils_malloc0(len + 1);
            memcpy(schema_names[active], schema_id.c_str(), len);
            active += 1;
        }
    }
    FcitxRimeClearAndSetSchemaList(rime, rime->default_conf, schema_names,
                                   active);
    for (int i = 0; i < active; i++) {
        fcitx_utils_free(schema_names[i]);
    }
    fcitx_utils_free(schema_names);

    FcitxRimeConfigSync(rime);
    return;
}

void ConfigMain::yamlToModel() {
    FcitxRimeConfigOpenDefault(rime);
    // load page size
    int page_size = 0;
    bool suc = rime->api->config_get_int(rime->default_conf, "menu/page_size",
                                         &page_size);
    if (suc) {
        model->candidate_per_word = page_size;
    } else {
        model->candidate_per_word = default_page_size;
    }
    // toggle keys
    size_t keys_size =
        FcitxRimeConfigGetToggleKeySize(rime, rime->default_conf);
    keys_size = keys_size > max_shortcuts ? max_shortcuts : keys_size;
    char **keys = (char **)fcitx_utils_malloc0(sizeof(char *) * keys_size);
    FcitxRimeConfigGetToggleKeys(rime, rime->default_conf, keys, keys_size);
    for (size_t i = 0; i < keys_size; i++) {
        if (strlen(keys[i]) != 0) { // skip the empty keys
            model->toggle_keys.push_back(FcitxKeySeq(keys[i]));
        }
        fcitx_utils_free(keys[i]);
    }
    fcitx_utils_free(keys);
    // load other shortcuts
    size_t buffer_size = 30;
    char *accept = (char *)fcitx_utils_malloc0(buffer_size);
    char *act_key = (char *)fcitx_utils_malloc0(buffer_size);
    char *act_type = (char *)fcitx_utils_malloc0(buffer_size);
    FcitxRimeBeginKeyBinding(rime->default_conf);
    size_t toggle_length = FcitxRimeConfigGetKeyBindingSize(rime->default_conf);
    for (size_t i = 0; i < toggle_length; i++) {
        memset(accept, 0, buffer_size);
        memset(act_key, 0, buffer_size);
        memset(act_type, 0, buffer_size);
        FcitxRimeConfigGetNextKeyBinding(rime->default_conf, act_type, act_key,
                                         accept, buffer_size);

        if (strlen(accept) != 0) {
            if (strcmp(act_key, "ascii_mode") == 0) {
                FcitxKeySeq seq = FcitxKeySeq(accept);
                model->ascii_key.push_back(seq);
            } else if (strcmp(act_key, "full_shape") == 0) {
                FcitxKeySeq seq = FcitxKeySeq(accept);
                model->halffull_key.push_back(seq);
            } else if (strcmp(act_key, "simplification") == 0) {
                FcitxKeySeq seq = FcitxKeySeq(accept);
                model->trasim_key.push_back(seq);
            } else if (strcmp(act_key, "Page_Up") == 0) {
                FcitxKeySeq seq = FcitxKeySeq(accept);
                model->pgup_key.push_back(seq);
            } else if (strcmp(act_key, "Page_Down") == 0) {
                FcitxKeySeq seq = FcitxKeySeq(accept);
                model->pgdown_key.push_back(seq);
            }
        }
    }
    fcitx_utils_free(accept);
    fcitx_utils_free(act_key);
    fcitx_utils_free(act_type);
    FcitxRimeEndKeyBinding(rime->default_conf);
    model->sortKeys();
    getAvailableSchemas();
}

void ConfigMain::getAvailableSchemas() {
    const char *absolute_path = RimeGetUserDataDir();
    FcitxStringHashSet *files =
        FcitxXDGGetFiles(fcitx_rime_dir_prefix, NULL, fcitx_rime_schema_suffix);
    HASH_SORT(files, fcitx_utils_string_hash_set_compare);
    HASH_FOREACH(f, files, FcitxStringHashSet) {
        auto schema = FcitxRimeSchema();
        schema.path = QString::fromLocal8Bit(f->name).prepend(absolute_path);
        auto basefilename = QString::fromLocal8Bit(f->name).section(".", 0, 0);
        size_t buffer_size = 50;
        char *name = static_cast<char *>(fcitx_utils_malloc0(buffer_size));
        char *id = static_cast<char *>(fcitx_utils_malloc0(buffer_size));
        FcitxRimeGetSchemaAttr(rime, basefilename.toStdString().c_str(), name,
                               buffer_size, "schema/name");
        FcitxRimeGetSchemaAttr(rime, basefilename.toStdString().c_str(), id,
                               buffer_size, "schema/schema_id");
        schema.name = QString::fromLocal8Bit(name);
        schema.id = QString::fromLocal8Bit(id);
        schema.index =
            FcitxRimeCheckSchemaEnabled(rime, rime->default_conf, id);
        schema.active = (bool)schema.index;
        fcitx_utils_free(name);
        fcitx_utils_free(id);
        model->schemas_.push_back(schema);
    }
    fcitx_utils_free_string_hash_set(files);
    model->sortSchemas();
}

}; // namespace fcitx_rime
