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

class RimeConfigCleanUp {
public:
    RimeConfigCleanUp(RimeConfig *config) : config_(config) {}
    ~RimeConfigCleanUp() { RimeConfigClose(config_); }

private:
    RimeConfig *config_;
};

RimeConfigParser::RimeConfigParser() : api(rime_get_api()) {
    memset(&default_conf, 0, sizeof(default_conf));
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
    fcitx_rime_traits.app_name = "rime.fcitx-rime-config";
    fcitx_rime_traits.user_data_dir = user_path;
    fcitx_rime_traits.distribution_name = "Rime";
    fcitx_rime_traits.distribution_code_name = "fcitx-rime-config";
    fcitx_rime_traits.distribution_version = "0.0.2";
    if (firstRun) {
        api->setup(&fcitx_rime_traits);
    }
    api->initialize(&fcitx_rime_traits);
    api->config_open("default", &default_conf);

    free(user_path);
}

void RimeConfigParser::setToggleKeys(const std::vector<std::string> &keys) {
    RimeConfigClear(&default_conf, "switcher/hotkeys");
    RimeConfigCreateList(&default_conf, "switcher/hotkeys");
    RimeConfigIterator iterator;
    RimeConfigBeginList(&iterator, &default_conf, "switcher/hotkeys");
    RimeConfigNext(&iterator);
    for (size_t i = 0; i < keys.size(); i++) {
        RimeConfigNext(&iterator);
        RimeConfigSetString(&default_conf, iterator.path, keys[i].data());
    }
    RimeConfigEnd(&iterator);
}

std::vector<std::string> RimeConfigParser::toggleKeys() {
    std::vector<std::string> result;
    listForeach(&default_conf, "switcher/hotkeys",
                [&result](RimeConfig *config, const char *path) {
                    auto str = RimeConfigGetCString(config, path);
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
    RimeConfigClear(&default_conf, "key_binder/bindings");
    RimeConfigCreateList(&default_conf, "key_binder/bindings");
    RimeConfigIterator iterator;
    RimeConfigBeginList(&iterator, &default_conf, "key_binder/bindings");
    RimeConfigNext(&iterator);
    for (const auto &binding : bindings) {
        RimeConfigNext(&iterator);
        RimeConfigCreateMap(&default_conf, iterator.path);
        RimeConfig map;
        memset(&map, 0, sizeof(RimeConfig));
        RimeConfigCleanUp cleanUp(&map);
        RimeConfigGetItem(&default_conf, iterator.path, &map);
        RimeConfigSetString(&map, "when",
                            keyBindingConditionToString(binding.when));
        RimeConfigSetString(&map, "accept", binding.accept.data());
        RimeConfigSetString(&map, keybindingTypeToString(binding.type),
                            binding.action.data());
    }
    RimeConfigEnd(&iterator);
}

void RimeConfigParser::setInteger(const char *key, int i) {
    RimeConfigSetInt(&default_conf, key, i);
}

bool RimeConfigParser::readInteger(const char *key, int *i) {
    return RimeConfigGetInt(&default_conf, key, i);
}

std::vector<Keybinding> RimeConfigParser::keybindings() {
    std::vector<Keybinding> result;
    listForeach(&default_conf, "key_binder/bindings",
                [&result](RimeConfig *config, const char *path) {
                    RimeConfig map;
                    memset(&map, 0, sizeof(RimeConfig));
                    RimeConfigCleanUp cleanUp(&map);
                    RimeConfigGetItem(config, path, &map);
                    auto when = RimeConfigGetCString(&map, "when");
                    if (!when) {
                        return false;
                    }
                    Keybinding binding;
                    binding.when = keyBindingConditionFromString(when);
                    auto accept = RimeConfigGetCString(&map, "accept");
                    if (!accept) {
                        return false;
                    }
                    binding.accept = accept;
                    auto action = RimeConfigGetCString(&map, "send");
                    if (action) {
                        binding.type = KeybindingType::Send;
                    } else {
                        action = RimeConfigGetCString(&map, "toggle");
                    }
                    if (action) {
                        binding.type = KeybindingType::Toggle;
                    } else {
                        action = RimeConfigGetCString(&map, "select");
                        binding.type = KeybindingType::Select;
                    }
                    if (!action) {
                        return false;
                    }
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
    RimeConfigClose(&default_conf);
    memset(&default_conf, 0, sizeof(default_conf));
    api->finalize();
}

void RimeConfigParser::sync() {
    RimeStartMaintenanceOnWorkspaceChange();
    finalize();
    start();
}

std::string RimeConfigParser::stringFromYAML(const char *yaml,
                                             const char *attr) {
    RimeConfig rime_schema_config;
    memset(&rime_schema_config, 0, sizeof(RimeConfig));
    RimeConfigLoadString(&rime_schema_config, yaml);
    RimeConfigCleanUp cleanUp(&rime_schema_config);
    auto str = RimeConfigGetCString(&rime_schema_config, attr);
    std::string result;
    if (str) {
        result = str;
    }
    return result;
}

// reset active schema list to nothing
void RimeConfigParser::setSchemas(const std::vector<std::string> &schemas) {
    RimeConfigClear(&default_conf, "schema_list");
    RimeConfigCreateList(&default_conf, "schema_list");
    RimeConfigIterator iterator;
    RimeConfigBeginList(&iterator, &default_conf, "schema_list");
    RimeConfigNext(&iterator);
    for (const auto &schema : schemas) {
        RimeConfigNext(&iterator);
        RimeConfigCreateMap(&default_conf, iterator.path);
        RimeConfig map;
        memset(&map, 0, sizeof(RimeConfig));
        RimeConfigCleanUp cleanUp(&map);
        RimeConfigGetItem(&default_conf, iterator.path, &map);
        RimeConfigSetString(&map, "schema", schema.data());
    }
    RimeConfigEnd(&iterator);
    return;
}

int RimeConfigParser::schemaIndex(const char *schema_id) {
    int idx = 0;
    bool found = false;
    listForeach(
        &default_conf, "schema_list",
        [&idx, &found, schema_id](RimeConfig *config, const char *path) {
            RimeConfig map;
            memset(&map, 0, sizeof(RimeConfig));
            RimeConfigCleanUp cleanUp(&map);
            RimeConfigGetItem(config, path, &map);
            auto schema = RimeConfigGetCString(&map, "schema");
            /* This schema is enabled in default*/
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
