#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shellapi.h>

#include <stdio.h>
#include <string.h>
#include <time.h>

#define NK_INCLUDE_FIXED_TYPES
#define NK_INCLUDE_STANDARD_IO
#define NK_INCLUDE_STANDARD_VARARGS
#define NK_INCLUDE_DEFAULT_ALLOCATOR
#define NK_IMPLEMENTATION
#define NK_GDI_IMPLEMENTATION
#include "nuklear.h"
#include "nuklear_gdi.h"

#include "db.h"
#include "gui.h"
#include "lang.h"

#define WINDOW_WIDTH 640
#define WINDOW_HEIGHT 680
#define MAX_ROWS 4096

#define WM_TRAYICON (WM_APP + 1)
#define AUTOSTART_KEY "Software\\Microsoft\\Windows\\CurrentVersion\\Run"
#define AUTOSTART_NAME "tREND"

static NOTIFYICONDATAW g_nid;
static int g_tray_added = 0;
static HWND g_wnd = NULL;

static void utf8_to_wide(const char *src, wchar_t *dst, int dst_len) {
    MultiByteToWideChar(CP_UTF8, 0, src, -1, dst, dst_len);
}

/* ------------------- Автозагрузка ------------------- */

static int autostart_is_enabled(void) {
    HKEY key;
    if (RegOpenKeyExA(HKEY_CURRENT_USER, AUTOSTART_KEY,
                      0, KEY_QUERY_VALUE, &key) != ERROR_SUCCESS)
        return 0;
    char buf[512];
    DWORD size = sizeof buf;
    LONG rc = RegQueryValueExA(key, AUTOSTART_NAME, NULL, NULL, (LPBYTE)buf, &size);
    RegCloseKey(key);
    return rc == ERROR_SUCCESS;
}

static int autostart_set(int enable) {
    HKEY key;
    if (RegOpenKeyExA(HKEY_CURRENT_USER, AUTOSTART_KEY,
                      0, KEY_SET_VALUE, &key) != ERROR_SUCCESS)
        return -1;

    int rc = 0;
    if (enable) {
        char path[MAX_PATH];
        GetModuleFileNameA(NULL, path, MAX_PATH);
        char cmd[MAX_PATH + 2];
        snprintf(cmd, sizeof cmd, "\"%s\"", path);
        if (RegSetValueExA(key, AUTOSTART_NAME, 0, REG_SZ,
                           (const BYTE *)cmd, (DWORD)strlen(cmd) + 1) != ERROR_SUCCESS)
            rc = -1;
    } else {
        if (RegDeleteValueA(key, AUTOSTART_NAME) != ERROR_SUCCESS)
            rc = -1;
    }
    RegCloseKey(key);
    return rc;
}

/* ------------------- Трей и оповещения ------------------- */

static void tray_add(HWND wnd) {
    memset(&g_nid, 0, sizeof g_nid);
    g_nid.cbSize = sizeof g_nid;
    g_nid.hWnd = wnd;
    g_nid.uID = 1;
    g_nid.uFlags = NIF_MESSAGE | NIF_ICON | NIF_TIP;
    g_nid.uCallbackMessage = WM_TRAYICON;
    g_nid.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    utf8_to_wide(tr(S_APP_TITLE), g_nid.szTip, 128);
    g_tray_added = Shell_NotifyIconW(NIM_ADD, &g_nid) ? 1 : 0;
}

static void tray_remove(void) {
    if (g_tray_added) {
        Shell_NotifyIconW(NIM_DELETE, &g_nid);
        g_tray_added = 0;
    }
}

static void tray_notify(int title_id, const char *text_u8) {
    if (!g_tray_added) return;
    g_nid.uFlags = NIF_INFO;
    utf8_to_wide(tr(title_id), g_nid.szInfoTitle, 64);
    utf8_to_wide(text_u8, g_nid.szInfo, 256);
    g_nid.dwInfoFlags = NIIF_INFO;
    g_nid.uTimeout = 10000;
    Shell_NotifyIconW(NIM_MODIFY, &g_nid);
}

