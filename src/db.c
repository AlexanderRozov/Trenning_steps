#include "db.h"
#include "sqlite3.h"

#include <stdio.h>
#include <string.h>
#include <time.h>

#define KEY_START_STEPS   "start_steps"
#define KEY_STEP_INCREMENT "step_increment"
#define KEY_NOTIFY_HOUR   "notify_hour"
#define KEY_NOTIFY_MINUTE "notify_minute"
#define KEY_NOTIFY_PERIOD "notify_period_days"
#define KEY_LAST_NOTIFY   "last_notify"

#define DEFAULT_START_STEPS 5000
#define DEFAULT_STEP_INCREMENT 500
#define DEFAULT_NOTIFY_HOUR 9
#define DEFAULT_NOTIFY_MINUTE 0
#define DEFAULT_NOTIFY_PERIOD 1

static sqlite3 *g_db = NULL;

static int exec_simple(const char *sql) {
    char *err = NULL;
    if (sqlite3_exec(g_db, sql, NULL, NULL, &err) != SQLITE_OK) {
        fprintf(stderr, "Ошибка SQL: %s\n", err);
        sqlite3_free(err);
        return -1;
    }
    return 0;
}

int db_open(const char *path) {
    if (sqlite3_open(path, &g_db) != SQLITE_OK) {
        fprintf(stderr, "Ошибка открытия базы данных: %s\n", sqlite3_errmsg(g_db));
        sqlite3_close(g_db);
        g_db = NULL;
        return -1;
    }

    if (exec_simple(
            "CREATE TABLE IF NOT EXISTS schedule ("
            "  id    INTEGER PRIMARY KEY AUTOINCREMENT,"
            "  date  TEXT NOT NULL UNIQUE,"
            "  steps INTEGER NOT NULL,"
            "  done  INTEGER NOT NULL DEFAULT 0"
            ");") != 0)
        return -1;

    if (exec_simple(
            "CREATE TABLE IF NOT EXISTS settings ("
            "  key   TEXT PRIMARY KEY,"
            "  value TEXT NOT NULL"
            ");") != 0)
        return -1;

    /* Значения по умолчанию */
    sqlite3_stmt *st;
    sqlite3_prepare_v2(g_db,
        "INSERT OR IGNORE INTO settings (key, value) VALUES (?, ?);",
        -1, &st, NULL);

    const struct { const char *key; const char *value; } defaults[] = {
        { KEY_START_STEPS,    "5000" },
        { KEY_STEP_INCREMENT, "500"  },
        { KEY_NOTIFY_HOUR,    "9"    },
        { KEY_NOTIFY_MINUTE,  "0"    },
        { KEY_NOTIFY_PERIOD,  "1"    },
        { KEY_LAST_NOTIFY,    ""     },
    };
    for (size_t i = 0; i < sizeof defaults / sizeof defaults[0]; i++) {
        sqlite3_reset(st);
        sqlite3_bind_text(st, 1, defaults[i].key, -1, SQLITE_STATIC);
        sqlite3_bind_text(st, 2, defaults[i].value, -1, SQLITE_STATIC);
        sqlite3_step(st);
    }
    sqlite3_finalize(st);
    return 0;
}

void db_close(void) {
    if (g_db) {
        sqlite3_close(g_db);
        g_db = NULL;
    }
}

void get_setting_str(const char *key, char *out, int out_size, const char *fallback) {
    if (!g_db || out_size <= 0) {
        if (out_size > 0) snprintf(out, out_size, "%s", fallback);
        return;
    }

    sqlite3_stmt *st;
    if (sqlite3_prepare_v2(g_db,
            "SELECT value FROM settings WHERE key = ?;", -1, &st, NULL) != SQLITE_OK) {
        snprintf(out, out_size, "%s", fallback);
        return;
    }
    sqlite3_bind_text(st, 1, key, -1, SQLITE_STATIC);

    if (sqlite3_step(st) == SQLITE_ROW) {
        snprintf(out, out_size, "%s", (const char *)sqlite3_column_text(st, 0));
    } else {
        snprintf(out, out_size, "%s", fallback);
    }
    sqlite3_finalize(st);
}

