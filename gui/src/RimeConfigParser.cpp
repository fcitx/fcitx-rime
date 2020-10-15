//
// Copyright (C) 2018~2018 by xuzhao9 <i@xuzhao.net>
// Copyright (C) 2018~2018 by Weng Xuetian <wengxt@gmail.com>
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
#include "RimeConfigParser.h"
#include <QDebug>
#include <fcitx-config/xdg.h>
#include <fcitx-utils/utils.h>

namespace fcitx_rime {

RimeConfigParser::RimeConfigParser() : api(rime_get_api()), default_conf({0}), inError(false) {
    RimeModule *module = api->find_module("levers");
    if (!module) {
        inError = true;
    } else {
        levers = (RimeLeversApi *)module->get_api();
        start(true);
    }
}

RimeConfigParser::~RimeConfigParser() { api->finalize(); }

bool RimeConfigParser::isError() {
    return inError;
}

bool RimeConfigParser::start(bool firstRun) {
    bool suc;
    char *user_path = NULL;
    FILE *fp = FcitxXDGGetFileUserWithPrefix(fcitx_rime_dir_prefix,
                                             ".place_holder", "w", NULL);
    if (fp) {
        fclose(fp);
    }
    FcitxXDGGetFileUserWithPrefix(fcitx_rime_dir_prefix, "", nullptr,
                                  &user_path);

    RIME_STRUCT(RimeTraits, fcitx_rime_traits);
    fcitx_rime_traits.shared_data_dir = RIME_DATA_DIR;
    fcitx_rime_traits.user_data_dir = user_path;
    fcitx_rime_traits.distribution_name = "Rime";
    fcitx_rime_traits.distribution_code_name = "fcitx-rime-config";
    fcitx_rime_traits.distribution_version = "0.1.0";
    fcitx_rime_traits.app_name = "rime.fcitx-rime-config";
    if (firstRun) {
        api->setup(&fcitx_rime_traits);
    }
    default_conf = {0};
    api->initialize(&fcitx_rime_traits);
    settings = levers->custom_settings_init("default", "rime_patch");
    suc = levers->load_settings(settings);
    if(!suc) {
        return false;
    }
    suc = levers->settings_get_config(settings, &default_conf);
    if(!suc) {
        return false;
    }

    free(user_path);
    return true;
}

void RimeConfigParser::setToggleKeys(const std::vector<std::string> &keys) {
    api->config_clear(&default_conf, "switcher/hotkeys");
    api->config_create_list(&default_conf, "switcher/hotkeys");
    RimeConfigIterator iterator;
    api->config_begin_list(&iterator, &default_conf, "switcher/hotkeys");
    api->config_next(&iterator);
    for (size_t i = 0; i < keys.size(); i++) {
        api->config_next(&iterator);
        api->config_set_string(&default_conf, iterator.path, keys[i].data());
    }
    api->config_end(&iterator);
}

std::vector<std::string> RimeConfigParser::getToggleKeys() {
    std::vector<std::string> result;
    listForeach(&default_conf, "switcher/hotkeys",
                [=, &result](RimeConfig *config, const char *path) {
                    auto str = api->config_get_cstring(config, path);
                    if (str) {
                        result.push_back(str);
                    }
                    return true;
                });
    return result;
}

const char *keyBindingConditionToString(KeybindingCondition condition) {
    switch (condition) {
    case KeybindingCondition::Composing:
        return "composing";
    case KeybindingCondition::HasMenu:
        return "has_menu";
    case KeybindingCondition::Always:
        return "always";
    case KeybindingCondition::Paging:
        return "paging";
    }
    return "";
}

KeybindingCondition keyBindingConditionFromString(const char *str) {
    if (strcmp(str, "composing") == 0) {
        return KeybindingCondition::Composing;
    } else if (strcmp(str, "has_menu") == 0) {
        return KeybindingCondition::HasMenu;
    } else if (strcmp(str, "paging") == 0) {
        return KeybindingCondition::Paging;
    } else if (strcmp(str, "always") == 0) {
        return KeybindingCondition::Always;
    }
    return KeybindingCondition::Composing;
}

const char *keybindingTypeToString(KeybindingType type) {
    switch (type) {
    case KeybindingType::Send:
        return "send";
    case KeybindingType::Select:
        return "select";
    case KeybindingType::Toggle:
        return "toggle";
    }
    return "";
}

const char *switchKeyFunctionToString(SwitchKeyFunction type) {
    switch (type) {
    case SwitchKeyFunction::Noop:
        return "noop";
    case SwitchKeyFunction::InlineASCII:
        return "inline_ascii";
    case SwitchKeyFunction::CommitText:
        return "commit_text";
    case SwitchKeyFunction::CommitCode:
        return "commit_code";
    case SwitchKeyFunction::Clear:
        return "clear";
    }
    return "";
}

SwitchKeyFunction switchKeyFunctionFromString(const char *str) {
    if (strcmp(str, "noop") == 0) {
        return SwitchKeyFunction::Noop;
    } else if (strcmp(str, "inline_ascii") == 0) {
        return SwitchKeyFunction::InlineASCII;
    } else if (strcmp(str, "commit_text")) {
        return SwitchKeyFunction::CommitText;
    } else if (strcmp(str, "commit_code")) {
        return SwitchKeyFunction::CommitCode;
    } else if (strcmp(str, "clear")) {
        return SwitchKeyFunction::Clear;
    }
    return SwitchKeyFunction::Noop;
}

void RimeConfigParser::setKeybindings(const std::vector<Keybinding> &bindings) {
    RimeConfig copy_config = {0};
    RimeConfig copy_config_map = {0};
    RimeConfigIterator iterator;
    RimeConfigIterator copy_iterator;
    api->config_init(&copy_config);
    api->config_create_list(&copy_config, "key_binder/bindings");
    api->config_begin_list(&iterator, &default_conf, "key_binder/bindings");
    api->config_begin_list(&copy_iterator, &copy_config, "key_binder/bindings");
    while (!copy_iterator.path) {
        api->config_next(&copy_iterator);
    }
    while (api->config_next(&iterator)) {
        RimeConfig map = {0};
        const char *send_key = NULL;
        api->config_get_item(&default_conf, iterator.path, &map);
        send_key = api->config_get_cstring(&map, "send");
        if (!send_key) {
            send_key = api->config_get_cstring(&map, "toggle");
        }
        if (!send_key) {
            send_key = api->config_get_cstring(&map, "select");
        }
        if (strcmp(send_key, "Page_Up") && strcmp(send_key, "Page_Down") &&
            strcmp(send_key, "ascii_mode") && strcmp(send_key, "full_shape") &&
            strcmp(send_key, "simplification")) {
            api->config_set_item(&copy_config, copy_iterator.path, &map);
            api->config_next(&copy_iterator);
        }
    };
    api->config_end(&iterator);
    for (auto &binding : bindings) {
        RimeConfig map = {0};
        api->config_init(&map);
        api->config_set_string(&map, "accept", binding.accept.data());
        api->config_set_string(&map, "when",
                               keyBindingConditionToString(binding.when));
        api->config_set_string(&map, keybindingTypeToString(binding.type),
                               binding.action.data());
        api->config_set_item(&copy_config, copy_iterator.path, &map);
        api->config_next(&copy_iterator);
    }
    api->config_end(&copy_iterator);
    api->config_get_item(&copy_config, "key_binder/bindings", &copy_config_map);
    api->config_set_item(&default_conf, "key_binder/bindings",
                         &copy_config_map);
}

void RimeConfigParser::setPageSize(int page_size) {
    api->config_set_int(&default_conf, "menu/page_size", page_size);
}

bool RimeConfigParser::getPageSize(int *page_size) {
    return api->config_get_int(&default_conf, "menu/page_size", page_size);
}

std::vector<Keybinding> RimeConfigParser::getKeybindings() {
    std::vector<Keybinding> result;
    listForeach(&default_conf, "key_binder/bindings",
                [=, &result](RimeConfig *config, const char *path) {
                    RimeConfig map = {0};
                    api->config_get_item(config, path, &map);
                    auto when = api->config_get_cstring(&map, "when");
                    if (!when) {
                        return false;
                    }
                    Keybinding binding;
                    binding.when = keyBindingConditionFromString(when);
                    auto accept = api->config_get_cstring(&map, "accept");
                    if (!accept) {
                        return false;
                    }
                    binding.accept = accept;
                    auto action = api->config_get_cstring(&map, "send");
                    if (action) {
                        binding.type = KeybindingType::Send;
                    } else {
                        action = api->config_get_cstring(&map, "toggle");
                    }
                    if (action) {
                        binding.type = KeybindingType::Toggle;
                    } else {
                        action = api->config_get_cstring(&map, "select");
                        binding.type = KeybindingType::Select;
                    }
                    if (!action) {
                        return false;
                    }
                    binding.action = action;
                    result.push_back(std::move(binding));
                    return true;
                });
    return result;
}

void RimeConfigParser::listForeach(
    RimeConfig *config, const char *key,
    std::function<bool(RimeConfig *, const char *)> callback) {
    size_t size = RimeConfigListSize(config, key);
    if (!size) {
        return;
    }

    RimeConfigIterator iterator;
    RimeConfigBeginList(&iterator, config, key);
    for (auto i = 0; i < size; i++) {
        RimeConfigNext(&iterator);
        if (!callback(config, iterator.path)) {
            break;
        }
    }
    RimeConfigEnd(&iterator);
}

bool RimeConfigParser::sync() {
    int page_size;
    bool suc;
    RimeConfig hotkeys = {0};
    RimeConfig keybindings = {0};
    RimeConfig schema_list = {0};
    std::string yaml;

    api->config_get_int(&default_conf, "menu/page_size", &page_size);
    levers->customize_int(settings, "menu/page_size", page_size);
    api->config_get_item(&default_conf, "switcher/hotkeys", &hotkeys);
    levers->customize_item(settings, "switcher/hotkeys", &hotkeys);
    api->config_get_item(&default_conf, "key_binder/bindings", &keybindings);
    levers->customize_item(settings, "key_binder/bindings", &keybindings);
    levers->customize_string(
        settings, "ascii_composer/switch_key/Shift_L",
        api->config_get_cstring(&default_conf,
                                "ascii_composer/switch_key/Shift_L"));
    levers->customize_string(
        settings, "ascii_composer/switch_key/Shift_R",
        api->config_get_cstring(&default_conf,
                                "ascii_composer/switch_key/Shift_R"));

    /* Concatenate all active schemas */
    for (const auto &schema : schema_id_list) {
        yaml += "- { schema: " + schema + " } \n";
    }
    api->config_load_string(&schema_list, yaml.c_str());
    levers->customize_item(settings, "schema_list", &schema_list);
    suc = levers->save_settings(settings);
    if(!suc) {
        return false;
    }
    levers->custom_settings_destroy(settings);
    suc = api->start_maintenance(true); // Full check mode
    if(!suc) {
        return false;
    }
    api->finalize();
    return start(false);
}

std::string RimeConfigParser::stringFromYAML(const char *yaml,
                                             const char *attr) {
    RimeConfig rime_schema_config = {0};
    api->config_load_string(&rime_schema_config, yaml);
    auto str = api->config_get_cstring(&rime_schema_config, attr);
    std::string result;
    if (str) {
        result = str;
    }
    return result;
}

void RimeConfigParser::setSchemas(const std::vector<std::string> &schemas) {
    schema_id_list = schemas;
    return;
}

int RimeConfigParser::schemaIndex(const char *schema_id) {
    int idx = 0;
    bool found = false;
    listForeach(&default_conf, "schema_list",
                [=, &idx, &found](RimeConfig *config, const char *path) {
                    RimeConfig map = {0};
                    this->api->config_get_item(config, path, &map);
                    auto schema = this->api->config_get_cstring(&map, "schema");
                    /* This schema is enabled in default */
                    if (schema && strcmp(schema, schema_id) == 0) {
                        found = true;
                        return false;
                    }
                    idx++;
                    return true;
                });

    return found ? (idx + 1) : 0;
}

std::vector<SwitchKeyFunction> RimeConfigParser::getSwitchKeys() {
    std::vector<SwitchKeyFunction> out;
    const char *shift_l = NULL, *shift_r = NULL;
    shift_l = api->config_get_cstring(&default_conf,
                                      "ascii_composer/switch_key/Shift_L");
    shift_r = api->config_get_cstring(&default_conf,
                                      "ascii_composer/switch_key/Shift_R");
    out.push_back(switchKeyFunctionFromString(shift_l));
    out.push_back(switchKeyFunctionFromString(shift_r));
    return out;
}

void RimeConfigParser::setSwitchKeys(
    const std::vector<SwitchKeyFunction> &switch_keys) {
    if (switch_keys.size() < 2) {
        return;
    }
    api->config_set_string(&default_conf, "ascii_composer/switch_key/Shift_L",
                           switchKeyFunctionToString(switch_keys[0]));
    api->config_set_string(&default_conf, "ascii_composer/switch_key/Shift_R",
                           switchKeyFunctionToString(switch_keys[1]));
    return;
}

} // namespace fcitx_rime
