#include <fcitx/instance.h>
#include <fcitx/context.h>
#include <fcitx/candidate.h>
#include <fcitx/hook.h>
#include <fcitx-config/xdg.h>
#include <fcitx/module/freedesktop-notify/fcitx-freedesktop-notify.h>
#include <libintl.h>
#include <rime_api.h>

#define _(x) dgettext("fcitx-rime", (x))

typedef struct _FcitxRime {
    FcitxInstance* owner;
    RimeSessionId session_id;
    char* iconname;
    RimeApi* api;
    boolean firstRun;
    FcitxUIMenu schemamenu;
} FcitxRime;

static void* FcitxRimeCreate(FcitxInstance* instance);
static void FcitxRimeDestroy(void* arg);
static boolean FcitxRimeInit(void* arg);
static void FcitxRimeReset(void* arg);
static void FcitxRimeReloadConfig(void* arg);
static INPUT_RETURN_VALUE FcitxRimeDoInput(void* arg, FcitxKeySym sym, unsigned int state);
static INPUT_RETURN_VALUE FcitxRimeDoReleaseInput(void* arg, FcitxKeySym sym, unsigned int state);
static INPUT_RETURN_VALUE FcitxRimeDoInputReal(void* arg, FcitxKeySym _sym, unsigned int _state);
static INPUT_RETURN_VALUE FcitxRimeGetCandWords(void* arg);
static void FcitxRimeToggleEnZh(void* arg);
static const char* FcitxRimeGetIMIcon(void* arg);
static const char* FcitxRimeGetDeployIcon(void *arg);
static const char* FcitxRimeGetSyncIcon(void *arg);
static void FcitxRimeToggleSync(void* arg);
static void FcitxRimeToggleDeploy(void* arg);
static void FcitxRimeResetUI(void* arg);
static void FcitxRimeUpdateStatus(FcitxRime* rime);
static boolean FcitxRimeSchemaMenuAction(FcitxUIMenu *menu, int index);
static void FcitxRimeSchemaMenuUpdate(FcitxUIMenu *menu);
static boolean FcitxRimePaging(void* arg, boolean prev);

FCITX_EXPORT_API
FcitxIMClass ime = {
    FcitxRimeCreate,
    FcitxRimeDestroy
};

FCITX_EXPORT_API
int ABI_VERSION = FCITX_ABI_VERSION;

void FcitxRimeNotificationHandler(void* context_object,
                                  RimeSessionId session_id,
                                  const char* message_type,
                                  const char* message_value)
{
    const char* message = NULL;
    FcitxRime* rime = (FcitxRime*) context_object;
    if (!strcmp(message_type, "deploy")) {
        if (!strcmp(message_value, "start")) {
            message = _("Rime is under maintenance ...");
        } else if (!strcmp(message_value, "success")) {
            message = _("Rime is ready.");
        } else if (!strcmp(message_value, "failure")) {
            message = _("Rime has encountered an error. "
                        "See /tmp/rime.fcitx.ERROR for details.");
        }
    }

    if (message) {
        FcitxFreeDesktopNotifyShowAddonTip(rime->owner, "fcitx-rime-deploy", "fcitx-rime-deploy", _("Rime"), message);
    }
}

