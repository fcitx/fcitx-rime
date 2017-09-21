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

#include "rime.h"
#include "notifications_public.h"
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

class RimeState : public InputContextProperty {
public:
    RimeState(RimeEngine *engine) : engine_(engine) {
        if (auto api = engine_->api()) {
            session_ = api->create_session();
        }
    }

    virtual ~RimeState() {
        if (auto api = engine_->api()) {
            api->destroy_session(session_);
        }
    }

    void clear() {
        if (auto api = engine_->api()) {
            api->clear_composition(session_);
        }
    }

    void keyEvent(KeyEvent &event) {
        auto api = engine_->api();
        if (!api || api->is_maintenance_mode()) {
            return;
        }
        if (!api->find_session(session_)) {
            session_ = api->create_session();
        }
        if (!session_) {
            return;
        }
        uint32_t states = event.key().states();
        if (event.isRelease()) {
            states |= (1 << 30);
        }
        auto result = api->process_key(session_, event.key().sym(), states);

        auto ic = event.inputContext();
        RIME_STRUCT(RimeCommit, commit);
        if (api->get_commit(session_, &commit)) {
            ic->commitString(commit.text);
            api->free_commit(&commit);
        }

        updateUI(ic);

        if (result) {
            event.filterAndAccept();
        }
    }

    bool getStatus(RimeStatus *status) {
        auto api = engine_->api();
        if (!api || !session_) {
            return false;
        }
        return api->get_status(session_, status);
    }

    void updateUI(InputContext *ic);

    RimeEngine *engine_;
    RimeSessionId session_ = 0;
};

class RimeCandidateWord : public CandidateWord {
public:
    RimeCandidateWord(RimeEngine *engine, const RimeCandidate &candidate,
                      KeySym sym)
        : CandidateWord(), engine_(engine), sym_(sym) {
        Text text;
        text.append(std::string(candidate.text));
        if (candidate.comment && strlen(candidate.comment)) {
            text.append(" ");
            text.append(std::string(candidate.comment));
        }
        setText(text);
    }

    void select(InputContext *inputContext) const override {
        auto state = engine_->state(inputContext);
        KeyEvent event(inputContext, Key(sym_));
        state->keyEvent(event);
    }

    RimeEngine *engine_;
    KeySym sym_;
};

class RimeCandidateList final : public CandidateList,
                                public PageableCandidateList {
public:
    RimeCandidateList(RimeEngine *engine, InputContext *ic,
                      const RimeMenu &menu)
        : engine_(engine), ic_(ic), hasPrev_(menu.page_no != 0),
          hasNext_(!menu.is_last_page) {
        setPageable(this);

        int num_select_keys = menu.select_keys ? strlen(menu.select_keys) : 0;
        int i;
        for (i = 0; i < menu.num_candidates; ++i) {
            KeySym sym = FcitxKey_None;
            if (i < num_select_keys) {
                sym = static_cast<KeySym>(menu.select_keys[i]);
                labels_.emplace_back(std::string(menu.select_keys[i], 1));
            } else {
                sym = static_cast<KeySym>('0' + (i + 1) % 10);
                labels_.emplace_back(std::to_string((i + 1) % 10));
            }
            auto candWord = std::make_shared<RimeCandidateWord>(
                engine, menu.candidates[i], sym);
            candidateWords_.push_back(candWord);

            if (i == menu.highlighted_candidate_index) {
                cursor_ = i;
            }
        }
    }

    const Text &label(int idx) const override {
        checkIndex(idx);
        return labels_[idx];
    }

    std::shared_ptr<const CandidateWord> candidate(int idx) const override {
        checkIndex(idx);
        return candidateWords_[idx];
    }
    int size() const override { return candidateWords_.size(); }

    int cursorIndex() const override { return cursor_; }

    CandidateLayoutHint layoutHint() const override { return layout_; }

    bool hasPrev() const override { return hasPrev_; }
    bool hasNext() const override { return hasNext_; }
    void prev() override {
        KeyEvent event(ic_, Key(FcitxKey_Page_Up));
        engine_->state(ic_)->keyEvent(event);

    }
    void next() override {
        KeyEvent event(ic_, Key(FcitxKey_Page_Down));
        engine_->state(ic_)->keyEvent(event);
    }

    bool usedNextBefore() const override { return true; }

private:
    void checkIndex(int idx) const {
        if (idx < 0 && idx >= size()) {
            throw std::invalid_argument("invalid index");
        }
    }

    RimeEngine *engine_;
    InputContext *ic_;
    std::vector<Text> labels_;
    bool hasPrev_ = false;
    bool hasNext_ = false;
    CandidateLayoutHint layout_ = CandidateLayoutHint::Vertical;
    int cursor_ = -1;
    std::vector<std::shared_ptr<CandidateWord>> candidateWords_;
};

