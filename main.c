#include "src/db.h"
#include "src/gui.h"
#include "src/lang.h"

#include <stdio.h>
#include <string.h>

#ifdef _WIN32
#include <windows.h>
#endif

#define DB_PATH "trend.db"

static void setup_console(void) {
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
#endif
}

static void init_language(void) {
    char code[8];
    get_setting_str("language", code, sizeof code, "ru");
    int lang = lang_from_code(code);
    lang_set(lang >= 0 ? lang : LANG_RU);
}

static int read_int(const char *prompt, int min, int max) {
    char buf[64];
    int value;
    for (;;) {
        printf("%s", prompt);
        if (!fgets(buf, sizeof buf, stdin)) return min - 1;
        if (sscanf(buf, "%d", &value) == 1 && value >= min && value <= max) {
            return value;
        }
        printf("%s\n", tr(S_INVALID_INPUT));
    }
}

static void ui_generate(void) {
    int days = read_int(tr(S_PROMPT_DAYS), 1, 365);
    int added = generate_schedule(days);
    if (added >= 0) {
        printf(tr(S_MSG_ADDED), added);
        printf("\n");
    } else {
        printf("%s\n", tr(S_MSG_GEN_ERROR));
    }
}

static void ui_list(void) {
    printf("\n%s\n", tr(S_LIST_HEADER));
    print_schedule();
}

static void ui_set_done(int done) {
    print_schedule();
    int id = read_int(tr(S_PROMPT_ID), 1, 2147483647);
    int rc = set_done(id, done);
    if (rc == 0) {
        printf(tr(S_RECORD_SET), id, tr(done ? S_DONE : S_NOT_DONE));
        printf("\n");
    } else if (rc == 1) {
        printf(tr(S_RECORD_NOT_FOUND), id);
        printf("\n");
    }
}

static void ui_delete(void) {
    print_schedule();
    int id = read_int(tr(S_PROMPT_ID), 1, 2147483647);
    int rc = delete_entry(id);
    if (rc == 0) {
        printf(tr(S_RECORD_DELETED), id);
        printf("\n");
    } else if (rc == 1) {
        printf(tr(S_RECORD_NOT_FOUND), id);
        printf("\n");
    }
}

static void ui_language(void) {
    printf("\n%s\n", tr(S_CHOOSE_LANG));
    for (int i = 0; i < LANG_COUNT; i++) {
        printf("%d. %s\n", i + 1, lang_name(i));
    }
    int choice = read_int(tr(S_MENU_CHOICE), 1, LANG_COUNT);
    if (choice >= 1) {
        int lang = choice - 1;
        lang_set(lang);
        set_setting("language", lang_code(lang));
        printf("%s\n", lang_name(lang));
    }
}

static void print_menu(void) {
    printf("\n========== tREND ==========\n");
    printf("1. %s\n", tr(S_MENU_GENERATE));
    printf("2. %s\n", tr(S_MENU_LIST));
    printf("3. %s\n", tr(S_MENU_DONE));
    printf("4. %s\n", tr(S_MENU_RESET));
    printf("5. %s\n", tr(S_MENU_DELETE));
    printf("6. %s\n", tr(S_MENU_LANGUAGE));
    printf("0. %s\n", tr(S_MENU_EXIT));
    printf("===========================\n");
}

static int run_cli(void) {
    printf(tr(S_DB_CONNECTED), DB_PATH);
    printf("\n");

    for (;;) {
        print_menu();
        int choice = read_int(tr(S_MENU_CHOICE), 0, 6);
        switch (choice) {
            case 1: ui_generate(); break;
            case 2: ui_list(); break;
            case 3: ui_set_done(1); break;
            case 4: ui_set_done(0); break;
            case 5: ui_delete(); break;
            case 6: ui_language(); break;
            case 0:
                printf("%s\n", tr(S_BYE));
                return 0;
        }
    }
}

int main(int argc, char **argv) {
    setup_console();

    if (db_open(DB_PATH) != 0) {
        return 1;
    }
    init_language();

    int rc;
    if (argc > 1 && strcmp(argv[1], "--cli") == 0) {
        rc = run_cli();
    } else {
        rc = run_gui();
    }

    db_close();
    return rc;
}
