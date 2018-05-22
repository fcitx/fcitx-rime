#include <fcitx-config/xdg.h>

#include <QtConcurrentRun>
#include <QFutureWatcher>
#include <QDialogButtonBox>
#include <QTreeWidgetItem>

#include "ConfigMain.h"
#include "Common.h"
#include "ui_ConfigMain.h"
#include <QListWidgetItem>
#include <QStandardItemModel>

// TODO: when failed-read happens, disable ui
// TODO: when failed-save happens, disable ui and show reason

namespace fcitx_rime {
  ConfigMain::ConfigMain(QWidget* parent) :
    FcitxQtConfigUIWidget(parent), m_ui(new Ui::MainUI),
    model(new FcitxRimeConfigDataModel()) {
    // Setup UI
    setMinimumSize(680, 500);
    m_ui->setupUi(this);
    m_ui->verticallayout_general->setAlignment(Qt::AlignTop);
    // m_ui->filterTextEdit->setPlaceholderText("Search Input Method");
     m_ui->addIMButton->setIcon(QIcon::fromTheme("go-next"));
    m_ui->removeIMButton->setIcon(QIcon::fromTheme("go-previous"));
    m_ui->moveUpButton->setIcon(QIcon::fromTheme("go-up"));
    m_ui->moveDownButton->setIcon(QIcon::fromTheme("go-down"));
    // m_ui->configureButton->setIcon(QIcon::fromTheme("help-about"));
    // listViews for currentIM and availIM
    QStandardItemModel* listModel = new QStandardItemModel();
    m_ui->currentIMView->setModel(listModel);
    QStandardItemModel* availIMModel = new QStandardItemModel();
    m_ui->availIMView->setModel(availIMModel);
    // tab shortcut 
    connect(m_ui->cand_cnt_spinbox, SIGNAL(valueChanged(int)), 
            this, SLOT(stateChanged()));
    QList<FcitxQtKeySequenceWidget*> keywgts =
      m_ui->general_tab->findChildren<FcitxQtKeySequenceWidget*>();
    for(size_t i = 0; i < keywgts.size(); i ++) {
      connect(keywgts[i], SIGNAL(keySequenceChanged(QKeySequence, FcitxQtModifierSide)),
	      this, SLOT(keytoggleChanged()));
    }
    // tab schemas
    connect(m_ui->removeIMButton, SIGNAL(clicked(bool)), this, SLOT(removeIM()));
    connect(m_ui->addIMButton, SIGNAL(clicked(bool)), this, SLOT(addIM()));
    connect(m_ui->moveUpButton, SIGNAL(clicked(bool)), this, SLOT(moveUpIM()));
    connect(m_ui->moveDownButton, SIGNAL(clicked(bool)), this, SLOT(moveDownIM()));
    connect(m_ui->availIMView->selectionModel(), SIGNAL(currentChanged(QModelIndex, QModelIndex)), this, SLOT(availIMSelectionChanged()));
    connect(m_ui->currentIMView->selectionModel(), SIGNAL(currentChanged(QModelIndex, QModelIndex)), this, SLOT(activeIMSelectionChanged()));
    rime = FcitxRimeConfigCreate();
    FcitxRimeConfigStart(rime);
    yamlToModel();
    modelToUi();
  }
  
  ConfigMain::~ConfigMain() {
    FcitxRimeDestroy(rime);
    delete model;
    delete m_ui;
  }
  
  void ConfigMain::keytoggleChanged() {
    stateChanged();
  }
  
  // SLOTs
  void ConfigMain::stateChanged() {
    emit changed(true);
  }
  
