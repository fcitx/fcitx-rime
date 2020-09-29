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

RimeConfigParser::RimeConfigParser() : api(rime_get_api()), default_conf({0}) {
    RimeModule* module = api->find_module("levers");
    if(!module) {
        qDebug() << "missing Rime module: levers, please check your install.\n";
        exit(1);
    }
    levers = (RimeLeversApi*)module->get_api();
    start(true);
}

RimeConfigParser::~RimeConfigParser() { finalize(); }

void RimeConfigParser::start(bool firstRun) {
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
    api->initialize(&fcitx_rime_traits);
    settings = levers->custom_settings_init("default", "rime_patch");
    levers->load_settings(settings);
    levers->settings_get_config(settings, &default_conf);
    free(user_path);
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

std::vector<std::string> RimeConfigParser::toggleKeys() {
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

void RimeConfigParser::setKeybindings(const std::vector<Keybinding> &bindings) {
    api->config_clear(&default_conf, "key_binder/bindings");
    api->config_create_list(&default_conf, "key_binder/bindings");
    RimeConfigIterator iterator;
    api->config_begin_list(&iterator, &default_conf, "key_binder/bindings");
    api->config_next(&iterator);
    for (const auto &binding : bindings) {
        api->config_next(&iterator);
        api->config_create_map(&default_conf, iterator.path);
        RimeConfig map = {0};
        api->config_get_item(&default_conf, iterator.path, &map);
        api->config_set_string(&map, "when",
                               keyBindingConditionToString(binding.when));
        api->config_set_string(&map, "accept", binding.accept.data());
        api->config_set_string(&map, keybindingTypeToString(binding.type),
                            binding.action.data());
    }
    api->config_end(&iterator);
}

void RimeConfigParser::setPageSize(int page_size) {
    api->config_set_int(&default_conf, "menu/page_size", page_size);
}

bool RimeConfigParser::getPageSize(int *page_size) {
    return api->config_get_int(&default_conf, "menu/page_size", page_size);
}

std::vector<Keybinding> RimeConfigParser::keybindings() {
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

void RimeConfigParser::finalize() {
    api->finalize();
}

void RimeConfigParser::sync() {
    int page_size;
    RimeConfig hotkeys = {0};
    RimeConfig keybindings = {0};
    RimeConfig schema_list = {0};
    api->config_get_int(&default_conf, "menu/page_size", &page_size);
    levers->customize_int(settings, "menu/page_size", page_size);
    api->config_get_item(&default_conf, "switcher/hotkeys", &hotkeys);
    levers->customize_item(settings, "switcher/hotkeys", &hotkeys);
    api->config_get_item(&default_conf, "key_binder/bindings", &keybindings);
    levers->customize_item(settings, "key_binder/bindings", &keybindings);

    /* Concatenate all active schemas */
    std::string yaml = "";
    for (const auto &schema : schema_id_list) {
        yaml += "- { schema: " + schema + " } \n";
    }
    api->config_load_string(&schema_list, yaml.c_str());
    levers->customize_item(settings, "schema_list", &schema_list);
    levers->save_settings(settings);
    levers->custom_settings_destroy(settings);
    api->start_maintenance(true); // Full check mode
    RimeStartMaintenanceOnWorkspaceChange();
    finalize();
    start(false);
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
    listForeach(
        &default_conf, "schema_list",
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

} // namespace fcitx_rime