void RimeState::updateUI(InputContext *ic) {
    ic->inputPanel().reset();
    // FIXME: update status (button, menu)

    do {
        auto api = engine_->api();
        if (!api || api->is_maintenance_mode()) {
            return;
        }

        RIME_STRUCT(RimeContext, context);
        if (!api->get_context(session_, &context)) {
            break;
        }

        if (context.composition.length == 0) {
            api->free_context(&context);
            break;
        }
        Text preedit;
        Text clientPreedit;

        /* converted text */
        if (context.composition.sel_start > 0) {
            preedit.append(std::string(context.composition.preedit,
                                       context.composition.sel_start));
            if (context.commit_text_preview) {
                clientPreedit.append(
                    std::string(context.commit_text_preview,
                                context.composition.sel_start));
            }
        }

        /* converting candidate */
        if (context.composition.sel_start < context.composition.sel_end) {
            preedit.append(
                std::string(
                    &context.composition.preedit[context.composition.sel_start],
                    &context.composition.preedit[context.composition.sel_end]),
                TextFormatFlag::HighLight);
            if (context.commit_text_preview) {
                clientPreedit.append(
                    std::string(&context.commit_text_preview[context.composition
                                                                 .sel_start]),
                    TextFormatFlag::HighLight);
            }
        }

        /* remaining input to convert */
        if (static_cast<size_t>(context.composition.sel_end) < strlen(context.composition.preedit)) {
            preedit.append(std::string(
                &context.composition.preedit[context.composition.sel_end]));
        }

        preedit.setCursor(context.composition.cursor_pos);
        ic->inputPanel().setPreedit(preedit);
        ic->inputPanel().setClientPreedit(clientPreedit);

        if (context.menu.num_candidates) {
            ic->inputPanel().setCandidateList(
                new RimeCandidateList(engine_, ic, context.menu));

            api->free_context(&context);
        }
    } while (0);

    ic->updatePreedit();
    ic->updateUserInterface(UserInterfaceComponent::InputPanel);
}

RimeEngine::RimeEngine(Instance *instance)
    : instance_(instance), api_(rime_get_api()),
      factory_([this](InputContext &) { return new RimeState(this); }) {
    reloadConfig();
    KeySym syms[] = {
        FcitxKey_1, FcitxKey_2, FcitxKey_3, FcitxKey_4, FcitxKey_5,
        FcitxKey_6, FcitxKey_7, FcitxKey_8, FcitxKey_9, FcitxKey_0,
    };

    KeyStates states;
    for (auto sym : syms) {
        selectionKeys_.emplace_back(sym, states);
    }
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
void RimeEngine::activate(const fcitx::InputMethodEntry &entry,
                          fcitx::InputContextEvent &event) {}
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

void RimeEngine::rimeNotificationHandler(void *, RimeSessionId session_id,
                                        const char *message_type,
                                        const char *message_value) {}

RimeState *RimeEngine::state(InputContext *ic) {
    return ic->propertyFor(&factory_);
}

std::string RimeEngine::subMode(const InputMethodEntry &entry, InputContext &ic) {
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
