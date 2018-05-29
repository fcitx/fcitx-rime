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
#include <fcitx-config/xdg.h>
#include <fcitx-utils/utils.h>
#include "FcitxRimeConfig.h"

static RimeConfigIterator* global_iterator;

FcitxRime* FcitxRimeConfigCreate() {
  FcitxRime* rime = (FcitxRime*) fcitx_utils_malloc0(sizeof(FcitxRime));
  rime->api = rime_get_api();
  rime->firstRun = True;
  rime->default_conf = NULL;
  if (!rime->api) {
    free(rime);
    return NULL;
  }
  return rime;
}

void FcitxRimeConfigStart(FcitxRime* rime) {
  char* user_path = NULL;
  FILE* fp = FcitxXDGGetFileUserWithPrefix(fcitx_rime_dir_prefix, ".place_holder", "w", NULL);
  if (fp) {
    fclose(fp);
  }
  FcitxXDGGetFileUserWithPrefix(fcitx_rime_dir_prefix, "", NULL, &user_path);
  const char* shared_data_dir = fcitx_utils_get_fcitx_path_with_filename("pkgdatadir", fcitx_rime_dir_prefix);
  RIME_STRUCT(RimeTraits, fcitx_rime_traits);
  fcitx_rime_traits.shared_data_dir = shared_data_dir;
  fcitx_rime_traits.app_name = "rime.fcitx-rime-config";
  fcitx_rime_traits.user_data_dir = user_path;
  fcitx_rime_traits.distribution_name = "Rime";
  fcitx_rime_traits.distribution_code_name = "fcitx-rime-config";
  fcitx_rime_traits.distribution_version = "0.0.2";
  if(rime->firstRun) {
    rime->api->setup(&fcitx_rime_traits);
    rime->firstRun = False;
  }
  rime->api->initialize(&fcitx_rime_traits);
}

RimeConfig* FcitxRimeConfigOpenDefault(FcitxRime* rime) {
  RimeConfig* fcitx_rime_config_default = (RimeConfig*) fcitx_utils_malloc0(sizeof(RimeConfig));
  Bool suc = rime->api->config_open("default", fcitx_rime_config_default);
  if (!suc) {
    return NULL;
  }
  rime->default_conf = fcitx_rime_config_default;
  return fcitx_rime_config_default;
}
  
void FcitxRimeConfigSetToggleKeys(FcitxRime* rime, RimeConfig* config, const char** keys,
				  size_t keys_size) {
  RimeConfigClear(config, "switcher/hotkeys");
  RimeConfigCreateList(config, "switcher/hotkeys");
  RimeConfigIterator iterator;
  RimeConfigBeginList(&iterator, config, "switcher/hotkeys");
  RimeConfigNext(&iterator);
  for(size_t i = 0; i < keys_size; i ++) {
    RimeConfigNext(&iterator);
    RimeConfigSetString(config, iterator.path, keys[i]);
  }
}

size_t FcitxRimeConfigGetToggleKeySize(FcitxRime *rime, RimeConfig *config) {
  return RimeConfigListSize(config, "switcher/hotkeys");
}
  
void FcitxRimeConfigGetToggleKeys(FcitxRime* rime, RimeConfig* config, char** keys, size_t keys_size) {
  size_t s = RimeConfigListSize(config, "switcher/hotkeys");
  RimeConfigIterator iterator;
  RimeConfigBeginList(&iterator, config, "switcher/hotkeys");
  for(size_t i = 0; i < s; i ++) {
    RimeConfigNext(&iterator);
    if (i >= keys_size) {
      RimeConfigEnd(&iterator);
      break;
    } else {
      char* mem = (char*) fcitx_utils_malloc0(30);
      RimeConfigGetString(config, iterator.path, mem, 30); 
      keys[i] = mem;
    }
  }
}

void FcitxRimeBeginKeyBinding(RimeConfig* config) {
  global_iterator = (RimeConfigIterator*) fcitx_utils_malloc0(sizeof(RimeConfigIterator));  
  RimeConfigBeginList(global_iterator, config, "key_binder/bindings");
}
  
size_t FcitxRimeConfigGetKeyBindingSize(RimeConfig *config) {
  return RimeConfigListSize(config, "key_binder/bindings");
}
  
void FcitxRimeEndKeyBinding(RimeConfig* config) {
  fcitx_utils_free(global_iterator);
}
  
void FcitxRimeConfigGetNextKeyBinding(RimeConfig* config, char* act_type, char* act_key, char* accept, size_t buffer_size) {
  RimeConfigNext(global_iterator);
  RimeConfig map;
  memset(&map, 0, sizeof(RimeConfig));
  RimeConfigGetItem(config, global_iterator->path, &map);
    
  char* accept_try = (char*) fcitx_utils_malloc0(buffer_size * sizeof(char));
  memset(accept_try, 0 , buffer_size * sizeof(char));
  RimeConfigGetString(&map, "accept", accept_try, buffer_size);
    
  char* send_try = (char*) fcitx_utils_malloc0(buffer_size * sizeof(char));
  memset(send_try, 0 , buffer_size * sizeof(char));
  RimeConfigGetString(&map, "send", send_try, buffer_size);
    
  char* toggle_try = (char*) fcitx_utils_malloc0(buffer_size * sizeof(char));
  memset(toggle_try, 0 , buffer_size * sizeof(char));
  RimeConfigGetString(&map, "toggle", toggle_try, buffer_size);
    
  if(strlen(send_try) != 0) {
    strncpy(act_type, "send", buffer_size);
    strncpy(act_key, send_try, buffer_size);
  } else if (strlen(toggle_try) != 0) {
    strncpy(act_type, "toggle", buffer_size);
    strncpy(act_key, toggle_try, buffer_size);
  }
    
  strncpy(accept, accept_try, buffer_size);
  
  fcitx_utils_free(accept_try);
  fcitx_utils_free(send_try);
  fcitx_utils_free(toggle_try);
}
  
