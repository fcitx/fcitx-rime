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
#ifndef FCITX_RIME_CONFIG_H
#define FCITX_RIME_CONFIG_H

#include <rime_api.h>

static constexpr const char* fcitx_rime_dir_prefix = "rime";
static constexpr const char* fcitx_rime_schema_suffix = ".schema.yaml";
  
typedef struct _FcitxRime {
  RimeApi* api;
  RimeConfig* default_conf;
  RimeConfig* custom_conf;
  Bool firstRun;
} FcitxRime;
  
// open and start, sync and close
FcitxRime* FcitxRimeConfigCreate();
void FcitxRimeConfigStart(FcitxRime* rime);
RimeConfig* FcitxRimeConfigOpenDefault(FcitxRime* rime);
void FcitxRimeConfigSync(FcitxRime* rime);
void FcitxRimeDestroy(FcitxRime* rime);
// toggle
size_t FcitxRimeConfigGetToggleKeySize(FcitxRime *rime, RimeConfig *config);
void FcitxRimeConfigGetToggleKeys(FcitxRime* rime, RimeConfig* config, char** keys, size_t keys_size);
void FcitxRimeConfigSetToggleKeys(FcitxRime* rime, RimeConfig* config, const char** keys,
				  size_t keys_size);
// other bindings
void FcitxRimeBeginKeyBinding(RimeConfig* config);
size_t FcitxRimeConfigGetKeyBindingSize(RimeConfig *config);
void FcitxRimeConfigGetNextKeyBinding(RimeConfig* config, char* act_type, char* act_key, char* accept, size_t buffer_size);
void FcitxRimeEndKeyBinding(RimeConfig* config);
void FcitxRimeConfigSetKeyBinding(RimeConfig* config, const char* act_type, const char* act_key, const char* value, size_t index);
void FcitxRimeConfigSetKeyBindingSet(RimeConfig* config, int type, const char* key, const char** shortcuts, size_t shortcut_size);
// schemas
void FcitxRimeGetSchemaAttr(FcitxRime* rime, const char* schema_id, char* name, size_t buffer_size, const char* attr);
int FcitxRimeCheckSchemaEnabled(FcitxRime* rime, RimeConfig* config, const char* schema_id);
void FcitxRimeClearAndSetSchemaList(FcitxRime* rime, RimeConfig* config,
				    char ** schema_names, size_t count);

#endif // FCITX_RIME_CONFIG_H
