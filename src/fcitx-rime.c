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
static void FcitxRimeReloadConfig(void* arg);
static INPUT_RETURN_VALUE FcitxRimeDoInput(void* arg, FcitxKeySym sym, unsigned int state);
static INPUT_RETURN_VALUE FcitxRimeGetCandWords(void* arg);

FCITX_EXPORT_API
FcitxIMClass ime = {
    FcitxRimeCreate,
    FcitxRimeDestroy
};

FCITX_EXPORT_API
int ABI_VERSION = FCITX_ABI_VERSION;

static void FcitxRimeStart(FcitxRime* rime) {

    char* user_path = NULL;
    FILE* fp = FcitxXDGGetFileUserWithPrefix("rime", ".place_holder", "w", NULL);
    if (fp)
        fclose(fp);
    FcitxXDGGetFileUserWithPrefix("rime", "", NULL, &user_path);
    //char* shared_data_dir = fcitx_utils_get_fcitx_path_with_filename("pkgdatadir", "rime");
    const char* shared_data_dir = RIME_DATA_DIR;

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

    rime->session_id = RimeCreateSession();
}

static void* FcitxRimeCreate(FcitxInstance* instance)
{
    FcitxRime* rime = (FcitxRime*) fcitx_utils_malloc0(sizeof(FcitxRime));
    rime->owner = instance;
    FcitxRimeStart(rime);

    FcitxIMIFace iface;
    memset(&iface, 0, sizeof(FcitxIMIFace));
    iface.Init = FcitxRimeInit;
    iface.ResetIM = FcitxRimeReset;
    iface.DoInput = FcitxRimeDoInput;
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
    boolean flag = false;
    FcitxInstanceSetContext(rime->owner, CONTEXT_IM_KEYBOARD_LAYOUT, "us");
    FcitxInstanceSetContext(rime->owner, CONTEXT_DISABLE_AUTO_FIRST_CANDIDATE_HIGHTLIGHT, &flag);
    FcitxInstanceSetContext(rime->owner, CONTEXT_DISABLE_AUTOENG, &flag);
    FcitxInstanceSetContext(rime->owner, CONTEXT_DISABLE_QUICKPHRASE, &flag);

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

INPUT_RETURN_VALUE FcitxRimeDoInput(void* arg, FcitxKeySym _sym, unsigned int _state)
{
    FcitxRime *rime = (FcitxRime *)arg;
    FcitxInputState *input = FcitxInstanceGetInputState(rime->owner);
    uint32_t sym = FcitxInputStateGetKeySym(input);
    uint32_t state = FcitxInputStateGetKeyState(input);

    _state &= (FcitxKeyState_Ctrl | FcitxKeyState_Alt | FcitxKeyState_Shift | FcitxKeyState_Super);

    if (_state & (~(FcitxKeyState_Ctrl | FcitxKeyState_Alt))) {
        return IRV_TO_PROCESS;
    }

    state &= (FcitxKeyState_Ctrl | FcitxKeyState_Alt);

    if (!RimeFindSession(rime->session_id)) {
        rime->session_id = RimeCreateSession();
    }
    if (!rime->session_id) { // service disabled
        return IRV_TO_PROCESS;
    }
    boolean result = RimeProcessKey(rime->session_id, sym, state);

    RimeCommit commit = {0};
    if (RimeGetCommit(rime->session_id, &commit)) {
        FcitxInputContext* ic = FcitxInstanceGetCurrentIC(rime->owner);
        FcitxInstanceCommitString(rime->owner, ic, commit.text);
        RimeFreeCommit(&commit);
    }
    if (!result) {
        return IRV_TO_PROCESS;
    }
    else
        return IRV_DISPLAY_CANDWORDS;
}

INPUT_RETURN_VALUE FcitxRimeGetCandWord(void* arg, FcitxCandidateWord* candWord)
{
    RimeContext context = {0};
    FcitxRime *rime = (FcitxRime *)arg;
    RIME_STRUCT_INIT(RimeContext, context);
    INPUT_RETURN_VALUE retVal = IRV_TO_PROCESS;
    if (RimeGetContext(rime->session_id, &context)) {
        if (context.menu.num_candidates)
        {
            int i = *(int*) candWord->priv;
            const char* digit = DIGIT_STR_CHOOSE;
            int num_select_keys = strlen(context.menu.select_keys);
            FcitxKeySym sym = FcitxKey_None;
            if (i < 10) {
                if (i < num_select_keys)
                    sym = context.menu.select_keys[i];
                else
                    sym = digit[i];
            }
            if (sym != FcitxKey_None) {
                boolean result = RimeProcessKey(rime->session_id, sym, 0);
                RimeCommit commit = {0};
                if (RimeGetCommit(rime->session_id, &commit)) {
                    FcitxInputContext* ic = FcitxInstanceGetCurrentIC(rime->owner);
                    FcitxInstanceCommitString(rime->owner, ic, commit.text);
                    RimeFreeCommit(&commit);
                }
                if (!result)
                    retVal = IRV_TO_PROCESS;
                else
                    retVal = IRV_DISPLAY_CANDWORDS;
            }
        }
    }

    RimeFreeContext(&context);
    return retVal;
}


INPUT_RETURN_VALUE FcitxRimeGetCandWords(void* arg)
{
    FcitxRime *rime = (FcitxRime *)arg;
    FcitxInputState *input = FcitxInstanceGetInputState(rime->owner);
    FcitxInstanceCleanInputWindow(rime->owner);

    RimeContext context = {0};
    RIME_STRUCT_INIT(RimeContext, context);
    if (!RimeGetContext(rime->session_id, &context) ||
        context.composition.length == 0) {
        RimeFreeContext(&context);
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
        if (context.commit_text_preview) {
            FcitxMessagesAddMessageAtLast(msgClientPreedit, MSG_DONOT_COMMIT_WHEN_UNFOCUS, "%s", &context.composition.preedit[context.composition.sel_end]);
        }
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
    }

    RimeFreeContext(&context);
    return IRV_DISPLAY_CANDWORDS;
}

void FcitxRimeReloadConfig(void* arg)
{
    FcitxRime* rime = (FcitxRime*) arg;
    if (rime->session_id) {
        RimeDestroySession(rime->session_id);
        rime->session_id = 0;
    }
    RimeFinalize();
    FcitxRimeStart(rime);
}

