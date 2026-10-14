#define WIN32_LEAN_AND_MEAN
#include <windows.h>

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

#define WINDOW_WIDTH 560
#define WINDOW_HEIGHT 520
#define MAX_ROWS 1024

static LRESULT CALLBACK
WindowProc(HWND wnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    switch (msg)
    {
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }

    if (nk_gdi_handle_event(wnd, msg, wparam, lparam))
        return 0;

    return DefWindowProcW(wnd, msg, wparam, lparam);
}

int run_gui(void)
{
    GdiFont* font;
    struct nk_context *ctx;

    WNDCLASSW wc;
    RECT rect = { 0, 0, WINDOW_WIDTH, WINDOW_HEIGHT };
    DWORD style = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX;
    DWORD exstyle = WS_EX_APPWINDOW;
    HWND wnd;
    HDC dc;
    int running = 1;
    int needs_refresh = 1;

    static struct DateRow rows[MAX_ROWS];
    int row_count = 0;
    int needs_reload = 1;

    time_t now = time(NULL);
    struct tm *tm_now = localtime(&now);
    static int day, month, year;
    day = tm_now->tm_mday;
    month = tm_now->tm_mon + 1;
    year = tm_now->tm_year + 1900;

    char status[128] = "";

    /* Win32 */
    memset(&wc, 0, sizeof(wc));
    wc.style = CS_DBLCLKS;
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = GetModuleHandleW(0);
    wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.lpszClassName = L"TrendWindowClass";
    RegisterClassW(&wc);

    AdjustWindowRectEx(&rect, style, FALSE, exstyle);
    wnd = CreateWindowExW(exstyle, wc.lpszClassName, L"tREND — календарь дат",
        style | WS_VISIBLE, CW_USEDEFAULT, CW_USEDEFAULT,
        rect.right - rect.left, rect.bottom - rect.top,
        NULL, NULL, wc.hInstance, NULL);
    dc = GetDC(wnd);

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

        if (needs_reload) {
            row_count = get_all_dates(rows, MAX_ROWS);
            if (row_count < 0) {
                row_count = 0;
                snprintf(status, sizeof status, "Ошибка чтения базы данных");
            }
            needs_reload = 0;
        }

        /* GUI */
        if (nk_begin(ctx, "tREND", nk_rect(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT),
            NK_WINDOW_NO_SCROLLBAR))
        {
            nk_layout_row_dynamic(ctx, 24, 1);
            nk_label(ctx, "Новая дата", NK_TEXT_LEFT);

            nk_layout_row_dynamic(ctx, 34, 3);
            nk_property_int(ctx, "День", 1, &day, 31, 1, 1);
            nk_property_int(ctx, "Месяц", 1, &month, 12, 1, 1);
            nk_property_int(ctx, "Год", 1, &year, 9999, 1, 1);

            nk_layout_row_dynamic(ctx, 34, 1);
            if (nk_button_label(ctx, "Добавить")) {
                struct Date d = { day, month, year };
                if (save_date(d) == 0) {
                    snprintf(status, sizeof status,
                        "Дата %02d.%02d.%04d сохранена", d.day, d.month, d.year);
                    needs_reload = 1;
                } else {
                    snprintf(status, sizeof status, "Ошибка сохранения");
                }
            }

            if (status[0]) {
                nk_layout_row_dynamic(ctx, 22, 1);
                nk_label(ctx, status, NK_TEXT_LEFT);
            }

            nk_layout_row_dynamic(ctx, 24, 1);
            nk_label(ctx, "Сохранённые даты", NK_TEXT_LEFT);

            nk_layout_row_dynamic(ctx, 26, 3);
            nk_label(ctx, "ID", NK_TEXT_LEFT);
            nk_label(ctx, "Дата", NK_TEXT_LEFT);
            nk_label(ctx, "", NK_TEXT_LEFT);

            for (int i = 0; i < row_count; i++) {
                char id_buf[16], date_buf[24];
                snprintf(id_buf, sizeof id_buf, "%d", rows[i].id);
                snprintf(date_buf, sizeof date_buf, "%02d.%02d.%04d",
                    rows[i].date.day, rows[i].date.month, rows[i].date.year);

                nk_layout_row_dynamic(ctx, 26, 3);
                nk_label(ctx, id_buf, NK_TEXT_LEFT);
                nk_label(ctx, date_buf, NK_TEXT_LEFT);
                if (nk_button_label(ctx, "Удалить")) {
                    if (delete_date(rows[i].id) == 0) {
                        snprintf(status, sizeof status,
                            "Запись %d удалена", rows[i].id);
                        needs_reload = 1;
                    } else {
                        snprintf(status, sizeof status, "Ошибка удаления");
                    }
                }
            }

            if (row_count == 0) {
                nk_layout_row_dynamic(ctx, 22, 1);
                nk_label(ctx, "Записей пока нет", NK_TEXT_LEFT);
            }
        }
        nk_end(ctx);

        /* Draw */
        nk_gdi_render(nk_rgb(30, 30, 30));
    }

    nk_gdifont_del(font);
    ReleaseDC(wnd, dc);
    UnregisterClassW(wc.lpszClassName, wc.hInstance);
    return 0;
}
