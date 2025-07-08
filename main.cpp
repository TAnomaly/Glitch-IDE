#include "TextEditor.h"

// Pencere sınıfı adı
const char *WINDOW_CLASS = "Glitch";

// Global editör instance
ModernTextEditor *g_editor = nullptr;

// Window procedure
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_CREATE:
        g_editor = new ModernTextEditor();
        g_editor->setHwnd(hwnd);
        g_editor->handleResize();
        break;

    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);

        RECT client_rect;
        GetClientRect(hwnd, &client_rect);

        HDC mem_dc = CreateCompatibleDC(hdc);
        HBITMAP mem_bitmap = CreateCompatibleBitmap(hdc, client_rect.right, client_rect.bottom);
        SelectObject(mem_dc, mem_bitmap);

        if (g_editor)
        {
            g_editor->paint(mem_dc);
        }

        BitBlt(hdc, 0, 0, client_rect.right, client_rect.bottom, mem_dc, 0, 0, SRCCOPY);

        DeleteObject(mem_bitmap);
        DeleteDC(mem_dc);

        EndPaint(hwnd, &ps);
    }
    break;

    case WM_ERASEBKGND:
    {
        RECT client_rect;
        GetClientRect(hwnd, &client_rect);
        HBRUSH black_brush = CreateSolidBrush(RGB(0, 0, 0));
        FillRect((HDC)wParam, &client_rect, black_brush);
        DeleteObject(black_brush);
        return TRUE;
    }

    case WM_LBUTTONDOWN:
        if (g_editor)
        {
            g_editor->handleMouseClick(LOWORD(lParam), HIWORD(lParam));
        }
        break;

    case WM_KEYDOWN:
        if (g_editor)
        {
            g_editor->handleKeyPress(wParam);
        }
        break;

    case WM_CHAR:
        if (g_editor)
        {
            g_editor->handleChar(wParam);
        }
        break;

    case WM_SIZE:
        if (g_editor)
        {
            g_editor->handleResize();
        }
        break;

    case WM_DESTROY:
        delete g_editor;
        g_editor = nullptr;
        PostQuitMessage(0);
        break;

    default:
        return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    return 0;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    // Window class kaydet
    WNDCLASS wc = {0};
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.hbrBackground = CreateSolidBrush(RGB(0, 0, 0)); // Siyah arka plan
    wc.lpszClassName = WINDOW_CLASS;
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.hCursor = LoadCursor(NULL, IDC_IBEAM);
    wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);

    if (!RegisterClass(&wc))
    {
        MessageBox(NULL, "Window class registration failed!", "Error", MB_OK);
        return 1;
    }

    // Pencere oluştur
    HWND hwnd = CreateWindow(
        WINDOW_CLASS,
        "Glitch IDE - Modern Text Editor",
        WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN,
        CW_USEDEFAULT, CW_USEDEFAULT,
        1200, 800,
        NULL, NULL, hInstance, NULL);

    if (!hwnd)
    {
        MessageBox(NULL, "Window creation failed!", "Error", MB_OK);
        return 1;
    }

    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);

    // Mesaj döngüsü
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return msg.wParam;
}