static void FcitxRimeStart(FcitxRime* rime, boolean fullcheck) {

    char* user_path = NULL;
    FILE* fp = FcitxXDGGetFileUserWithPrefix("rime", ".place_holder", "w", NULL);
    if (fp)
        fclose(fp);
    FcitxXDGGetFileUserWithPrefix("rime", "", NULL, &user_path);
    //char* shared_data_dir = fcitx_utils_get_fcitx_path_with_filename("pkgdatadir", "rime");
    const char* shared_data_dir = RIME_DATA_DIR;

    RIME_STRUCT(RimeTraits, fcitx_rime_traits);
    fcitx_rime_traits.shared_data_dir = shared_data_dir;
    fcitx_rime_traits.app_name = "rime.fcitx-rime";
    fcitx_rime_traits.user_data_dir = user_path;
    fcitx_rime_traits.distribution_name = "Rime";
    fcitx_rime_traits.distribution_code_name = "fcitx-rime";
    fcitx_rime_traits.distribution_version = "0.2.3";
    if (rime->firstRun) {
        rime->api->setup(&fcitx_rime_traits);
        rime->firstRun = false;
    }
    rime->api->initialize(&fcitx_rime_traits);
    rime->api->set_notification_handler(FcitxRimeNotificationHandler, rime);
    rime->api->start_maintenance(fullcheck);

    rime->session_id = rime->api->create_session();
    free(user_path);
}

static void* FcitxRimeCreate(FcitxInstance* instance)
{
    FcitxRime* rime = (FcitxRime*) fcitx_utils_malloc0(sizeof(FcitxRime));
    rime->owner = instance;
    rime->api = rime_get_api();
    rime->firstRun = true;
    if (!rime->api) {
        free(rime);
        return NULL;
    }

    FcitxRimeStart(rime, false);

    FcitxIMIFace iface;
    memset(&iface, 0, sizeof(FcitxIMIFace));
    iface.Init = FcitxRimeInit;
    iface.ResetIM = FcitxRimeReset;
    iface.DoInput = FcitxRimeDoInput;
    iface.DoReleaseInput = FcitxRimeDoReleaseInput;
    iface.GetCandWords = FcitxRimeGetCandWords;
    iface.ReloadConfig = FcitxRimeReloadConfig;

    FcitxInstanceRegisterIMv2(
        instance,
        rime,
        "rime",
        _("Rime"),
        "rime",
        iface,
        10,
        "zh"
    );

    FcitxUIRegisterComplexStatus(
        instance,
        rime,
        "rime-enzh",
        "",
        "",
        FcitxRimeToggleEnZh,
        FcitxRimeGetIMIcon);

    FcitxUIRegisterComplexStatus(
        instance,
        rime,
        "rime-deploy",
        _("Deploy"),
        _("Deploy"),
        FcitxRimeToggleDeploy,
        FcitxRimeGetDeployIcon);

    FcitxUIRegisterComplexStatus(
        instance,
        rime,
        "rime-sync",
        _("Synchronize"),
        _("Synchronize"),
        FcitxRimeToggleSync,
        FcitxRimeGetSyncIcon);

    FcitxUISetStatusVisable(instance, "rime-enzh", false);
    FcitxUISetStatusVisable(instance, "rime-sync", false);
    FcitxUISetStatusVisable(instance, "rime-deploy", false);
    FcitxIMEventHook hk;
    hk.arg = rime;
    hk.func = FcitxRimeResetUI;

    FcitxInstanceRegisterResetInputHook(instance, hk);
    
    FcitxMenuInit(&rime->schemamenu);
    rime->schemamenu.name = strdup(_("Schema List"));
    rime->schemamenu.candStatusBind = strdup("rime-enzh");
    rime->schemamenu.MenuAction = FcitxRimeSchemaMenuAction;
    rime->schemamenu.UpdateMenu = FcitxRimeSchemaMenuUpdate;
    rime->schemamenu.priv = rime;
    rime->schemamenu.isSubMenu = false;
    FcitxUIRegisterMenu(rime->owner, &rime->schemamenu);

    return rime;
}

void FcitxRimeDestroy(void* arg)
{
    FcitxRime* rime = (FcitxRime*) arg;
    if (rime->session_id) {
        rime->api->destroy_session(rime->session_id);
        rime->session_id = 0;
    }

    FcitxUIUnRegisterMenu(rime->owner, &rime->schemamenu);
    FcitxMenuFinalize(&rime->schemamenu);
    
    fcitx_utils_free(rime->iconname);
    rime->api->finalize();
    free(rime);
}

