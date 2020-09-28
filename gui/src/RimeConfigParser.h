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
#ifndef FCITX_RIME_CONFIG_H
#define FCITX_RIME_CONFIG_H

#include <functional>
#include <rime_levers_api.h>
#include <string>
#include <vector>

static constexpr const char *fcitx_rime_dir_prefix = "rime";
static constexpr const char *fcitx_rime_schema_suffix = ".schema.yaml";

namespace fcitx_rime {

enum class KeybindingCondition {
    Composing,
    HasMenu,
    Paging,
    Always,
};

enum class KeybindingType {
    Send,
    Toggle,
    Select,
};

struct Keybinding {
    KeybindingCondition when;
    std::string accept;
    KeybindingType type;
    std::string action;
};

class RimeConfigParser {
public:
    RimeConfigParser();
    ~RimeConfigParser();

    void sync();
    void setToggleKeys(const std::vector<std::string> &keys);
    std::vector<std::string> toggleKeys();

    void setKeybindings(const std::vector<Keybinding> &bindings);
    std::vector<Keybinding> keybindings();

    void setPageSize(int page_size);
    bool getPageSize(int *page_size);

    std::string stringFromYAML(const char *yaml, const char *attr);
    void setSchemas(const std::vector<std::string> &schemas);
    int schemaIndex(const char *schema);

private:
    void start(bool firstRun = false);
    void finalize();
    static void
    listForeach(RimeConfig *config, const char *key,
                std::function<bool(RimeConfig *config, const char *path)>);

    RimeApi *api;
    RimeLeversApi *levers;

    RimeCustomSettings *settings;
    RimeConfig default_conf;
    std::vector<std::string> schema_id_list;
};

} // namespace fcitx_rime

#endif // FCITX_RIME_CONFIG_H
