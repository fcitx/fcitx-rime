/*
 * Copyright (C) 2017~2017 by CSSlayer
 * wengxt@gmail.com
 *
 * This library is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation; either version 2.1 of the
 * License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; see the file COPYING. If not,
 * see <http://www.gnu.org/licenses/>.
 */

#include "rimeengine.h"
#include "notifications_public.h"
#include "rimestate.h"
#include <cstring>
#include <fcitx-utils/fs.h>
#include <fcitx-utils/log.h>
#include <fcitx-utils/standardpath.h>
#include <fcitx/candidatelist.h>
#include <fcitx/inputcontext.h>
#include <fcitx/inputcontextmanager.h>
#include <fcitx/inputpanel.h>
#include <rime_api.h>

namespace fcitx {

RimeEngine::RimeEngine(Instance *instance)
    : instance_(instance), api_(rime_get_api()),
      factory_([this](InputContext &) { return new RimeState(this); }) {
    reloadConfig();
}

RimeEngine::~RimeEngine() {
    factory_.unregister();
    if (api_) {
        api_->finalize();
    }
}

void RimeEngine::rimeStart(bool fullcheck) {
    if (!api_) {
        return;
    }

    auto userDir =
        StandardPath::global().userDirectory(StandardPath::Type::PkgData) +
        "/rime";
    fs::makePath(userDir);
    const char *sharedDataDir = RIME_DATA_DIR;

    RIME_STRUCT(RimeTraits, fcitx_rime_traits);
    fcitx_rime_traits.shared_data_dir = sharedDataDir;
    fcitx_rime_traits.app_name = "rime.fcitx-rime";
    fcitx_rime_traits.user_data_dir = userDir.c_str();
    fcitx_rime_traits.distribution_name = "Rime";
    fcitx_rime_traits.distribution_code_name = "fcitx-rime";
    fcitx_rime_traits.distribution_version = FCITX_RIME_VERSION;
    if (firstRun_) {
        api_->setup(&fcitx_rime_traits);
        firstRun_ = false;
    }
    api_->initialize(&fcitx_rime_traits);
    api_->set_notification_handler(&rimeNotificationHandler, this);
    api_->start_maintenance(fullcheck);
}

void RimeEngine::reloadConfig() {
    factory_.unregister();
    if (api_) {
        api_->finalize();
    }
    rimeStart(false);
    instance_->inputContextManager().registerProperty("rimeState", &factory_);
}
void RimeEngine::activate(const fcitx::InputMethodEntry &,
                          fcitx::InputContextEvent &) {}
void RimeEngine::keyEvent(const InputMethodEntry &entry, KeyEvent &event) {
    FCITX_UNUSED(entry);
    FCITX_LOG(Debug) << "Rime receive key: " << event.key() << " "
                     << event.isRelease();
    auto inputContext = event.inputContext();
    auto state = inputContext->propertyFor(&factory_);
    state->keyEvent(event);
}

void RimeEngine::reset(const InputMethodEntry &, InputContextEvent &event) {
    auto inputContext = event.inputContext();

    auto state = inputContext->propertyFor(&factory_);
    state->clear();
    inputContext->inputPanel().reset();
    inputContext->updatePreedit();
    inputContext->updateUserInterface(UserInterfaceComponent::InputPanel);
}

void RimeEngine::save() {}

void RimeEngine::rimeNotificationHandler(void *, RimeSessionId, const char *,
                                         const char *) {}

RimeState *RimeEngine::state(InputContext *ic) {
    return ic->propertyFor(&factory_);
}

std::string RimeEngine::subMode(const InputMethodEntry &, InputContext &ic) {
    auto rimeState = state(&ic);

    RIME_STRUCT(RimeStatus, status);
    if (rimeState->getStatus(&status)) {
        if (status.schema_name) {
            return status.schema_name;
        }
    }
    return {};
}
}

FCITX_ADDON_FACTORY(fcitx::RimeEngineFactory)
