#include <fcitx/instance.h>
#include <fcitx/context.h>
#include <fcitx/candidate.h>
#include <fcitx-config/xdg.h>
#include <libintl.h>
#include <rime_api.h>

#define _(x) dgettext("fcitx-rime", (x))

typedef struct _FcitxRime {
    FcitxInstance* owner;
    RimeSessionId session_id;
} FcitxRime;

static void* FcitxRimeCreate(FcitxInstance* instance);
static void FcitxRimeDestroy(void* arg);
static boolean FcitxRimeInit(void* arg);
static void FcitxRimeReset(void* arg);
static INPUT_RETURN_VALUE FcitxRimeDoInput(void* arg, FcitxKeySym sym, unsigned int state);
static INPUT_RETURN_VALUE FcitxRimeGetCandWords(void* arg);

FCITX_EXPORT_API
FcitxIMClass ime = {
    FcitxRimeCreate,
    FcitxRimeDestroy
};

FCITX_EXPORT_API
int ABI_VERSION = FCITX_ABI_VERSION;

static void* FcitxRimeCreate(FcitxInstance* instance)
{
    FcitxRime* rime = (FcitxRime*) fcitx_utils_malloc0(sizeof(FcitxRime));

    char* user_path = NULL;
    FILE* fp = FcitxXDGGetFileUserWithPrefix("rime", ".place_holder", "w", NULL);
    if (fp)
        fclose(fp);
    FcitxXDGGetFileUserWithPrefix("rime", "", NULL, &user_path);
    char* shared_data_dir = fcitx_utils_get_fcitx_path_with_filename("pkgdatadir", "rime");

    RimeTraits ibus_rime_traits;
    ibus_rime_traits.shared_data_dir = shared_data_dir;
    ibus_rime_traits.user_data_dir = user_path;
    ibus_rime_traits.distribution_name = "Rime";
    ibus_rime_traits.distribution_code_name = "fcitx-rime";
    ibus_rime_traits.distribution_version = "0.1";
    RimeInitialize(&ibus_rime_traits);
    if (RimeStartMaintenanceOnWorkspaceChange()) {
        // TODO: notification...
    }

    rime->owner = instance;
    rime->session_id = RimeCreateSession();
    FcitxInstanceRegisterIM(
        instance,
        rime,
        "rime",
        _("Rime"),
        "rime",
        FcitxRimeInit,
        FcitxRimeReset,
        FcitxRimeDoInput,
        FcitxRimeGetCandWords,
        NULL,
        NULL,
        NULL,
        NULL,
        10,
        "zh"
    );

    return rime;
}

void FcitxRimeDestroy(void* arg)
{
    FcitxRime* rime = (FcitxRime*) arg;
    if (rime->session_id) {
        RimeDestroySession(rime->session_id);
        rime->session_id = 0;
    }
    RimeFinalize();
}

boolean FcitxRimeInit(void* arg)
{
    FcitxRime* rime = (FcitxRime*) arg;
    FcitxInstanceSetContext(rime->owner, CONTEXT_IM_KEYBOARD_LAYOUT, "us");
    return true;
}


void FcitxRimeReset(void* arg)
{
    FcitxRime* rime = (FcitxRime*) arg;
    if (!RimeFindSession(rime->session_id)) {
        rime->session_id = RimeCreateSession();
    }
    if (rime->session_id) {
        RimeProcessKey(rime->session_id, FcitxKey_Escape, 0);
    }
}

INPUT_RETURN_VALUE FcitxRimeDoInput(void* arg, FcitxKeySym sym, unsigned int state)
{
    FcitxRime *rime = (FcitxRime *)arg;

    state &= (FcitxKeyState_Ctrl | FcitxKeyState_Alt);

    if (!RimeFindSession(rime->session_id)) {
        rime->session_id = RimeCreateSession();
    }
    if (!rime->session_id) { // service disabled
        return IRV_TO_PROCESS;
    }
    boolean result = RimeProcessKey(rime->session_id, sym, state);

    RimeCommit commit;
    if (RimeGetCommit(rime->session_id, &commit)) {
        FcitxInputContext* ic = FcitxInstanceGetCurrentIC(rime->owner);
        FcitxInstanceCommitString(rime->owner, ic, commit.text);
    }
    if (!result) {
        return IRV_TO_PROCESS;
    }
    else
        return IRV_DISPLAY_CANDWORDS;
}


INPUT_RETURN_VALUE FcitxRimeGetCandWords(void* arg)
{
    FcitxRime *rime = (FcitxRime *)arg;
    FcitxInputState *input = FcitxInstanceGetInputState(rime->owner);
    FcitxInstanceCleanInputWindow(rime->owner);

    RimeContext context;
    if (!RimeGetContext(rime->session_id, &context) ||
        context.composition.length == 0) {
        return IRV_DISPLAY_CANDWORDS;
    }

    FcitxMessages* msgPreedit = FcitxInputStateGetPreedit(input);
    FcitxMessages* msgClientPreedit = FcitxInputStateGetClientPreedit(input);
    FcitxInputStateSetShowCursor(input, false);
    FcitxInputStateSetClientCursorPos(input, context.composition.cursor_pos);
    FcitxMessagesAddMessageAtLast(msgPreedit, MSG_INPUT, "%s", context.composition.preedit);

    if (context.composition.sel_start > 0) {
        char* temp = strndup(context.composition.preedit, context.composition.sel_start);
        FcitxMessagesAddMessageAtLast(msgClientPreedit, MSG_DONOT_COMMIT_WHEN_UNFOCUS, "%s", temp);
        free(temp);
    }

    if (context.composition.sel_start < context.composition.sel_end) {
        char* temp = strndup(&context.composition.preedit[context.composition.sel_start], context.composition.sel_end - context.composition.sel_start);
        FcitxMessagesAddMessageAtLast(msgClientPreedit, MSG_HIGHLIGHT | MSG_DONOT_COMMIT_WHEN_UNFOCUS, "%s", temp);
        free(temp);
    }

    if (context.composition.sel_end > 0) {
        FcitxMessagesAddMessageAtLast(msgClientPreedit, MSG_DONOT_COMMIT_WHEN_UNFOCUS, "%s", &context.composition.preedit[context.composition.sel_end]);
    }

    if (context.menu.num_candidates)
    {
        FcitxCandidateWordList* candList = FcitxInputStateGetCandidateList(input);
        const char* digit = DIGIT_STR_CHOOSE;
        char strChoose[11];
        strChoose[10] = '\0';
        FcitxCandidateWordSetPageSize(candList, 10);
        int num_select_keys = strlen(context.menu.select_keys);
        int i;
        for (i = 0; i < context.menu.num_candidates; ++i) {
            FcitxCandidateWord candWord;
            candWord.strWord = strdup (context.menu.candidates[i]);
            candWord.wordType = MSG_INPUT;
            candWord.strExtra = NULL;
            candWord.callback = NULL;
            candWord.priv = NULL;

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
    }

    return IRV_DISPLAY_CANDWORDS;
}