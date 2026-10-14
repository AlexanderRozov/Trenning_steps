#ifndef TREND_DB_H
#define TREND_DB_H

struct Entry {
    int id;
    char date[11]; /* YYYY-MM-DD */
    int steps;
    int done; /* 0 = не выполнено, 1 = выполнено */
};

int db_open(const char *path);
void db_close(void);

/* Настройки (таблица settings) */
int get_setting_int(const char *key, int fallback);
void get_setting_str(const char *key, char *out, int out_size, const char *fallback);
int set_setting(const char *key, const char *value);
int set_setting_int(const char *key, int value);

/* Расписание тренировок (таблица schedule) */
int generate_schedule(int days);
int get_schedule(struct Entry *out, int max);
int set_done(int id, int done);
int delete_entry(int id);
int get_today_entry(struct Entry *out); /* 0 = найдена, 1 = нет, -1 = ошибка */
int print_schedule(void);

#endif
