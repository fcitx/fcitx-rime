#ifndef FCITX_RIME_CONFIG_H
#define FCITX_RIME_CONFIG_H

#include <rime_api.h>

#define FCITX_RIME_DIR_PREFIX "rime"
#define FCITX_RIME_CONFIG_SUFFIX ".yaml"
#define FCITX_RIME_SCHEMA_SUFFIX ".schema.yaml"

#ifdef __cplusplus
extern "C" {
#endif
  
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
#ifdef __cplusplus
}
#endif

#endif // FCITX_RIME_CONFIG_H
