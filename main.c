#include "src/db.h"

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

static void ui_add_date(void) {
    struct Date d;
    printf("\n--- Новая дата ---\n");
    d.day = read_int("День (1-31): ", 1, 31);
    d.month = read_int("Месяц (1-12): ", 1, 12);
    d.year = read_int("Год (1-9999): ", 1, 9999);

    if (save_date(d) == 0) {
        printf("Дата %02d.%02d.%04d сохранена.\n", d.day, d.month, d.year);
    }
}

static void ui_list_dates(void) {
    printf("\n--- Сохранённые даты ---\n");
    print_all_dates();
}

static void ui_delete_date(void) {
    printf("\n--- Удаление даты ---\n");
    print_all_dates();
    int id = read_int("Введите ID для удаления: ", 1, 2147483647);
    int rc = delete_date(id);
    if (rc == 0) {
        printf("Запись %d удалена.\n", id);
    } else if (rc == 1) {
        printf("Запись с ID %d не найдена.\n", id);
    }
}

static void print_menu(void) {
    printf("\n========== tREND ==========\n");
    printf("1. Добавить дату\n");
    printf("2. Показать все даты\n");
    printf("3. Удалить дату\n");
    printf("0. Выход\n");
    printf("===========================\n");
}

int main(void) {
    setup_console();

    if (db_open(DB_PATH) != 0) {
        return 1;
    }

    printf("База данных '%s' подключена.\n", DB_PATH);

    for (;;) {
        print_menu();
        int choice = read_int("Выбор: ", 0, 3);
        switch (choice) {
            case 1: ui_add_date(); break;
            case 2: ui_list_dates(); break;
            case 3: ui_delete_date(); break;
            case 0:
                db_close();
                printf("До свидания!\n");
                return 0;
        }
    }
}