boolean FcitxRimeInit(void* arg)
{
    FcitxRime* rime = (FcitxRime*) arg;
    boolean flag = true;
    FcitxInstanceSetContext(rime->owner, CONTEXT_IM_KEYBOARD_LAYOUT, "us");
    FcitxInstanceSetContext(rime->owner, CONTEXT_DISABLE_AUTO_FIRST_CANDIDATE_HIGHTLIGHT, &flag);
    FcitxInstanceSetContext(rime->owner, CONTEXT_DISABLE_AUTOENG, &flag);
    FcitxInstanceSetContext(rime->owner, CONTEXT_DISABLE_QUICKPHRASE, &flag);

    FcitxRimeUpdateStatus(rime);

    return true;
}


void FcitxRimeReset(void* arg)
{
    FcitxRime* rime = (FcitxRime*) arg;

    if (rime->api->is_maintenance_mode()) {
        return;
    }
    if (!rime->api->find_session(rime->session_id)) {
        rime->session_id = rime->api->create_session();
    }
    if (rime->session_id) {
        rime->api->process_key(rime->session_id, FcitxKey_Escape, 0);
    }
}

INPUT_RETURN_VALUE FcitxRimeDoReleaseInput(void* arg, FcitxKeySym _sym, unsigned int _state)
{
    FcitxRime *rime = (FcitxRime *)arg;
    FcitxInputState *input = FcitxInstanceGetInputState(rime->owner);
    uint32_t sym = FcitxInputStateGetKeySym(input);
    uint32_t state = FcitxInputStateGetKeyState(input);
    _state &= (FcitxKeyState_SimpleMask);

    if (_state & (~(FcitxKeyState_Ctrl_Alt_Shift))) {
        return IRV_TO_PROCESS;
    }

    state &= (FcitxKeyState_SimpleMask);

    return FcitxRimeDoInputReal(arg, sym, state | (1 << 30));
}

void FcitxRimeUpdateStatus(FcitxRime* rime)
{
    if (rime->api->is_maintenance_mode()) {
        return;
    }
    if (!rime->api->find_session(rime->session_id)) {
        rime->session_id = rime->api->create_session();
    }
    
    RIME_STRUCT(RimeStatus, status);
    if (rime->api->get_status(rime->session_id, &status)) {
        char* text = "";
        if (status.is_disabled) {
            text = "\xe2\x8c\x9b";
        } else if (status.is_ascii_mode) {
            text = "A";
        } else if (status.schema_name &&
                status.schema_name[0] != '.') {
            text = status.schema_name;
        } else {
            text = "中";
        }
        FcitxUISetStatusString(rime->owner, "rime-enzh", text, text);
        rime->api->free_status(&status);
    } else {
        FcitxUISetStatusString(rime->owner, "rime-enzh", "\xe2\x8c\x9b", "\xe2\x8c\x9b");
    }
}

INPUT_RETURN_VALUE FcitxRimeDoInput(void* arg, FcitxKeySym _sym, unsigned int _state)
{
    FcitxRime *rime = (FcitxRime *)arg;
    FcitxInputState *input = FcitxInstanceGetInputState(rime->owner);
    uint32_t sym = FcitxInputStateGetKeySym(input);
    uint32_t state = FcitxInputStateGetKeyState(input);
    _state &= (FcitxKeyState_SimpleMask | FcitxKeyState_CapsLock);

    if (_state & (~(FcitxKeyState_Ctrl_Alt_Shift | FcitxKeyState_CapsLock))) {
        return IRV_TO_PROCESS;
    }

    state &= (FcitxKeyState_SimpleMask | FcitxKeyState_CapsLock);

    return FcitxRimeDoInputReal(arg, sym, state);
}

