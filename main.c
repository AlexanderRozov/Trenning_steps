#include "src/db.h"
#include "src/gui.h"

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

static int read_int(const char *prompt, int min, int max) {
    char buf[64];
    int value;
    for (;;) {
        printf("%s", prompt);
        if (!fgets(buf, sizeof buf, stdin)) return min - 1;
        if (sscanf(buf, "%d", &value) == 1 && value >= min && value <= max) {
            return value;
        }
        printf("Некорректный ввод, попробуйте ещё раз.\n");
    }
}

static void ui_generate(void) {
    int days = read_int("Сколько дней сгенерировать: ", 1, 365);
    int added = generate_schedule(days);
    if (added >= 0) {
        printf("Добавлено дней: %d\n", added);
    } else {
        printf("Ошибка генерации расписания.\n");
    }
}

static void ui_list(void) {
    printf("\n--- Расписание тренировок ---\n");
    print_schedule();
}

static void ui_set_done(int done) {
    print_schedule();
    int id = read_int("Введите ID записи: ", 1, 2147483647);
    int rc = set_done(id, done);
    if (rc == 0) {
        printf("Запись %d: %s.\n", id, done ? "выполнено" : "не выполнено");
    } else if (rc == 1) {
        printf("Запись с ID %d не найдена.\n", id);
    }
}

static void ui_delete(void) {
    print_schedule();
    int id = read_int("Введите ID для удаления: ", 1, 2147483647);
    int rc = delete_entry(id);
    if (rc == 0) {
        printf("Запись %d удалена.\n", id);
    } else if (rc == 1) {
        printf("Запись с ID %d не найдена.\n", id);
    }
}

static void print_menu(void) {
    printf("\n========== tREND ==========\n");
    printf("1. Сгенерировать расписание\n");
    printf("2. Показать расписание\n");
    printf("3. Отметить выполненным\n");
    printf("4. Сбросить статус\n");
    printf("5. Удалить запись\n");
    printf("0. Выход\n");
    printf("===========================\n");
}

static int run_cli(void) {
    printf("База данных '%s' подключена.\n", DB_PATH);

    for (;;) {
        print_menu();
        int choice = read_int("Выбор: ", 0, 5);
        switch (choice) {
            case 1: ui_generate(); break;
            case 2: ui_list(); break;
            case 3: ui_set_done(1); break;
            case 4: ui_set_done(0); break;
            case 5: ui_delete(); break;
            case 0:
                printf("До свидания!\n");
                return 0;
        }
    }
}

int main(int argc, char **argv) {
    setup_console();

    if (db_open(DB_PATH) != 0) {
        return 1;
    }

    int rc;
    if (argc > 1 && strcmp(argv[1], "--cli") == 0) {
        rc = run_cli();
    } else {
        rc = run_gui();
    }

    db_close();
    return rc;
}