static int days_since(const char *date) {
    if (!date[0]) return 1000000;
    int y, m, d;
    if (sscanf(date, "%d-%d-%d", &y, &m, &d) != 3) return 1000000;
    struct tm t;
    memset(&t, 0, sizeof t);
    t.tm_year = y - 1900;
    t.tm_mon = m - 1;
    t.tm_mday = d;
    t.tm_hour = 12;
    t.tm_isdst = -1;
    time_t then = mktime(&t);
    time_t now = time(NULL);
    return (int)((now - then) / (24 * 60 * 60));
}

static void check_reminder(void) {
    char last[32];
    get_setting_str("last_notify", last, sizeof last, "");

    time_t now = time(NULL);
    struct tm *lt = localtime(&now);
    char today[11];
    snprintf(today, sizeof today, "%04d-%02d-%02d",
             lt->tm_year + 1900, lt->tm_mon + 1, lt->tm_mday);

    int hour = get_setting_int("notify_hour", 9);
    int minute = get_setting_int("notify_minute", 0);
    int period = get_setting_int("notify_period_days", 1);

    if (lt->tm_hour < hour || (lt->tm_hour == hour && lt->tm_min < minute))
        return;
    if (days_since(last) < period)
        return;

    struct Entry e;
    if (get_today_entry(&e) == 0 && !e.done) {
        char text[256];
        snprintf(text, sizeof text, tr(S_NOTIFY_TEXT), e.steps);
        tray_notify(S_NOTIFY_TITLE, text);
        set_setting("last_notify", today);
    }
}

/* ------------------- Окно ------------------- */

static LRESULT CALLBACK
WindowProc(HWND wnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    switch (msg)
    {
    case WM_DESTROY:
        tray_remove();
        PostQuitMessage(0);
        return 0;
    }

    if (nk_gdi_handle_event(wnd, msg, wparam, lparam))
        return 0;

    return DefWindowProcW(wnd, msg, wparam, lparam);
}

static void update_window_title(void) {
    wchar_t title[128];
    utf8_to_wide(tr(S_APP_TITLE), title, 128);
    SetWindowTextW(g_wnd, title);
}