INPUT_RETURN_VALUE FcitxRimeDoInputReal(void* arg, FcitxKeySym sym, unsigned int state)
{
    FcitxRime *rime = (FcitxRime *)arg;

    if (rime->api->is_maintenance_mode()) {
        return IRV_TO_PROCESS;
    }
    if (!rime->api->find_session(rime->session_id)) {
        rime->session_id = rime->api->create_session();
    }
    if (!rime->session_id) { // service disabled
        FcitxRimeUpdateStatus(rime);
        return IRV_TO_PROCESS;
    }
    boolean result = rime->api->process_key(rime->session_id, sym, state);

    RIME_STRUCT(RimeCommit, commit);
    if (rime->api->get_commit(rime->session_id, &commit)) {
        FcitxInputContext* ic = FcitxInstanceGetCurrentIC(rime->owner);
        FcitxInstanceCommitString(rime->owner, ic, commit.text);
        rime->api->free_commit(&commit);
    }

    FcitxRimeUpdateStatus(rime);

    if (!result) {
        FcitxRimeGetCandWords(rime);
        FcitxUIUpdateInputWindow(rime->owner);
        return IRV_TO_PROCESS;
    }
    else
        return IRV_DISPLAY_CANDWORDS;
}

INPUT_RETURN_VALUE FcitxRimeGetCandWord(void* arg, FcitxCandidateWord* candWord)
{
    FcitxRime *rime = (FcitxRime *)arg;
    RIME_STRUCT(RimeContext, context);
    INPUT_RETURN_VALUE retVal = IRV_TO_PROCESS;
    if (rime->api->get_context(rime->session_id, &context)) {
        if (context.menu.num_candidates)
        {
            int i = *(int*) candWord->priv;
            const char* digit = DIGIT_STR_CHOOSE;
            int num_select_keys = context.menu.select_keys ? strlen(context.menu.select_keys) : 0;
            FcitxKeySym sym = FcitxKey_None;
            if (i < 10) {
                if (i < num_select_keys)
                    sym = context.menu.select_keys[i];
                else
                    sym = digit[i];
            }
            if (sym != FcitxKey_None) {
                boolean result = rime->api->process_key(rime->session_id, sym, 0);
                
                RIME_STRUCT(RimeCommit, commit);
                if (rime->api->get_commit(rime->session_id, &commit)) {
                    FcitxInputContext* ic = FcitxInstanceGetCurrentIC(rime->owner);
                    FcitxInstanceCommitString(rime->owner, ic, commit.text);
                    rime->api->free_commit(&commit);
                }
                if (!result) {
                    FcitxRimeGetCandWords(rime);
                    FcitxUIUpdateInputWindow(rime->owner);
                    retVal = IRV_TO_PROCESS;
                }
                else
                    retVal = IRV_DISPLAY_CANDWORDS;
            }
        }
        rime->api->free_context(&context);
    }

    return retVal;
}

boolean FcitxRimePaging(void* arg, boolean prev) {
    FcitxRime *rime = (FcitxRime *)arg;
    boolean result = RimeProcessKey(rime->session_id, prev ? FcitxKey_Page_Up : FcitxKey_Page_Down, 0);
    if (result) {
        FcitxRimeGetCandWords(rime);
        FcitxUIUpdateInputWindow(rime->owner);
    }
    return result;
}

