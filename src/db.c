#include "db.h"
#include "sqlite3.h"

#include <stdio.h>

static sqlite3 *g_db = NULL;

int db_open(const char *path) {
    if (sqlite3_open(path, &g_db) != SQLITE_OK) {
        fprintf(stderr, "Ошибка открытия базы данных: %s\n", sqlite3_errmsg(g_db));
        sqlite3_close(g_db);
        g_db = NULL;
        return -1;
    }

    const char *sql =
        "CREATE TABLE IF NOT EXISTS dates ("
        "  id    INTEGER PRIMARY KEY AUTOINCREMENT,"
        "  day   INTEGER NOT NULL,"
        "  month INTEGER NOT NULL,"
        "  year  INTEGER NOT NULL"
        ");";

    char *err = NULL;
    if (sqlite3_exec(g_db, sql, NULL, NULL, &err) != SQLITE_OK) {
        fprintf(stderr, "Ошибка создания таблицы: %s\n", err);
        sqlite3_free(err);
        return -1;
    }
    return 0;
}

void db_close(void) {
    if (g_db) {
        sqlite3_close(g_db);
        g_db = NULL;
    }
}

int save_date(struct Date d) {
    if (!g_db) return -1;

    sqlite3_stmt *stmt = NULL;
    const char *sql = "INSERT INTO dates (day, month, year) VALUES (?, ?, ?);";

    if (sqlite3_prepare_v2(g_db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        fprintf(stderr, "Ошибка подготовки запроса: %s\n", sqlite3_errmsg(g_db));
        return -1;
    }

    sqlite3_bind_int(stmt, 1, d.day);
    sqlite3_bind_int(stmt, 2, d.month);
    sqlite3_bind_int(stmt, 3, d.year);

    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    if (rc != SQLITE_DONE) {
        fprintf(stderr, "Ошибка сохранения: %s\n", sqlite3_errmsg(g_db));
        return -1;
    }
    return 0;
}

int delete_date(int id) {
    if (!g_db) return -1;

    sqlite3_stmt *stmt = NULL;
    const char *sql = "DELETE FROM dates WHERE id = ?;";

    if (sqlite3_prepare_v2(g_db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        fprintf(stderr, "Ошибка подготовки запроса: %s\n", sqlite3_errmsg(g_db));
        return -1;
    }

    sqlite3_bind_int(stmt, 1, id);

    int rc = sqlite3_step(stmt);
    int changes = sqlite3_changes(g_db);
    sqlite3_finalize(stmt);

    if (rc != SQLITE_DONE) {
        fprintf(stderr, "Ошибка удаления: %s\n", sqlite3_errmsg(g_db));
        return -1;
    }
    return changes > 0 ? 0 : 1;
}

int get_all_dates(struct DateRow *out, int max) {
    if (!g_db) return -1;

    sqlite3_stmt *stmt = NULL;
    const char *sql = "SELECT id, day, month, year FROM dates ORDER BY year, month, day;";

    if (sqlite3_prepare_v2(g_db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        fprintf(stderr, "Ошибка подготовки запроса: %s\n", sqlite3_errmsg(g_db));
        return -1;
    }

    int count = 0;
    while (count < max && sqlite3_step(stmt) == SQLITE_ROW) {
        out[count].id = sqlite3_column_int(stmt, 0);
        out[count].date.day = sqlite3_column_int(stmt, 1);
        out[count].date.month = sqlite3_column_int(stmt, 2);
        out[count].date.year = sqlite3_column_int(stmt, 3);
        count++;
    }
    sqlite3_finalize(stmt);
    return count;
}

int print_all_dates(void) {
    struct DateRow rows[1024];
    int count = get_all_dates(rows, 1024);
    if (count < 0) return -1;

    printf("+------+------------+\n");
    printf("|  ID  |    Дата    |\n");
    printf("+------+------------+\n");
    for (int i = 0; i < count; i++) {
        printf("| %4d | %02d.%02d.%04d |\n",
               rows[i].id,
               rows[i].date.day,
               rows[i].date.month,
               rows[i].date.year);
    }
    printf("+------+------------+\n");
    if (count == 0) {
        printf("Записей пока нет.\n");
    } else {
        printf("Всего записей: %d\n", count);
    }
    return 0;
}