int get_setting_int(const char *key, int fallback) {
    char buf[32];
    get_setting_str(key, buf, sizeof buf, "");
    if (buf[0] == '\0') return fallback;
    int value;
    return sscanf(buf, "%d", &value) == 1 ? value : fallback;
}

int set_setting(const char *key, const char *value) {
    if (!g_db) return -1;

    sqlite3_stmt *st;
    if (sqlite3_prepare_v2(g_db,
            "INSERT INTO settings (key, value) VALUES (?, ?) "
            "ON CONFLICT(key) DO UPDATE SET value = excluded.value;",
            -1, &st, NULL) != SQLITE_OK)
        return -1;

    sqlite3_bind_text(st, 1, key, -1, SQLITE_STATIC);
    sqlite3_bind_text(st, 2, value, -1, SQLITE_STATIC);
    int rc = sqlite3_step(st);
    sqlite3_finalize(st);
    return rc == SQLITE_DONE ? 0 : -1;
}

int set_setting_int(const char *key, int value) {
    char buf[32];
    snprintf(buf, sizeof buf, "%d", value);
    return set_setting(key, buf);
}

static void format_date(int year, int month, int day, char out[11]) {
    snprintf(out, 11, "%04d-%02d-%02d", year, month, day);
}

int generate_schedule(int days) {
    if (!g_db || days <= 0) return -1;

    int start_steps = get_setting_int(KEY_START_STEPS, DEFAULT_START_STEPS);
    int increment = get_setting_int(KEY_STEP_INCREMENT, DEFAULT_STEP_INCREMENT);

    /* Последняя дата и её число шагов */
    char last_date[11] = "";
    int last_steps = 0;
    sqlite3_stmt *st;
    if (sqlite3_prepare_v2(g_db,
            "SELECT date, steps FROM schedule ORDER BY date DESC LIMIT 1;",
            -1, &st, NULL) == SQLITE_OK) {
        if (sqlite3_step(st) == SQLITE_ROW) {
            snprintf(last_date, sizeof last_date, "%s", (const char *)sqlite3_column_text(st, 0));
            last_steps = sqlite3_column_int(st, 1);
        }
        sqlite3_finalize(st);
    }

    /* Базовая дата: завтра после последней записи, либо сегодня */
    time_t base;
    int base_steps;
    if (last_date[0]) {
        struct tm t;
        memset(&t, 0, sizeof t);
        int y, m, d;
        sscanf(last_date, "%d-%d-%d", &y, &m, &d);
        t.tm_year = y - 1900;
        t.tm_mon = m - 1;
        t.tm_mday = d + 1;
        t.tm_isdst = -1;
        base = mktime(&t);
        base_steps = last_steps + increment;
    } else {
        base = time(NULL);
        base_steps = start_steps;
    }

    if (sqlite3_prepare_v2(g_db,
            "INSERT OR IGNORE INTO schedule (date, steps, done) VALUES (?, ?, 0);",
            -1, &st, NULL) != SQLITE_OK)
        return -1;

    int added = 0;
    for (int i = 0; i < days; i++) {
        time_t t = base + (time_t)i * 24 * 60 * 60;
        struct tm *lt = localtime(&t);
        char date[11];
        format_date(lt->tm_year + 1900, lt->tm_mon + 1, lt->tm_mday, date);

        sqlite3_reset(st);
        sqlite3_bind_text(st, 1, date, -1, SQLITE_TRANSIENT);
        sqlite3_bind_int(st, 2, base_steps + i * increment);
        if (sqlite3_step(st) == SQLITE_DONE && sqlite3_changes(g_db) > 0)
            added++;
    }
    sqlite3_finalize(st);
    return added;
}