int run_gui(void)
{
    GdiFont* font;
    struct nk_context *ctx;

    WNDCLASSW wc;
    RECT rect = { 0, 0, WINDOW_WIDTH, WINDOW_HEIGHT };
    DWORD style = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX;
    DWORD exstyle = WS_EX_APPWINDOW;
    HDC dc;
    int running = 1;
    int needs_refresh = 1;

    static struct Entry rows[MAX_ROWS];
    int row_count = 0;
    int needs_reload = 1;
    char status[256] = "";

    /* Настройки из БД */
    static int start_steps, increment, notify_hour, notify_minute, notify_period;
    start_steps = get_setting_int("start_steps", 5000);
    increment = get_setting_int("step_increment", 500);
    notify_hour = get_setting_int("notify_hour", 9);
    notify_minute = get_setting_int("notify_minute", 0);
    notify_period = get_setting_int("notify_period_days", 1);
    static int autostart;
    autostart = autostart_is_enabled();
    static int lang_idx;
    lang_idx = lang_get();

    const char *lang_names_list[LANG_COUNT];
    for (int i = 0; i < LANG_COUNT; i++)
        lang_names_list[i] = lang_name(i);

    /* Win32 */
    memset(&wc, 0, sizeof(wc));
    wc.style = CS_DBLCLKS;
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = GetModuleHandleW(0);
    wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.lpszClassName = L"TrendWindowClass";
    RegisterClassW(&wc);

    wchar_t title[128];
    utf8_to_wide(tr(S_APP_TITLE), title, 128);

    AdjustWindowRectEx(&rect, style, FALSE, exstyle);
    g_wnd = CreateWindowExW(exstyle, wc.lpszClassName, title,
        style | WS_VISIBLE, CW_USEDEFAULT, CW_USEDEFAULT,
        rect.right - rect.left, rect.bottom - rect.top,
        NULL, NULL, wc.hInstance, NULL);
    dc = GetDC(g_wnd);
    tray_add(g_wnd);

    /* GUI */
    font = nk_gdifont_create("Arial", 18);
    ctx = nk_gdi_init(font, dc, WINDOW_WIDTH, WINDOW_HEIGHT);

    while (running)
    {
        /* Input */
        MSG msg;
        nk_input_begin(ctx);
        if (needs_refresh == 0) {
            if (GetMessageW(&msg, NULL, 0, 0) <= 0) {
                running = 0;
            } else {
                TranslateMessage(&msg);
                DispatchMessageW(&msg);
            }
            needs_refresh = 1;
        } else needs_refresh = 0;

        while (PeekMessageW(&msg, NULL, 0, 0, PM_REMOVE)) {
            if (msg.message == WM_QUIT)
                running = 0;
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
            needs_refresh = 1;
        }
        nk_input_end(ctx);

        check_reminder();

        if (needs_reload) {
            row_count = get_schedule(rows, MAX_ROWS);
            if (row_count < 0) {
                row_count = 0;
                snprintf(status, sizeof status, "%s", tr(S_MSG_READ_ERROR));
            }
            needs_reload = 0;
        }

        /* GUI */
        if (nk_begin(ctx, "tREND", nk_rect(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT), 0))
        {
            /* Сегодня */
            struct Entry today;
            nk_layout_row_dynamic(ctx, 24, 1);
            nk_label(ctx, tr(S_TODAY), NK_TEXT_LEFT);
            if (get_today_entry(&today) == 0) {
                char line[160];
                snprintf(line, sizeof line, tr(S_TODAY_LINE),
                         today.date, today.steps,
                         tr(today.done ? S_DONE : S_NOT_DONE));
                nk_layout_row_dynamic(ctx, 26, 1);
                nk_label(ctx, line, NK_TEXT_LEFT);
                if (!today.done) {
                    nk_layout_row_dynamic(ctx, 32, 1);
                    if (nk_button_label(ctx, tr(S_MARK_DONE))) {
                        if (set_done(today.id, 1) == 0) {
                            snprintf(status, sizeof status, "%s", tr(S_MSG_TRAINING_DONE));
                            needs_reload = 1;
                        }
                    }
                }
            } else {
                nk_layout_row_dynamic(ctx, 26, 1);
                nk_label(ctx, tr(S_NO_TODAY), NK_TEXT_LEFT);
            }

            /* Расписание */
            nk_layout_row_dynamic(ctx, 24, 1);
            nk_label(ctx, tr(S_SCHEDULE), NK_TEXT_LEFT);

            nk_layout_row_dynamic(ctx, 32, 2);
            if (nk_button_label(ctx, tr(S_GENERATE))) {
                int added = generate_schedule(7);
                if (added >= 0) {
                    snprintf(status, sizeof status, tr(S_MSG_ADDED), added);
                    needs_reload = 1;
                } else {
                    snprintf(status, sizeof status, "%s", tr(S_MSG_GEN_ERROR));
                }
            }
            if (nk_button_label(ctx, tr(S_REFRESH))) {
                needs_reload = 1;
            }

            nk_layout_row_dynamic(ctx, 26, 5);
            nk_label(ctx, tr(S_COL_DATE), NK_TEXT_LEFT);
            nk_label(ctx, tr(S_COL_STEPS), NK_TEXT_LEFT);
            nk_label(ctx, tr(S_COL_STATUS), NK_TEXT_LEFT);
            nk_label(ctx, "", NK_TEXT_LEFT);
            nk_label(ctx, "", NK_TEXT_LEFT);

            for (int i = 0; i < row_count; i++) {
                char steps_buf[24];
                snprintf(steps_buf, sizeof steps_buf, "%d", rows[i].steps);

                nk_layout_row_dynamic(ctx, 28, 5);
                nk_label(ctx, rows[i].date, NK_TEXT_LEFT);
                nk_label(ctx, steps_buf, NK_TEXT_LEFT);
                nk_label(ctx, tr(rows[i].done ? S_DONE : S_NOT_DONE), NK_TEXT_LEFT);
                if (!rows[i].done) {
                    if (nk_button_label(ctx, tr(S_BTN_DONE))) {
                        set_done(rows[i].id, 1);
                        snprintf(status, sizeof status, tr(S_MSG_MARKED_DONE), rows[i].date);
                        needs_reload = 1;
                    }
                } else {
                    if (nk_button_label(ctx, tr(S_BTN_RESET))) {
                        set_done(rows[i].id, 0);
                        snprintf(status, sizeof status, tr(S_MSG_MARKED_UNDONE), rows[i].date);
                        needs_reload = 1;
                    }
                }
                if (nk_button_label(ctx, tr(S_BTN_DELETE))) {
                    delete_entry(rows[i].id);
                    snprintf(status, sizeof status, tr(S_MSG_DELETED), rows[i].date);
                    needs_reload = 1;
                }
            }

            /* Настройки */
            nk_layout_row_dynamic(ctx, 24, 1);
            nk_label(ctx, tr(S_SETTINGS), NK_TEXT_LEFT);

            int prev;

            nk_layout_row_dynamic(ctx, 30, 2);
            prev = start_steps;
            nk_property_int(ctx, tr(S_START_STEPS), 500, &start_steps, 100000, 500, 10);
            if (start_steps != prev) set_setting_int("start_steps", start_steps);
            prev = increment;
            nk_property_int(ctx, tr(S_INCREMENT), 0, &increment, 10000, 100, 10);
            if (increment != prev) set_setting_int("step_increment", increment);

            prev = notify_hour;
            nk_property_int(ctx, tr(S_NOTIFY_HOUR), 0, &notify_hour, 23, 1, 1);
            if (notify_hour != prev) set_setting_int("notify_hour", notify_hour);
            prev = notify_minute;
            nk_property_int(ctx, tr(S_NOTIFY_MINUTE), 0, &notify_minute, 59, 1, 1);
            if (notify_minute != prev) set_setting_int("notify_minute", notify_minute);

            nk_layout_row_dynamic(ctx, 30, 1);
            prev = notify_period;
            nk_property_int(ctx, tr(S_NOTIFY_PERIOD), 1, &notify_period, 30, 1, 1);
            if (notify_period != prev) set_setting_int("notify_period_days", notify_period);

            nk_layout_row_dynamic(ctx, 24, 1);
            nk_label(ctx, tr(S_LANGUAGE), NK_TEXT_LEFT);
            nk_layout_row_dynamic(ctx, 30, 1);
            int new_lang = nk_combo(ctx, lang_names_list, LANG_COUNT, lang_idx,
                                    25, nk_vec2(nk_widget_width(ctx), 220));
            if (new_lang != lang_idx) {
                lang_idx = new_lang;
                lang_set(lang_idx);
                set_setting("language", lang_code(lang_idx));
                update_window_title();
                status[0] = '\0';
            }

            int prev_auto = autostart;
            nk_layout_row_dynamic(ctx, 26, 1);
            nk_checkbox_label(ctx, tr(S_AUTOSTART), &autostart);
            if (autostart != prev_auto) {
                if (autostart_set(autostart) == 0) {
                    snprintf(status, sizeof status, "%s",
                             tr(autostart ? S_MSG_AUTOSTART_ON : S_MSG_AUTOSTART_OFF));
                } else {
                    autostart = prev_auto;
                    snprintf(status, sizeof status, "%s", tr(S_MSG_AUTOSTART_ERROR));
                }
            }

            if (status[0]) {
                nk_layout_row_dynamic(ctx, 22, 1);
                nk_label(ctx, status, NK_TEXT_LEFT);
            }
        }
        nk_end(ctx);

        /* Draw */
        nk_gdi_render(nk_rgb(30, 30, 30));
    }

    tray_remove();
    nk_gdifont_del(font);
    ReleaseDC(g_wnd, dc);
    UnregisterClassW(wc.lpszClassName, wc.hInstance);
    return 0;
}
