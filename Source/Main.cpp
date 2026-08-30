#include <windows.h>
#include <dwmapi.h>

namespace
{
    constexpr COLORREF background = RGB(5, 8, 14);
    constexpr COLORREF panel = RGB(12, 17, 27);
    constexpr COLORREF border = RGB(42, 50, 68);
    constexpr COLORREF text = RGB(244, 246, 252);
    constexpr COLORREF muted = RGB(150, 160, 180);
    constexpr COLORREF purple = RGB(161, 72, 255);
    constexpr COLORREF teal = RGB(40, 209, 204);

    void fillRect(HDC dc, const RECT& rect, COLORREF colour)
    {
        HBRUSH brush = CreateSolidBrush(colour);
        FillRect(dc, &rect, brush);
        DeleteObject(brush);
    }

    void drawPanel(HDC dc, int left, int top, int right, int bottom)
    {
        HPEN pen = CreatePen(PS_SOLID, 1, border);
        HBRUSH brush = CreateSolidBrush(panel);
        auto oldPen = SelectObject(dc, pen);
        auto oldBrush = SelectObject(dc, brush);
        RoundRect(dc, left, top, right, bottom, 18, 18);
        SelectObject(dc, oldBrush);
        SelectObject(dc, oldPen);
        DeleteObject(brush);
        DeleteObject(pen);
    }

    void drawTextLine(HDC dc, const wchar_t* value, int x, int y, int size, COLORREF colour, int weight = FW_NORMAL)
    {
        HFONT font = CreateFontW(-size, 0, 0, 0, weight, FALSE, FALSE, FALSE,
                                 DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                                 CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
        auto oldFont = SelectObject(dc, font);
        SetBkMode(dc, TRANSPARENT);
        SetTextColor(dc, colour);
        RECT r { x, y, x + 900, y + size + 16 };
        DrawTextW(dc, value, -1, &r, DT_LEFT | DT_SINGLELINE | DT_VCENTER);
        SelectObject(dc, oldFont);
        DeleteObject(font);
    }

    LRESULT CALLBACK windowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
    {
        switch (message)
        {
            case WM_CREATE:
            {
                BOOL dark = TRUE;
                DwmSetWindowAttribute(hwnd, 20, &dark, sizeof(dark));
                return 0;
            }

            case WM_GETMINMAXINFO:
            {
                auto* info = reinterpret_cast<MINMAXINFO*>(lParam);
                info->ptMinTrackSize.x = 900;
                info->ptMinTrackSize.y = 560;
                return 0;
            }

            case WM_ERASEBKGND:
                return 1;

            case WM_PAINT:
            {
                PAINTSTRUCT ps {};
                HDC dc = BeginPaint(hwnd, &ps);

                RECT client {};
                GetClientRect(hwnd, &client);
                fillRect(dc, client, background);

                drawTextLine(dc, L"ANONYMOUS", 42, 34, 30, text, FW_BOLD);
                drawTextLine(dc, L"VOICE", 238, 34, 30, purple, FW_BOLD);
                drawTextLine(dc, L"Native Windows baseline", 44, 78, 15, muted);

                drawPanel(dc, 40, 126, client.right - 40, 280);
                drawTextLine(dc, L"UI FOUNDATION ONLINE", 68, 154, 18, teal, FW_BOLD);
                drawTextLine(dc, L"AnonymousVoice now starts without the JUCE GUI memory bottleneck.", 68, 194, 17, text);
                drawTextLine(dc, L"Audio/DSP is temporarily isolated while the Windows-native shell is stabilized.", 68, 226, 15, muted);

                drawPanel(dc, 40, 304, (client.right / 2) - 12, 470);
                drawTextLine(dc, L"VOICE ENGINE", 68, 334, 17, text, FW_BOLD);
                drawTextLine(dc, L"Status: isolated for hotfix", 68, 374, 15, muted);
                drawTextLine(dc, L"Next: safe WASAPI input/output", 68, 408, 15, purple);

                drawPanel(dc, (client.right / 2) + 12, 304, client.right - 40, 470);
                drawTextLine(dc, L"VIRTUAL MICROPHONE", (client.right / 2) + 40, 334, 17, text, FW_BOLD);
                drawTextLine(dc, L"Status: transport pending", (client.right / 2) + 40, 374, 15, muted);
                drawTextLine(dc, L"Target: Xbox Party / Discord capture endpoint", (client.right / 2) + 40, 408, 15, teal);

                drawTextLine(dc, L"v0.2.0-hotfix", 42, client.bottom - 54, 14, muted);
                drawTextLine(dc, L"STABLE STARTUP BASELINE", client.right - 300, client.bottom - 54, 14, teal, FW_BOLD);

                EndPaint(hwnd, &ps);
                return 0;
            }

            case WM_CLOSE:
                DestroyWindow(hwnd);
                return 0;

            case WM_DESTROY:
                PostQuitMessage(0);
                return 0;

            default:
                return DefWindowProcW(hwnd, message, wParam, lParam);
        }
    }
}

int WINAPI wWinMain(HINSTANCE instance, HINSTANCE, PWSTR, int showCommand)
{
    SetProcessDPIAware();

    const wchar_t* className = L"AnonymousVoiceMainWindow";

    WNDCLASSEXW wc {};
    wc.cbSize = sizeof(wc);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = windowProc;
    wc.hInstance = instance;
    wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    wc.hbrBackground = nullptr;
    wc.lpszClassName = className;

    if (RegisterClassExW(&wc) == 0)
        return 1;

    HWND window = CreateWindowExW(
        0,
        className,
        L"Anonymous Voice",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        1280,
        720,
        nullptr,
        nullptr,
        instance,
        nullptr);

    if (window == nullptr)
        return 2;

    ShowWindow(window, showCommand == 0 ? SW_SHOW : showCommand);
    UpdateWindow(window);

    MSG message {};
    while (GetMessageW(&message, nullptr, 0, 0) > 0)
    {
        TranslateMessage(&message);
        DispatchMessageW(&message);
    }

    return static_cast<int>(message.wParam);
}