INPUT_RETURN_VALUE FcitxRimeGetCandWords(void* arg)
{
    FcitxRime *rime = (FcitxRime *)arg;
    FcitxInputState *input = FcitxInstanceGetInputState(rime->owner);
    FcitxInstanceCleanInputWindow(rime->owner);

    RIME_STRUCT(RimeContext, context);
    if (!rime->api->get_context(rime->session_id, &context)) {
        return IRV_DISPLAY_CANDWORDS;
    }

    if (context.composition.length == 0) {
        rime->api->free_context(&context);
        return IRV_DISPLAY_CANDWORDS;
    }

    FcitxMessages* msgPreedit = FcitxInputStateGetPreedit(input);
    FcitxMessages* msgClientPreedit = FcitxInputStateGetClientPreedit(input);
    FcitxInputStateSetShowCursor(input, true);
    FcitxInputStateSetCursorPos(input, context.composition.cursor_pos);
    if (context.commit_text_preview) {
        FcitxInputStateSetClientCursorPos(input, strlen(context.commit_text_preview));
    }

    /* converted text */
    if (context.composition.sel_start > 0) {
        char* temp = strndup(context.composition.preedit, context.composition.sel_start);
        FcitxMessagesAddMessageAtLast(msgPreedit, MSG_OTHER, "%s", temp);
        free(temp);
        if (context.commit_text_preview) {
            temp = strndup(context.commit_text_preview, context.composition.sel_start);
            FcitxMessagesAddMessageAtLast(msgClientPreedit, MSG_INPUT, "%s", temp);
            free(temp);
        }
    }

    /* converting candidate */
    if (context.composition.sel_start < context.composition.sel_end) {
        char* temp = strndup(&context.composition.preedit[context.composition.sel_start], context.composition.sel_end - context.composition.sel_start);
        FcitxMessagesAddMessageAtLast(msgPreedit, MSG_HIGHLIGHT | MSG_CODE, "%s", temp);
        free(temp);
        if (context.commit_text_preview) {
            FcitxMessagesAddMessageAtLast(msgClientPreedit, MSG_HIGHLIGHT, "%s", &context.commit_text_preview[context.composition.sel_start]);
        }
    }

    /* remaining input to convert */
    if (context.composition.sel_end < strlen(context.composition.preedit)) {
        FcitxMessagesAddMessageAtLast(msgPreedit, MSG_CODE, "%s", &context.composition.preedit[context.composition.sel_end]);
    }

    if (context.menu.num_candidates)
    {
        FcitxCandidateWordList* candList = FcitxInputStateGetCandidateList(input);
        const char* digit = DIGIT_STR_CHOOSE;
        char strChoose[11];
        strChoose[10] = '\0';
        FcitxCandidateWordSetPageSize(candList, 10);
        int num_select_keys = context.menu.select_keys ? strlen(context.menu.select_keys) : 0;
        int i;
        for (i = 0; i < context.menu.num_candidates; ++i) {
            FcitxCandidateWord candWord;
            candWord.strWord = strdup (context.menu.candidates[i].text);
            if (i == context.menu.highlighted_candidate_index)
                candWord.wordType = MSG_CANDIATE_CURSOR;
            else
                candWord.wordType = MSG_OTHER;
            candWord.strExtra = context.menu.candidates[i].comment ? strdup (context.menu.candidates[i].comment) : NULL;
            candWord.extraType = MSG_CODE;
            candWord.callback = FcitxRimeGetCandWord;
            candWord.owner = rime;
            int* priv = fcitx_utils_new(int);
            *priv = i;
            candWord.priv = priv;

            FcitxCandidateWordAppend(candList, &candWord);
            if (i < 10) {
                if (i < num_select_keys) {
                    strChoose[i] = context.menu.select_keys[i];
                }
                else {
                    strChoose[i] = digit[i];
                }
            }
        }
        FcitxCandidateWordSetChoose(candList, strChoose);

        FcitxCandidateWordSetOverridePaging(candList, context.menu.page_no != 0, !context.menu.is_last_page, FcitxRimePaging, rime, NULL);
    }

    rime->api->free_context(&context);
    return IRV_DISPLAY_CANDWORDS;
}

void FcitxRimeReloadConfig(void* arg)
{
    FcitxRime* rime = (FcitxRime*) arg;
    if (rime->session_id) {
        rime->api->destroy_session(rime->session_id);
        rime->session_id = 0;
    }
    rime->api->finalize();
    FcitxRimeStart(rime, false);

    FcitxRimeUpdateStatus(rime);
}


