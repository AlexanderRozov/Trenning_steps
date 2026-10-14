#ifndef TREND_DB_H
#define TREND_DB_H

struct Date {
    int day;
    int month;
    int year;
};

int db_open(const char *path);
void db_close(void);

int save_date(struct Date d);
int delete_date(int id);
int print_all_dates(void);

#endif