  void ConfigMain::focusSelectedIM(const QString im_name) {
    // search enabled IM first
    int sz = m_ui->currentIMView->model()->rowCount();
    for(int i = 0; i < sz; i ++) {
      QModelIndex ind = m_ui->currentIMView->model()->index(i, 0);
      const QString name = m_ui->currentIMView->model()->data(ind, Qt::DisplayRole).toString();
      if(name == im_name) {
	m_ui->currentIMView->setCurrentIndex(ind);
	m_ui->currentIMView->setFocus();
	return;
      }
    }
    // if not found, search avali IM list
    sz = m_ui->availIMView->model()->rowCount();
    for(int i = 0; i < sz; i ++) {
      QModelIndex ind = m_ui->availIMView->model()->index(i, 0);
      const QString name = m_ui->availIMView->model()->data(ind, Qt::DisplayRole).toString();
      if(name == im_name) {
	m_ui->availIMView->setCurrentIndex(ind);
	m_ui->availIMView->setFocus();
	return;
      }
    }
  }
  
  void ConfigMain::addIM() {
    if(m_ui->availIMView->currentIndex().isValid()) {
      const QString uniqueName =m_ui->availIMView->currentIndex().data(Qt::DisplayRole).toString();
      int largest = 0;
      int find = -1;
      for(size_t i = 0; i < model->schemas_.size(); i ++) {
        if(model->schemas_[i].name == uniqueName) {
	  find = i;
	}
	if(model->schemas_[i].index > largest) {
	  largest = model->schemas_[i].index;
	}
      }
      if(find != -1) {
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
    if(m_ui->currentIMView->currentIndex().isValid()) {
      const QString uniqueName = m_ui->currentIMView->currentIndex().data(Qt::DisplayRole).toString();
      for(size_t i = 0; i < model->schemas_.size(); i ++) {
        if(model->schemas_[i].name == uniqueName) {
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
    if(m_ui->currentIMView->currentIndex().isValid()) {
      const QString uniqueName =m_ui->currentIMView->currentIndex().data(Qt::DisplayRole).toString();
      int cur_index = -1;
      for(size_t i = 0; i < model->schemas_.size(); i ++) {
        if(model->schemas_[i].name == uniqueName) {
	  cur_index = model->schemas_[i].index;
	  Q_ASSERT(cur_index == (i + 1)); // make sure the schema is sorted
	}
      }
      // can't move up the top schema because the button should be grey
      if(cur_index == -1 || cur_index == 0) {
	return;
      }

      int temp;
      temp = model->schemas_[cur_index - 1].index;
      model->schemas_[cur_index - 1].index = model->schemas_[cur_index - 2].index;
      model->schemas_[cur_index - 2].index = temp;
      model->sortSchemas();
      updateIMList();
      focusSelectedIM(uniqueName);
      stateChanged();
    }
  }
  
  void ConfigMain::moveDownIM() {
    if(m_ui->currentIMView->currentIndex().isValid()) {
      const QString uniqueName =m_ui->currentIMView->currentIndex().data(Qt::DisplayRole).toString();
      int cur_index = -1;
      for(size_t i = 0; i < model->schemas_.size(); i ++) {
        if(model->schemas_[i].name == uniqueName) {
	  cur_index = model->schemas_[i].index;
	  Q_ASSERT(cur_index == (i + 1)); // make sure the schema is sorted
	}
      }
      // can't move down the bottom schema because the button should be grey
      if(cur_index == -1 || cur_index == 0) {
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
    if(!m_ui->availIMView->currentIndex().isValid()) {
      m_ui->addIMButton->setEnabled(false);
    } else {
      m_ui->addIMButton->setEnabled(true);
    }
  }
  
  void ConfigMain::activeIMSelectionChanged() {
    if(!m_ui->currentIMView->currentIndex().isValid()) {
      m_ui->removeIMButton->setEnabled(false);
      m_ui->moveUpButton->setEnabled(false);
      m_ui->moveDownButton->setEnabled(false);
      // m_ui->configureButton->setEnabled(false);
    } else {
      m_ui->removeIMButton->setEnabled(true);
      // m_ui->configureButton->setEnabled(true);
      if(m_ui->currentIMView->currentIndex().row() == 0) {
        m_ui->moveUpButton->setEnabled(false);
      } else {
        m_ui->moveUpButton->setEnabled(true);
      }
      if(m_ui->currentIMView->currentIndex().row() == 
        m_ui->currentIMView->model()->rowCount() - 1)
      {
        m_ui->moveDownButton->setEnabled(false);
      } else {
        m_ui->moveDownButton->setEnabled(true);
      }
    }
  }
  // end of SLOTs
  
  QString ConfigMain::icon() {
    return "fcitx-rime";
  }
  
  QString ConfigMain::addon() {
    return "fcitx-rime";
  }
  
  QString ConfigMain::title() {
    return _("Fcitx Rime Config GUI Tool");
  }
  
  void ConfigMain::load() {
  }

  void ConfigMain::setModelFromLayout(QVector<FcitxKeySeq>& model_keys, QLayout *layout) {
    QList<FcitxQtKeySequenceWidget*> keys = getKeyWidgetsFromLayout(layout);
    model_keys.clear();
    for(int i = 0; i < keys.size(); i ++) {
      if(!keys[i]->keySequence().isEmpty()) {
	model_keys.push_back(keys[i]->keySequence());
      }
    }
  }
  
  void ConfigMain::uiToModel() {
    model->candidate_per_word = m_ui->cand_cnt_spinbox->value();
    setModelFromLayout(model->toggle_keys, m_ui->horizontallayout_toggle);
    setModelFromLayout(model->ascii_key, m_ui->horizontallayout_ascii);
    setModelFromLayout(model->pgdown_key, m_ui->horizontallayout_pagedown);
    setModelFromLayout(model->pgup_key, m_ui->horizontallayout_pageup);
    setModelFromLayout(model->trasim_key, m_ui->horizontallayout_trasim);
    setModelFromLayout(model->halffull_key, m_ui->horizontallayout_hfshape);
    
    // clear cuurent model and save from the ui
    for(int i = 0; i < model->schemas_.size(); i ++) {
      model->schemas_[i].index = 0;
      model->schemas_[i].active = false;
    }
    QStandardItemModel* qmodel = static_cast<QStandardItemModel*>(m_ui->currentIMView->model());
    QModelIndex parent;
    int seqno = 1;
    for(int r = 0; r < qmodel->rowCount(parent); ++r) {
      QModelIndex index = qmodel->index(r, 0, parent);
      QVariant name = qmodel->data(index);
      for(int i = 0; i < model->schemas_.size(); i ++) {
	if(model->schemas_[i].name == name) {
	  model->schemas_[i].index = seqno ++;
	  model->schemas_[i].active = true;
	}
      }
    }
    model->sortSchemas();
  }
  
  void ConfigMain::save() {
    uiToModel();
    QFutureWatcher<void>* futureWatcher = new QFutureWatcher<void>(this);
    futureWatcher->setFuture(QtConcurrent::run<void>(this, &ConfigMain::modelToYaml));
    connect(futureWatcher, SIGNAL(finished()), this, SIGNAL(saveFinished()));
  }

  QList<FcitxQtKeySequenceWidget*> ConfigMain::getKeyWidgetsFromLayout(QLayout *layout) {
    int count = layout->count();
    QList<FcitxQtKeySequenceWidget*> out;
    for(int i = 0; i < count; i ++) {
      FcitxQtKeySequenceWidget* widget =
	qobject_cast<FcitxQtKeySequenceWidget*>(layout->itemAt(i)->widget());
      if(widget != NULL) {
	out.push_back(widget);
      }
    }
    return out;
  }

  void ConfigMain::setKeySeqFromLayout(QLayout *layout, QVector<FcitxKeySeq>& model_keys) {
    QList<FcitxQtKeySequenceWidget*> keywidgets = getKeyWidgetsFromLayout(layout);
    Q_ASSERT(keywidgets.size() >= model_keys.size());
    for(int i = 0; i < model_keys.size(); i ++) {
      keywidgets[i]->setKeySequence(QKeySequence(FcitxQtKeySequenceWidget::keyFcitxToQt(model_keys[i].sym_, model_keys[i].states_)));
    }
    return;
  }
  
  void ConfigMain::modelToUi() {
    m_ui->cand_cnt_spinbox->setValue(model->candidate_per_word);
    // set shortcut keys
    setKeySeqFromLayout(m_ui->horizontallayout_toggle, model->toggle_keys);
    setKeySeqFromLayout(m_ui->horizontallayout_pagedown, model->pgdown_key);
    setKeySeqFromLayout(m_ui->horizontallayout_pageup, model->pgup_key);
    setKeySeqFromLayout(m_ui->horizontallayout_ascii, model->ascii_key);
    setKeySeqFromLayout(m_ui->horizontallayout_trasim, model->trasim_key);
    setKeySeqFromLayout(m_ui->horizontallayout_hfshape, model->halffull_key);
    
    // set available and enabled input methods
    for(size_t i = 0; i < model->schemas_.size(); i ++) {
      auto& schema = model->schemas_[i];
      if(schema.active) {
        QStandardItem* active_schema = new QStandardItem(schema.name);
	active_schema->setEditable(false);
        auto qmodel = static_cast<QStandardItemModel*>(m_ui->currentIMView->model());
        qmodel->appendRow(active_schema);
      } else {
        QStandardItem* inactive_schema = new QStandardItem(schema.name);
	inactive_schema->setEditable(false);
        auto qmodel = static_cast<QStandardItemModel*>(m_ui->availIMView->model());
        qmodel->appendRow(inactive_schema);
      }
    }
  }

  void ConfigMain::updateIMList() {
    auto avail_IMmodel = static_cast<QStandardItemModel*>(m_ui->availIMView->model());
    auto active_IMmodel = static_cast<QStandardItemModel*>(m_ui->currentIMView->model());
    avail_IMmodel->removeRows(0, avail_IMmodel->rowCount());
    active_IMmodel->removeRows(0, active_IMmodel->rowCount());
    for(size_t i = 0; i < model->schemas_.size(); i ++) {
      auto& schema = model->schemas_[i];
      if(schema.active) {
        QStandardItem* active_schema = new QStandardItem(schema.name);
	active_schema->setEditable(false);
        active_IMmodel->appendRow(active_schema);
      } else {
        QStandardItem* inactive_schema = new QStandardItem(schema.name);
	inactive_schema->setEditable(false);
        avail_IMmodel->appendRow(inactive_schema);
      }
    }
  }

  // type: toggle 0, send 1
  void ConfigMain::setModelKeysToYaml(QVector<FcitxKeySeq>& model_keys,
				      int type, const char* key) {
    char ** keys = (char **) fcitx_utils_malloc0(sizeof(char*) * model_keys.size());
    for(int i = 0; i < model_keys.size(); i ++) {
      std::string s = model_keys[i].toString();
      keys[i] = (char*) fcitx_utils_malloc0(s.length() + 1);
      memcpy(keys[i], s.c_str(), s.length());
    }
    FcitxRimeConfigSetKeyBindingSet(rime->default_conf, type, key,
      (const char **)keys, model_keys.size());
    for(int i = 0; i < model_keys.size(); i ++) {
      fcitx_utils_free(keys[i]);
    }
    fcitx_utils_free(keys);
  }
  
  void ConfigMain::modelToYaml() {
    rime->api->config_set_int(rime->default_conf,
			      "menu/page_size", model->candidate_per_word);
    char * * tkptrs = (char * * ) fcitx_utils_malloc0(model->toggle_keys.size());
    for(size_t i = 0; i < model->toggle_keys.size(); i ++) {
      std::string s = model->toggle_keys[i].toString();
      tkptrs[i] = (char*)fcitx_utils_malloc0(s.length() + 1);
      memset(tkptrs[i], 0, s.length() + 1);
      memcpy(tkptrs[i], s.c_str(), s.length());
    }
    FcitxRimeConfigSetToggleKeys(rime, rime->default_conf, (const char **)tkptrs,
				 model->toggle_keys.size());
    for(size_t i = 0; i < model->toggle_keys.size(); i ++) {
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
    char** schema_names = (char**)fcitx_utils_malloc0(sizeof(char*) * model->schemas_.size());
    for(int i = 0; i < model->schemas_.size(); i ++) {
      if(model->schemas_[i].index == 0) {
	break;
      } else {
	std::string schema_id = model->schemas_[i].id.toStdString();
	size_t len = schema_id.length();
	schema_names[active] = (char*)fcitx_utils_malloc0(len + 1);
	memcpy(schema_names[active], schema_id.c_str(), len);
	active += 1;
      }
    }
    FcitxRimeClearAndSetSchemaList(rime, rime->default_conf, schema_names, active);
    for(int i = 0; i < active; i ++) {
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
    bool suc = rime->api->config_get_int(rime->default_conf, "menu/page_size", &page_size);
    if(suc) {
      model->candidate_per_word = page_size;
    } else {
      model->candidate_per_word = DEFAULT_PAGE_SIZE;
    }
    // toggle keys
    size_t keys_size = FcitxRimeConfigGetToggleKeySize(rime, rime->default_conf);
    keys_size = keys_size > MAX_SHORTCUTS ? MAX_SHORTCUTS : keys_size;
    char** keys = (char**)fcitx_utils_malloc0(sizeof(char*) * keys_size);
    FcitxRimeConfigGetToggleKeys(rime, rime->default_conf, keys, keys_size);
    for(size_t i = 0; i < keys_size; i ++) {
      if(strlen(keys[i]) != 0) { // skip the empty keys
	model->toggle_keys.push_back(FcitxKeySeq(keys[i]));
      }
      fcitx_utils_free(keys[i]);
    }
    fcitx_utils_free(keys);
    // load other shortcuts
    FcitxRimeBeginKeyBinding(rime->default_conf);
    size_t buffer_size = 30;
    char* accept = (char*)fcitx_utils_malloc0(buffer_size);
    char* act_key = (char*)fcitx_utils_malloc0(buffer_size);
    char* act_type = (char*)fcitx_utils_malloc0(buffer_size);
    FcitxRimeBeginKeyBinding(rime->default_conf);
    size_t toggle_length = FcitxRimeConfigGetKeyBindingSize(rime->default_conf);
    for(size_t i = 0; i < toggle_length; i ++) {
      memset(accept, 0, buffer_size);
      memset(act_key, 0, buffer_size);
      memset(act_type, 0, buffer_size);
      FcitxRimeConfigGetNextKeyBinding(rime->default_conf, act_type, act_key, accept, buffer_size);

      if(strlen(accept) != 0) {
	if(strcmp(act_key, "ascii_mode") == 0) {
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
    // load available schemas
    getAvailableSchemas();
  }
  
  void ConfigMain::getAvailableSchemas() {
    const char* absolute_path = RimeGetUserDataDir();
    FcitxStringHashSet* files = FcitxXDGGetFiles(FCITX_RIME_DIR_PREFIX, NULL, FCITX_RIME_SCHEMA_SUFFIX);
    HASH_SORT(files, fcitx_utils_string_hash_set_compare);
    HASH_FOREACH(f, files, FcitxStringHashSet) {
      auto schema = FcitxRimeSchema();
      schema.path = QString::fromLocal8Bit(f->name).prepend(absolute_path);
      auto basefilename = QString::fromLocal8Bit(f->name).section(".",0,0);
      size_t buffer_size = 50;
      char* name = static_cast<char*>(fcitx_utils_malloc0(buffer_size));
      char* id = static_cast<char*>(fcitx_utils_malloc0(buffer_size));
      FcitxRimeGetSchemaAttr(rime, basefilename.toStdString().c_str(), name, buffer_size, "schema/name");
      FcitxRimeGetSchemaAttr(rime, basefilename.toStdString().c_str(), id, buffer_size, "schema/schema_id");
      schema.name = QString::fromLocal8Bit(name);
      schema.id = QString::fromLocal8Bit(id);
      schema.index = FcitxRimeCheckSchemaEnabled(rime, rime->default_conf, id);
      schema.active = (bool)schema.index;
      fcitx_utils_free(name);
      fcitx_utils_free(id);
      model->schemas_.push_back(schema);
    }
    fcitx_utils_free_string_hash_set(files);
    model->sortSchemas();
  }

};
