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
        "zh_TW"
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
    FcitxInputStateSetCursorPos(input, context.composition.cursor_pos);
    FcitxMessagesAddMessageAtLast(msgPreedit, MSG_INPUT, "%s", context.composition.preedit);

    if (context.composition.sel_start < context.composition.sel_end) {/*
        glong start = g_utf8_strlen(context.composition.preedit, context.composition.sel_start);
        glong end = g_utf8_strlen(context.composition.preedit, context.composition.sel_end);
        ibus_attr_list_append(text->attrs,
                            ibus_attr_foreground_new(BLACK, start, end));
        ibus_attr_list_append(text->attrs,
                            ibus_attr_background_new(DARK, start, end));*/
    }

    if (context.menu.num_candidates)
    {
        FcitxCandidateWordList* candList = FcitxInputStateGetCandidateList(input);
        FcitxCandidateWordSetChoose(candList, DIGIT_STR_CHOOSE);
        int i;
        for (i = 0; i < context.menu.num_candidates; ++i) {
            FcitxCandidateWord candWord;
            candWord.strWord = strdup (context.menu.candidates[i]);
            candWord.wordType = MSG_INPUT;
            candWord.strExtra = NULL;
            candWord.callback = NULL;
            candWord.priv = NULL;

            FcitxCandidateWordAppend(candList, &candWord);
        }
    }

    return IRV_DISPLAY_CANDWORDS;
}