// type: 0: toggle, 1: send
// key: value of the act_type, Page_Up, Page_Down, etc
// value: shortcut
void FcitxRimeConfigSetKeyBindingSet(RimeConfig* config, int type, const char* key, const char** shortcuts, size_t shortcut_size) {
  RimeConfigIterator iter;
  size_t total_sz = RimeConfigListSize(config, "key_binder/bindings");
  RimeConfigBeginList(&iter, config, "key_binder/bindings");
  size_t ptr = 0;
  for(size_t i = 0; i < total_sz; i ++) {
    RimeConfigNext(&iter);
    RimeConfig map;
    memset(&map, 0, sizeof(RimeConfig));
    RimeConfigGetItem(config, iter.path, &map);
    size_t buffer_sz = 20;
    char* key_try = (char*) fcitx_utils_malloc0(buffer_sz * sizeof(char));
    if(type == 0) { // "toggle"
      RimeConfigGetString(&map, "toggle", key_try, buffer_sz);
      if(strcmp(key_try, key) == 0) { // matches!
	if(ptr < shortcut_size) {
	  RimeConfigSetString(&map, "accept", shortcuts[ptr++]);
	} else { // set the rest to ""
	  RimeConfigSetString(&map, "accept", "");
	}
      }
    } else if(type == 1) { // "send"
      RimeConfigGetString(&map, "send", key_try, buffer_sz);
      if(strcmp(key_try, key) == 0) { // matches!
	if(ptr < shortcut_size) {
	  RimeConfigSetString(&map, "accept", shortcuts[ptr++]);
	} else { // set the rest to ""
	  RimeConfigSetString(&map, "accept", "");
	}
      }
    }
    fcitx_utils_free(key_try);
  }
}
  
void FcitxRimeConfigSetKeyBinding(RimeConfig* config, const char* act_type, const char* act_key, const char* value, size_t index) {
  RimeConfigIterator iter;
  size_t sz = RimeConfigListSize(config, "key_binder/bindings");
  size_t j = 0;
  RimeConfigBeginList(&iter, config, "key_binder/bindings");
  for(size_t i = 0; i < sz; i ++) {
    RimeConfigNext(&iter);
    RimeConfig map;
    memset(&map, 0, sizeof(RimeConfig));
    RimeConfigGetItem(config, iter.path, &map);
    size_t buffer_size = 50;
    char* act_key_try = (char*) fcitx_utils_malloc0(buffer_size * sizeof(char));
    RimeConfigGetString(&map, act_type, act_key_try, buffer_size);
    if(strcmp(act_key_try, act_key) == 0) {
      if(j == index) { // we found the index-th keyboard shortcut
	RimeConfigSetString(&map, "accept", value);
      } else if(j > index) {
	RimeConfigSetString(&map, "accept", "");
      }
      j += 1;
    }
    fcitx_utils_free(act_key_try);
  }
}
  
void FcitxRimeDestroy(FcitxRime* rime) {
  RimeConfigClose(rime->default_conf);
  rime->api->finalize();
  fcitx_utils_free(rime);
}
  
// Write into config file and restart rime
void FcitxRimeConfigSync(FcitxRime* rime) {
  RimeStartMaintenanceOnWorkspaceChange();
  RimeConfigClose(rime->default_conf);
  rime->api->finalize();
  FcitxRimeConfigStart(rime);
  FcitxRimeConfigOpenDefault(rime);
}
  
void FcitxRimeGetSchemaAttr(FcitxRime* rime, const char* schema_id, char* name, size_t buffer_size, const char* attr) {
  RimeConfig rime_schema_config;
  memset(&rime_schema_config, 0, sizeof(RimeConfig));
  RimeSchemaOpen(schema_id, &rime_schema_config);
  RimeConfigGetString(&rime_schema_config, attr, name, buffer_size);
  RimeConfigClose(&rime_schema_config);
}

// reset active schema list to nothing
void FcitxRimeClearAndSetSchemaList(FcitxRime* rime, RimeConfig* config,
				    char ** schema_names, size_t count) {
  RimeConfigClear(config, "schema_list");
  RimeConfigCreateList(config, "schema_list");
  RimeConfigIterator iterator;
  RimeConfigBeginList(&iterator, config, "schema_list");
  RimeConfigNext(&iterator);
  for(size_t i = 0; i < count; i ++) {
    RimeConfigNext(&iterator);
    RimeConfigCreateMap(config, iterator.path);
    RimeConfig map;
    memset(&map, 0, sizeof(RimeConfig));
    RimeConfigGetItem(config, iterator.path, &map);
    RimeConfigSetString(&map, "schema", schema_names[i]);
  }
  return;
}
  
int FcitxRimeCheckSchemaEnabled(FcitxRime* rime, RimeConfig* config, const char* schema_id) {
  size_t s = RimeConfigListSize(config, "schema_list");
  RimeConfigIterator iterator;
  RimeConfigBeginList(&iterator, config, "schema_list");
  int result = 0;
  for(size_t i = 0; i < s; i ++) {
    RimeConfigNext(&iterator);
    RimeConfig map;
    memset(&map, 0, sizeof(RimeConfig));
    RimeConfigGetItem(config, iterator.path, &map);
    size_t buffer_size = 50;
    char* s = (char*) fcitx_utils_malloc0(buffer_size * sizeof(char));
    RimeConfigGetString(&map, "schema", s, buffer_size);
    if (strcmp(s, schema_id) == 0) { /* This schema is enabled in default*/        
      result = (i+1);
    }
  }
  return result;
}
  
