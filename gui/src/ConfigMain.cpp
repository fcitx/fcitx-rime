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

#include "Common.h"
#include "ConfigMain.h"
#include <QDialogButtonBox>
#include <QDir>
#include <QFutureWatcher>
#include <QListWidgetItem>
#include <QStandardItemModel>
#include <QTreeWidgetItem>
#include <QtConcurrentRun>
#include <QtGlobal>

// TODO: when failed-read happens, disable ui
// TODO: when failed-save happens, disable ui and show reason

namespace fcitx_rime {
ConfigMain::ConfigMain(QWidget *parent)
    : FcitxQtConfigUIWidget(parent), model(new RimeConfigDataModel()) {
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
    QStandardItemModel *listModel = new QStandardItemModel(this);
    currentIMView->setModel(listModel);
    QStandardItemModel *availIMModel = new QStandardItemModel(this);
    availIMView->setModel(availIMModel);
    // tab shortcut
    connect(cand_cnt_spinbox, QOverload<int>::of(&QSpinBox::valueChanged), this,
            &ConfigMain::stateChanged);
    QList<FcitxQtKeySequenceWidget *> keywgts =
        general_tab->findChildren<FcitxQtKeySequenceWidget *>();
    for (size_t i = 0; i < keywgts.size(); i++) {
        connect(keywgts[i], &FcitxQtKeySequenceWidget::keySequenceChanged, this,
                &ConfigMain::keytoggleChanged);
    }
    // tab schemas
    connect(removeIMButton, &QPushButton::clicked, this, &ConfigMain::removeIM);
    connect(addIMButton, &QPushButton::clicked, this, &ConfigMain::addIM);
    connect(moveUpButton, &QPushButton::clicked, this, &ConfigMain::moveUpIM);
    connect(moveDownButton, &QPushButton::clicked, this,
            &ConfigMain::moveDownIM);
    connect(availIMView->selectionModel(), &QItemSelectionModel::currentChanged,
            this, &ConfigMain::availIMSelectionChanged);
    connect(currentIMView->selectionModel(),
            &QItemSelectionModel::currentChanged, this,
            &ConfigMain::activeIMSelectionChanged);
    yamlToModel();
    modelToUi();
}

ConfigMain::~ConfigMain() { delete model; }

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
    connect(futureWatcher, &QFutureWatcher<void>::finished, this, [this]() {
        emit changed(false);
        emit saveFinished();
    });
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

void ConfigMain::modelToYaml() {
    config.setInteger("menu/page_size", model->candidate_per_word);
    std::vector<std::string> toggleKeys;
    for (size_t i = 0; i < model->toggle_keys.size(); i++) {
        toggleKeys.push_back(model->toggle_keys[i].toString());
    }

    config.setToggleKeys(toggleKeys);

    // FIXME: implement new ui for key bindings.
    // setModelKeysToYaml(model->ascii_key, 0, "ascii_mode");
    // setModelKeysToYaml(model->trasim_key, 0, "simplification");
    // setModelKeysToYaml(model->halffull_key, 0, "full_shape");
    // setModelKeysToYaml(model->pgup_key, 1, "Page_Up");
    // setModelKeysToYaml(model->pgdown_key, 1, "Page_Down");

    // set active schema list
    std::vector<std::string> schemaNames;
    schemaNames.reserve(model->schemas_.size());
    for (int i = 0; i < model->schemas_.size(); i++) {
        if (model->schemas_[i].index == 0) {
            break;
        } else {
            schemaNames.push_back(model->schemas_[i].id.toStdString());
        }
    }
    config.setSchemas(schemaNames);

    config.sync();
    return;
}

void ConfigMain::yamlToModel() {
    // load page size
    int page_size = 0;
    bool suc = config.readInteger("menu/page_size", &page_size);
    if (suc) {
        model->candidate_per_word = page_size;
    } else {
        model->candidate_per_word = default_page_size;
    }
    // toggle keys
    auto toggleKeys = config.toggleKeys();
    for (const auto &toggleKey : toggleKeys) {
        if (!toggleKey.empty()) { // skip the empty keys
            model->toggle_keys.push_back(FcitxKeySeq(toggleKey.data()));
        }
    }
    // load other shortcuts

    auto bindings = config.keybindings();
    for (const auto &binding : bindings) {
        if (binding.accept.empty()) {
            continue;
        }
        if (binding.action == "ascii_mode") {
            FcitxKeySeq seq(binding.accept);
            model->ascii_key.push_back(seq);
        } else if (binding.action == "full_shape") {
            FcitxKeySeq seq(binding.accept);
            model->halffull_key.push_back(seq);
        } else if (binding.action == "simplification") {
            FcitxKeySeq seq(binding.accept);
            model->trasim_key.push_back(seq);
        } else if (binding.action == "Page_Up") {
            FcitxKeySeq seq(binding.accept);
            model->pgup_key.push_back(seq);
        } else if (binding.action == "Page_Down") {
            FcitxKeySeq seq(binding.accept);
            model->pgdown_key.push_back(seq);
        }
    }
    model->sortKeys();
    getAvailableSchemas();
}

void ConfigMain::getAvailableSchemas() {
    const char *userPath = RimeGetUserDataDir();
    const char *sysPath = RimeGetSharedDataDir();

    QSet<QString> files;
    for (auto path : {sysPath, userPath}) {
        if (!path) {
            continue;
        }
        QDir dir(path);
        files.unite(QSet<QString>::fromList(dir.entryList(
            QStringList("*.schema.yaml"), QDir::Files | QDir::Readable)));
    }

    auto filesList = files.toList();
    filesList.sort();

    for (const auto &file : filesList) {
        auto schema = FcitxRimeSchema();
        QString fullPath;
        for (auto path : {userPath, sysPath}) {
            QDir dir(path);
            if (dir.exists(file)) {
                fullPath = dir.filePath(file);
                break;
            }
        }
        schema.path = fullPath;
        QFile fd(fullPath);
        if (!fd.open(QIODevice::ReadOnly)) {
            continue;
        }
        auto yamlData = fd.readAll();
        auto name = config.stringFromYAML(yamlData.constData(), "schema/name");
        auto id =
            config.stringFromYAML(yamlData.constData(), "schema/schema_id");
        schema.name = QString::fromStdString(name);
        schema.id = QString::fromStdString(id);
        schema.index = config.schemaIndex(id.data());
        schema.active = static_cast<bool>(schema.index);
        model->schemas_.push_back(schema);
    }
    model->sortSchemas();
}

}; // namespace fcitx_rime