boolean FcitxRimeSchemaMenuAction(FcitxUIMenu* menu, int index)
{
    FcitxRime * rime = menu->priv;
    if (rime->api->is_maintenance_mode()) {
        return false;
    }
    if (!rime->api->find_session(rime->session_id)) {
        rime->session_id = rime->api->create_session();
    }

    if (index == 0) {
        rime->api->set_option(rime->session_id, "ascii_mode", true);
    } else {
        rime->api->set_option(rime->session_id, "ascii_mode", false);
        RimeSchemaList list = {0};
        if (rime->api->get_schema_list(&list)) {
            if (list.size > index - 1) {
                rime->api->select_schema(rime->session_id,list.list[index-1].schema_id);
                FcitxRimeUpdateStatus(rime);
                FcitxRimeGetCandWords(rime);
                FcitxUIUpdateInputWindow(rime->owner);
            }
            rime->api->free_schema_list(&list);
        }
    }

    return true;
}

void FcitxRimeSchemaMenuUpdate(FcitxUIMenu* menu)
{
    FcitxRime * rime = menu->priv;
    if (rime->api->is_maintenance_mode()) {
        return;
    }
    if (!rime->api->find_session(rime->session_id)) {
        rime->session_id = rime->api->create_session();
    }

    FcitxMenuClear(menu);

    FcitxMenuAddMenuItem(menu, _("English"), MENUTYPE_SIMPLE, NULL);
    RimeSchemaList list = {0};
    if (rime->api->get_schema_list(&list)) {
        size_t i = 0;
        for (i = 0; i < list.size; ++i) {
            FcitxMenuAddMenuItem(menu, list.list[i].name, MENUTYPE_SIMPLE, NULL);
        }
        rime->api->free_schema_list(&list);
    }
}

static const char* FcitxRimeGetIMIcon(void* arg)
{
    FcitxRime* rime = (FcitxRime*) arg;
    RIME_STRUCT(RimeStatus, status);
    if (rime->api->get_status(rime->session_id, &status)) {
        char* text = "";
        if (status.is_disabled) {
            text = "@rime-disable";
        } else if (status.is_ascii_mode) {
            text = "@rime-latin";
        } else if (status.schema_id) {
            fcitx_utils_free(rime->iconname);
            fcitx_utils_alloc_cat_str(rime->iconname, "@rime-im-", status.schema_id);
            text = rime->iconname;
        } else {
            text = "@rime-im";
        }
        rime->api->free_status(&status);

        return text;
    }
    return "@rime-disable";
}

static const char* FcitxRimeGetDeployIcon(void *arg)
{
    return "rime-deploy";
}

static const char* FcitxRimeGetSyncIcon(void *arg)
{
    return "rime-sync";
}


void FcitxRimeToggleEnZh(void* arg)
{

}

void FcitxRimeResetUI(void* arg)
{

    FcitxRime* rime = (FcitxRime*) arg;
    FcitxInstance* instance = rime->owner;
    FcitxIM* im = FcitxInstanceGetCurrentIM(instance);
    boolean visible;
    if (!im || strcmp(im->uniqueName, "rime") != 0)
        visible = false;
    else
        visible = true;
    FcitxUISetStatusVisable(instance, "rime-enzh", visible);
    FcitxUISetStatusVisable(instance, "rime-sync", visible);
    FcitxUISetStatusVisable(instance, "rime-deploy", visible);
}

void FcitxRimeToggleSync(void* arg)
{
    FcitxRime* rime = (FcitxRime*) arg;
    rime->api->sync_user_data();
    FcitxRimeGetCandWords(rime);
    FcitxUIUpdateInputWindow(rime->owner);
}

void FcitxRimeToggleDeploy(void* arg)
{
    FcitxRime* rime = (FcitxRime*) arg;
    if (rime->session_id) {
        rime->api->sync_user_data();
        rime->session_id = 0;
    }
    rime->api->finalize();
    FcitxRimeStart(rime, true);

    FcitxRimeUpdateStatus(rime);
    FcitxRimeGetCandWords(rime);
    FcitxUIUpdateInputWindow(rime->owner);
}