int get_schedule(struct Entry *out, int max) {
    if (!g_db) return -1;

    sqlite3_stmt *st;
    if (sqlite3_prepare_v2(g_db,
            "SELECT id, date, steps, done FROM schedule ORDER BY date;",
            -1, &st, NULL) != SQLITE_OK)
        return -1;

    int count = 0;
    while (count < max && sqlite3_step(st) == SQLITE_ROW) {
        out[count].id = sqlite3_column_int(st, 0);
        snprintf(out[count].date, sizeof out[count].date, "%s",
                 (const char *)sqlite3_column_text(st, 1));
        out[count].steps = sqlite3_column_int(st, 2);
        out[count].done = sqlite3_column_int(st, 3);
        count++;
    }
    sqlite3_finalize(st);
    return count;
}

int set_done(int id, int done) {
    if (!g_db) return -1;

    sqlite3_stmt *st;
    if (sqlite3_prepare_v2(g_db,
            "UPDATE schedule SET done = ? WHERE id = ?;", -1, &st, NULL) != SQLITE_OK)
        return -1;

    sqlite3_bind_int(st, 1, done ? 1 : 0);
    sqlite3_bind_int(st, 2, id);
    int rc = sqlite3_step(st);
    int changes = sqlite3_changes(g_db);
    sqlite3_finalize(st);
    if (rc != SQLITE_DONE) return -1;
    return changes > 0 ? 0 : 1;
}

int delete_entry(int id) {
    if (!g_db) return -1;

    sqlite3_stmt *st;
    if (sqlite3_prepare_v2(g_db,
            "DELETE FROM schedule WHERE id = ?;", -1, &st, NULL) != SQLITE_OK)
        return -1;

    sqlite3_bind_int(st, 1, id);
    int rc = sqlite3_step(st);
    int changes = sqlite3_changes(g_db);
    sqlite3_finalize(st);
    if (rc != SQLITE_DONE) return -1;
    return changes > 0 ? 0 : 1;
}

int get_today_entry(struct Entry *out) {
    if (!g_db) return -1;

    time_t now = time(NULL);
    struct tm *lt = localtime(&now);
    char today[11];
    format_date(lt->tm_year + 1900, lt->tm_mon + 1, lt->tm_mday, today);

    sqlite3_stmt *st;
    if (sqlite3_prepare_v2(g_db,
            "SELECT id, date, steps, done FROM schedule WHERE date = ?;",
            -1, &st, NULL) != SQLITE_OK)
        return -1;

    sqlite3_bind_text(st, 1, today, -1, SQLITE_TRANSIENT);
    int rc = sqlite3_step(st);
    if (rc == SQLITE_ROW) {
        out->id = sqlite3_column_int(st, 0);
        snprintf(out->date, sizeof out->date, "%s", (const char *)sqlite3_column_text(st, 1));
        out->steps = sqlite3_column_int(st, 2);
        out->done = sqlite3_column_int(st, 3);
        sqlite3_finalize(st);
        return 0;
    }
    sqlite3_finalize(st);
    return 1;
}

int print_schedule(void) {
    struct Entry rows[4096];
    int count = get_schedule(rows, 4096);
    if (count < 0) return -1;

    printf("+------+------------+-------+----------------+\n");
    printf("|  ID  |    Дата    | Шаги  | Статус         |\n");
    printf("+------+------------+-------+----------------+\n");
    for (int i = 0; i < count; i++) {
        printf("| %4d | %s | %5d | %-14s |\n",
               rows[i].id, rows[i].date, rows[i].steps,
               rows[i].done ? "выполнено" : "не выполнено");
    }
    printf("+------+------------+-------+----------------+\n");
    if (count == 0) {
        printf("Расписание пусто. Сгенерируйте его (пункт 1).\n");
    } else {
        printf("Всего записей: %d\n", count);
    }
    return 0;
}
