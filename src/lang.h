#ifndef TREND_LANG_H
#define TREND_LANG_H

enum Lang {
    LANG_RU = 0,
    LANG_EN,
    LANG_DE,
    LANG_JV,
    LANG_FR,
    LANG_ZH,
    LANG_PT,
    LANG_ES,
    LANG_COUNT
};

enum StrId {
    S_APP_TITLE = 0,
    S_TODAY,
    S_TODAY_LINE,
    S_MARK_DONE,
    S_NO_TODAY,
    S_SCHEDULE,
    S_GENERATE,
    S_REFRESH,
    S_COL_DATE,
    S_COL_STEPS,
    S_COL_STATUS,
    S_DONE,
    S_NOT_DONE,
    S_BTN_DONE,
    S_BTN_RESET,
    S_BTN_DELETE,
    S_SETTINGS,
    S_START_STEPS,
    S_INCREMENT,
    S_NOTIFY_HOUR,
    S_NOTIFY_MINUTE,
    S_NOTIFY_PERIOD,
    S_AUTOSTART,
    S_LANGUAGE,
    S_MSG_ADDED,
    S_MSG_GEN_ERROR,
    S_MSG_READ_ERROR,
    S_MSG_TRAINING_DONE,
    S_MSG_MARKED_DONE,
    S_MSG_MARKED_UNDONE,
    S_MSG_DELETED,
    S_MSG_AUTOSTART_ON,
    S_MSG_AUTOSTART_OFF,
    S_MSG_AUTOSTART_ERROR,
    S_NOTIFY_TITLE,
    S_NOTIFY_TEXT,
    S_DB_CONNECTED,
    S_MENU_GENERATE,
    S_MENU_LIST,
    S_MENU_DONE,
    S_MENU_RESET,
    S_MENU_DELETE,
    S_MENU_LANGUAGE,
    S_MENU_EXIT,
    S_MENU_CHOICE,
    S_INVALID_INPUT,
    S_PROMPT_DAYS,
    S_PROMPT_ID,
    S_RECORD_SET,
    S_RECORD_DELETED,
    S_RECORD_NOT_FOUND,
    S_BYE,
    S_LIST_HEADER,
    S_EMPTY,
    S_TOTAL,
    S_CHOOSE_LANG,
    S_COUNT
};

void lang_set(int lang);
int lang_get(void);
int lang_from_code(const char *code);
const char *lang_code(int lang);
const char *lang_name(int lang);
const char *tr(int id);

#endif
