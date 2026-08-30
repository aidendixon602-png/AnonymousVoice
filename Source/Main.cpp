#include <windows.h>
#include <dwmapi.h>
#include <array>
#include <algorithm>

namespace
{
    constexpr COLORREF bg = RGB(5, 8, 14);
    constexpr COLORREF sidebarBg = RGB(8, 12, 20);
    constexpr COLORREF panel = RGB(12, 17, 27);
    constexpr COLORREF panelRaised = RGB(17, 23, 35);
    constexpr COLORREF panelHover = RGB(23, 30, 45);
    constexpr COLORREF border = RGB(42, 50, 68);
    constexpr COLORREF borderSoft = RGB(30, 37, 52);
    constexpr COLORREF text = RGB(244, 246, 252);
    constexpr COLORREF muted = RGB(146, 157, 178);
    constexpr COLORREF dim = RGB(92, 103, 124);
    constexpr COLORREF purple = RGB(161, 72, 255);
    constexpr COLORREF purpleDark = RGB(45, 27, 70);
    constexpr COLORREF teal = RGB(40, 209, 204);
    constexpr COLORREF green = RGB(45, 214, 119);
    constexpr COLORREF orange = RGB(255, 161, 69);

    struct Box
    {
        int x = 0;
        int y = 0;
        int w = 0;
        int h = 0;

        RECT rect() const { return RECT { x, y, x + w, y + h }; }
        bool contains(int px, int py) const
        {
            return px >= x && px < x + w && py >= y && py < y + h;
        }
    };

    struct VoicePreset
    {
        const wchar_t* name;
        const wchar_t* glyph;
        const wchar_t* tag;
    };

    constexpr std::array<VoicePreset, 10> voices {{
        { L"Anonymous", L"A", L"STEALTH" },
        { L"Deep",      L"D", L"LOW" },
        { L"Demon",     L"X", L"DARK" },
        { L"Robot",     L"R", L"CYBER" },
        { L"Alien",     L"O", L"SCI-FI" },
        { L"Ethereal",  L"E", L"AIRY" },
        { L"Baby",      L"B", L"HIGH" },
        { L"Radio",     L"Q", L"COMMS" },
        { L"Witch",     L"W", L"ARCANE" },
        { L"Custom",    L"+", L"NEW" }
    }};

    constexpr std::array<const wchar_t*, 9> effects {{
        L"GATE", L"PITCH", L"FORMANT", L"EQ", L"COMP", L"DRIVE", L"CHORUS", L"REVERB", L"LIMITER"
    }};

    std::array<Box, 10> voiceHitBoxes {};
    int selectedVoice = 0;
    int selectedNav = 0;
    bool monitorEnabled = true;

    void fill(HDC dc, const Box& b, COLORREF colour)
    {
        RECT r = b.rect();
        HBRUSH brush = CreateSolidBrush(colour);
        FillRect(dc, &r, brush);
        DeleteObject(brush);
    }

    void fillRectRaw(HDC dc, const RECT& r, COLORREF colour)
    {
        HBRUSH brush = CreateSolidBrush(colour);
        FillRect(dc, &r, brush);
        DeleteObject(brush);
    }

    void roundedPanel(HDC dc, const Box& b, COLORREF fillColour, COLORREF lineColour, int radius = 16, int lineWidth = 1)
    {
        HPEN pen = CreatePen(PS_SOLID, lineWidth, lineColour);
        HBRUSH brush = CreateSolidBrush(fillColour);
        HGDIOBJ oldPen = SelectObject(dc, pen);
        HGDIOBJ oldBrush = SelectObject(dc, brush);
        RoundRect(dc, b.x, b.y, b.x + b.w, b.y + b.h, radius, radius);
        SelectObject(dc, oldBrush);
        SelectObject(dc, oldPen);
        DeleteObject(brush);
        DeleteObject(pen);
    }

    void line(HDC dc, int x1, int y1, int x2, int y2, COLORREF colour, int width = 1)
    {
        HPEN pen = CreatePen(PS_SOLID, width, colour);
        HGDIOBJ old = SelectObject(dc, pen);
        MoveToEx(dc, x1, y1, nullptr);
        LineTo(dc, x2, y2);
        SelectObject(dc, old);
        DeleteObject(pen);
    }

    void textLine(HDC dc, const wchar_t* value, const Box& b, int size, COLORREF colour,
                  int weight = FW_NORMAL, UINT format = DT_LEFT | DT_SINGLELINE | DT_VCENTER)
    {
        HFONT font = CreateFontW(-size, 0, 0, 0, weight, FALSE, FALSE, FALSE,
                                 DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                                 CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
        HGDIOBJ old = SelectObject(dc, font);
        SetBkMode(dc, TRANSPARENT);
        SetTextColor(dc, colour);
        RECT r = b.rect();
        DrawTextW(dc, value, -1, &r, format);
        SelectObject(dc, old);
        DeleteObject(font);
    }

    void textWrap(HDC dc, const wchar_t* value, const Box& b, int size, COLORREF colour)
    {
        textLine(dc, value, b, size, colour, FW_NORMAL, DT_LEFT | DT_TOP | DT_WORDBREAK);
    }

    void circle(HDC dc, int cx, int cy, int radius, COLORREF fillColour, COLORREF lineColour, int width = 1)
    {
        HPEN pen = CreatePen(PS_SOLID, width, lineColour);
        HBRUSH brush = CreateSolidBrush(fillColour);
        HGDIOBJ oldPen = SelectObject(dc, pen);
        HGDIOBJ oldBrush = SelectObject(dc, brush);
        Ellipse(dc, cx - radius, cy - radius, cx + radius, cy + radius);
        SelectObject(dc, oldBrush);
        SelectObject(dc, oldPen);
        DeleteObject(brush);
        DeleteObject(pen);
    }

    void drawSlider(HDC dc, const Box& b, const wchar_t* label, const wchar_t* value, float amount)
    {
        textLine(dc, label, Box { b.x, b.y, b.w / 2, 20 }, 13, muted, FW_SEMIBOLD);
        textLine(dc, value, Box { b.x + b.w / 2, b.y, b.w / 2, 20 }, 13, text, FW_SEMIBOLD, DT_RIGHT | DT_SINGLELINE | DT_VCENTER);

        const int trackY = b.y + 31;
        roundedPanel(dc, Box { b.x, trackY, b.w, 6 }, borderSoft, borderSoft, 6);
        const int fillWidth = std::max(8, static_cast<int>(b.w * std::clamp(amount, 0.0f, 1.0f)));
        roundedPanel(dc, Box { b.x, trackY, fillWidth, 6 }, purple, purple, 6);
        circle(dc, b.x + fillWidth, trackY + 3, 7, text, purple, 1);
    }

    void drawKnob(HDC dc, int cx, int cy, int radius, float amount)
    {
        circle(dc, cx, cy, radius, RGB(9, 13, 21), border, 2);
        const int startX = cx;
        const int startY = cy - radius + 5;
        const int endX = cx + static_cast<int>((amount - 0.5f) * radius * 1.45f);
        const int endY = cy - static_cast<int>(radius * 0.48f);
        line(dc, startX, startY, endX, endY, purple, 3);
        circle(dc, cx, cy, radius - 9, panelRaised, borderSoft, 1);
    }

    void drawToggle(HDC dc, const Box& b, bool on)
    {
        const COLORREF c = on ? purple : border;
        roundedPanel(dc, b, on ? purpleDark : panelRaised, c, b.h);
        const int radius = (b.h - 6) / 2;
        const int cx = on ? b.x + b.w - radius - 3 : b.x + radius + 3;
        circle(dc, cx, b.y + b.h / 2, radius, on ? text : muted, c, 1);
    }

    void drawSidebar(HDC dc, int height)
    {
        const Box side { 14, 14, 204, height - 52 };
        roundedPanel(dc, side, sidebarBg, borderSoft, 20);

        circle(dc, 48, 53, 18, purpleDark, purple, 2);
        textLine(dc, L"A", Box { 34, 37, 28, 30 }, 16, text, FW_BOLD, DT_CENTER | DT_SINGLELINE | DT_VCENTER);
        textLine(dc, L"ANONYMOUS", Box { 76, 30, 122, 28 }, 20, text, FW_BOLD);
        textLine(dc, L"VOICE CHANGER", Box { 76, 55, 122, 20 }, 11, muted, FW_SEMIBOLD);

        line(dc, 34, 91, 196, 91, borderSoft);

        constexpr std::array<const wchar_t*, 5> nav {{ L"VOICEBOX", L"SOUNDBOARD", L"VOICE LAB", L"KEYBINDS", L"SETTINGS" }};
        constexpr std::array<const wchar_t*, 5> glyphs {{ L"V", L"S", L"L", L"K", L"G" }};

        for (int i = 0; i < 5; ++i)
        {
            const int y = 112 + i * 58;
            if (i == selectedNav)
            {
                roundedPanel(dc, Box { 29, y, 174, 44 }, purpleDark, purpleDark, 10);
                fill(dc, Box { 29, y + 8, 3, 28 }, purple);
            }

            circle(dc, 50, y + 22, 12, i == selectedNav ? RGB(36, 24, 55) : panelRaised,
                   i == selectedNav ? purple : borderSoft, 1);
            textLine(dc, glyphs[i], Box { 41, y + 12, 18, 20 }, 10, i == selectedNav ? text : muted, FW_BOLD,
                     DT_CENTER | DT_SINGLELINE | DT_VCENTER);
            textLine(dc, nav[i], Box { 76, y, 116, 44 }, 12, i == selectedNav ? text : muted, FW_SEMIBOLD);
        }

        const int userY = height - 124;
        line(dc, 34, userY - 16, 196, userY - 16, borderSoft);
        circle(dc, 50, userY + 13, 9, RGB(13, 48, 34), green, 1);
        textLine(dc, L"Anonymous", Box { 72, userY - 2, 118, 22 }, 13, text, FW_SEMIBOLD);
        textLine(dc, L"ONLINE", Box { 72, userY + 20, 118, 18 }, 10, green, FW_BOLD);
    }

    void drawVoiceBrowser(HDC dc, const Box& p)
    {
        roundedPanel(dc, p, panel, borderSoft, 16);
        textLine(dc, L"VOICE PRESETS", Box { p.x + 20, p.y + 15, 210, 26 }, 17, text, FW_BOLD);
        textLine(dc, L"10 VOICES", Box { p.x + p.w - 104, p.y + 15, 82, 26 }, 10, muted, FW_BOLD, DT_RIGHT | DT_SINGLELINE | DT_VCENTER);

        const Box search { p.x + 20, p.y + 52, p.w - 184, 36 };
        roundedPanel(dc, search, bg, border, 9);
        textLine(dc, L"Search voices...", Box { search.x + 14, search.y, search.w - 20, search.h }, 12, dim);

        const Box category { p.x + p.w - 154, p.y + 52, 134, 36 };
        roundedPanel(dc, category, bg, border, 9);
        textLine(dc, L"ALL  ▾", Box { category.x + 12, category.y, category.w - 24, category.h }, 11, muted, FW_SEMIBOLD,
                 DT_CENTER | DT_SINGLELINE | DT_VCENTER);

        const int gridX = p.x + 20;
        const int gridY = p.y + 103;
        const int gridW = p.w - 40;
        const int gap = 9;
        const int cols = 5;
        const int rows = 2;
        const int cardW = (gridW - gap * (cols - 1)) / cols;
        const int availableH = p.y + p.h - 18 - gridY;
        const int cardH = std::max(74, (availableH - gap) / rows);

        for (int i = 0; i < 10; ++i)
        {
            const int row = i / cols;
            const int col = i % cols;
            const Box card { gridX + col * (cardW + gap), gridY + row * (cardH + gap), cardW, cardH };
            voiceHitBoxes[static_cast<size_t>(i)] = card;
            const bool selected = i == selectedVoice;

            roundedPanel(dc, card, selected ? RGB(26, 18, 45) : panelRaised,
                         selected ? purple : borderSoft, 12, selected ? 2 : 1);

            circle(dc, card.x + card.w / 2, card.y + 31, 17,
                   selected ? RGB(40, 26, 63) : RGB(11, 16, 25), selected ? purple : border, 1);
            textLine(dc, voices[static_cast<size_t>(i)].glyph,
                     Box { card.x + card.w / 2 - 16, card.y + 15, 32, 32 }, 14,
                     selected ? purple : muted, FW_BOLD, DT_CENTER | DT_SINGLELINE | DT_VCENTER);

            textLine(dc, voices[static_cast<size_t>(i)].name,
                     Box { card.x + 6, card.y + 53, card.w - 12, 22 }, 11, text, FW_SEMIBOLD,
                     DT_CENTER | DT_SINGLELINE | DT_VCENTER);
            textLine(dc, voices[static_cast<size_t>(i)].tag,
                     Box { card.x + 6, card.y + card.h - 23, card.w - 12, 16 }, 8,
                     selected ? purple : dim, FW_BOLD, DT_CENTER | DT_SINGLELINE | DT_VCENTER);
        }
    }

    void drawPresetDetails(HDC dc, const Box& p)
    {
        roundedPanel(dc, p, panel, borderSoft, 16);
        textLine(dc, L"PRESET DETAILS", Box { p.x + 20, p.y + 15, p.w - 40, 26 }, 17, text, FW_BOLD);

        const auto& voice = voices[static_cast<size_t>(selectedVoice)];
        textLine(dc, voice.name, Box { p.x + 20, p.y + 50, p.w - 150, 28 }, 18, purple, FW_BOLD);
        textWrap(dc, L"Real-time voice profile controls. Audio processing will be connected after the native UI shell is stable.",
                 Box { p.x + 20, p.y + 84, p.w - 158, 54 }, 11, muted);

        const int visualX = p.x + p.w - 78;
        const int visualY = p.y + 91;
        circle(dc, visualX, visualY, 43, RGB(24, 16, 39), purple, 2);
        circle(dc, visualX, visualY, 34, RGB(9, 13, 21), borderSoft, 1);
        textLine(dc, voice.glyph, Box { visualX - 30, visualY - 30, 60, 60 }, 26, text, FW_BOLD,
                 DT_CENTER | DT_SINGLELINE | DT_VCENTER);

        const int sliderTop = p.y + 152;
        drawSlider(dc, Box { p.x + 20, sliderTop, p.w - 40, 45 }, L"PITCH", L"-5.0 st", 0.30f);
        drawSlider(dc, Box { p.x + 20, sliderTop + 56, p.w - 40, 45 }, L"FORMANT", L"-3.0", 0.38f);
        drawSlider(dc, Box { p.x + 20, sliderTop + 112, p.w - 40, 45 }, L"MIX", L"100%", 0.96f);

        const int buttonY = p.y + p.h - 53;
        roundedPanel(dc, Box { p.x + 20, buttonY, 132, 34 }, purpleDark, purple, 9);
        textLine(dc, L"TEST VOICE", Box { p.x + 20, buttonY, 132, 34 }, 10, text, FW_BOLD,
                 DT_CENTER | DT_SINGLELINE | DT_VCENTER);
        textLine(dc, L"DSP OFFLINE", Box { p.x + p.w - 118, buttonY, 98, 34 }, 9, orange, FW_BOLD,
                 DT_RIGHT | DT_SINGLELINE | DT_VCENTER);
    }

    void drawEditor(HDC dc, const Box& p)
    {
        roundedPanel(dc, p, panel, borderSoft, 16);
        textLine(dc, L"VOICE EDITOR", Box { p.x + 18, p.y + 12, 190, 24 }, 16, text, FW_BOLD);
        textLine(dc, L"EFFECT CHAIN", Box { p.x + p.w - 135, p.y + 12, 115, 24 }, 9, muted, FW_BOLD,
                 DT_RIGHT | DT_SINGLELINE | DT_VCENTER);

        const int gap = 7;
        const int x0 = p.x + 16;
        const int y = p.y + 45;
        const int available = p.w - 32;
        const int cardW = (available - gap * 8) / 9;
        const int cardH = p.h - 59;

        for (int i = 0; i < 9; ++i)
        {
            const Box c { x0 + i * (cardW + gap), y, cardW, cardH };
            roundedPanel(dc, c, panelRaised, borderSoft, 10);
            textLine(dc, effects[static_cast<size_t>(i)], Box { c.x + 4, c.y + 7, c.w - 8, 19 }, 8, text, FW_BOLD,
                     DT_CENTER | DT_SINGLELINE | DT_VCENTER);

            const float amount = 0.18f + static_cast<float>((i * 13) % 62) / 100.0f;
            drawKnob(dc, c.x + c.w / 2, c.y + c.h / 2, std::min(23, std::max(15, c.w / 4)), amount);

            drawToggle(dc, Box { c.x + c.w / 2 - 15, c.y + c.h - 27, 30, 16 }, i != 2 && i != 3);
        }
    }

    void drawDeviceBox(HDC dc, const Box& b, const wchar_t* label, const wchar_t* device, COLORREF accent)
    {
        textLine(dc, label, Box { b.x, b.y, b.w, 18 }, 9, muted, FW_BOLD);
        const Box chooser { b.x, b.y + 22, b.w, 34 };
        roundedPanel(dc, chooser, bg, border, 8);
        circle(dc, chooser.x + 17, chooser.y + 17, 5, accent, accent, 1);
        textLine(dc, device, Box { chooser.x + 30, chooser.y, chooser.w - 48, chooser.h }, 10, text, FW_SEMIBOLD);
        textLine(dc, L"▾", Box { chooser.x + chooser.w - 23, chooser.y, 16, chooser.h }, 10, muted, FW_BOLD,
                 DT_CENTER | DT_SINGLELINE | DT_VCENTER);
    }

    void drawIo(HDC dc, const Box& p)
    {
        roundedPanel(dc, p, panel, borderSoft, 16);
        const int gap = 18;
        const int innerX = p.x + 20;
        const int innerW = p.w - 40;
        const int third = (innerW - gap * 2) / 3;

        const Box input { innerX, p.y + 17, third, p.h - 30 };
        const Box monitor { innerX + third + gap, p.y + 17, third, p.h - 30 };
        const Box output { innerX + (third + gap) * 2, p.y + 17, third, p.h - 30 };

        drawDeviceBox(dc, input, L"AUDIO INPUT", L"Default Microphone", teal);
        drawDeviceBox(dc, output, L"VIRTUAL OUTPUT", L"Anonymous Virtual Mic", purple);

        textLine(dc, L"LIVE MONITOR", Box { monitor.x, monitor.y, monitor.w - 50, 18 }, 9, muted, FW_BOLD);
        drawToggle(dc, Box { monitor.x + monitor.w - 42, monitor.y, 42, 20 }, monitorEnabled);
        roundedPanel(dc, Box { monitor.x, monitor.y + 29, monitor.w, 7 }, borderSoft, borderSoft, 7);
        roundedPanel(dc, Box { monitor.x, monitor.y + 29, static_cast<int>(monitor.w * 0.72), 7 }, purple, purple, 7);
        textLine(dc, L"72%", Box { monitor.x, monitor.y + 43, monitor.w, 18 }, 9, muted, FW_SEMIBOLD,
                 DT_RIGHT | DT_SINGLELINE | DT_VCENTER);
    }

    void drawFooter(HDC dc, int width, int height)
    {
        const int y = height - 33;
        circle(dc, 32, y + 8, 4, green, green, 1);
        textLine(dc, L"UI ONLINE", Box { 44, y - 2, 100, 20 }, 9, green, FW_BOLD);
        textLine(dc, L"CPU: --", Box { 159, y - 2, 80, 20 }, 9, muted, FW_SEMIBOLD);
        textLine(dc, L"LATENCY: --", Box { 245, y - 2, 110, 20 }, 9, muted, FW_SEMIBOLD);
        textLine(dc, L"v0.3.0-native", Box { width - 150, y - 2, 122, 20 }, 9, muted, FW_SEMIBOLD,
                 DT_RIGHT | DT_SINGLELINE | DT_VCENTER);
    }

    void paintDashboard(HWND hwnd, HDC target)
    {
        RECT client {};
        GetClientRect(hwnd, &client);
        const int width = client.right;
        const int height = client.bottom;

        HDC dc = CreateCompatibleDC(target);
        HBITMAP bitmap = CreateCompatibleBitmap(target, std::max(1, width), std::max(1, height));
        HGDIOBJ oldBitmap = SelectObject(dc, bitmap);

        fillRectRaw(dc, client, bg);

        drawSidebar(dc, height);

        const int contentX = 232;
        const int right = width - 16;
        const int contentW = std::max(740, right - contentX);

        textLine(dc, L"VOICEBOX", Box { contentX + 6, 22, 220, 31 }, 23, text, FW_BOLD);
        textLine(dc, L"Shape your voice. Stay anonymous.", Box { contentX + 7, 51, 330, 20 }, 11, muted);

        roundedPanel(dc, Box { right - 188, 26, 170, 31 }, RGB(12, 31, 28), RGB(25, 86, 64), 9);
        circle(dc, right - 168, 41, 5, green, green, 1);
        textLine(dc, L"PROTECTION ACTIVE", Box { right - 155, 26, 134, 31 }, 9, green, FW_BOLD,
                 DT_CENTER | DT_SINGLELINE | DT_VCENTER);

        const int footerTop = height - 38;
        const int ioH = 94;
        const int ioY = footerTop - ioH - 8;
        const int editorH = std::clamp(height / 4, 190, 245);
        const int editorY = ioY - editorH - 10;
        const int topY = 86;
        const int topH = std::max(250, editorY - topY - 10);

        const int splitGap = 10;
        const int voiceW = static_cast<int>(contentW * 0.58f);
        const Box voicePanel { contentX, topY, voiceW - splitGap / 2, topH };
        const Box detailPanel { contentX + voiceW + splitGap / 2, topY,
                                contentW - voiceW - splitGap / 2, topH };
        const Box editorPanel { contentX, editorY, contentW, editorH };
        const Box ioPanel { contentX, ioY, contentW, ioH };

        drawVoiceBrowser(dc, voicePanel);
        drawPresetDetails(dc, detailPanel);
        drawEditor(dc, editorPanel);
        drawIo(dc, ioPanel);
        drawFooter(dc, width, height);

        BitBlt(target, 0, 0, width, height, dc, 0, 0, SRCCOPY);

        SelectObject(dc, oldBitmap);
        DeleteObject(bitmap);
        DeleteDC(dc);
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
                info->ptMinTrackSize.x = 1180;
                info->ptMinTrackSize.y = 720;
                return 0;
            }

            case WM_ERASEBKGND:
                return 1;

            case WM_LBUTTONDOWN:
            {
                const int x = static_cast<short>(LOWORD(lParam));
                const int y = static_cast<short>(HIWORD(lParam));

                for (int i = 0; i < 10; ++i)
                {
                    if (voiceHitBoxes[static_cast<size_t>(i)].contains(x, y))
                    {
                        selectedVoice = i;
                        InvalidateRect(hwnd, nullptr, FALSE);
                        return 0;
                    }
                }

                for (int i = 0; i < 5; ++i)
                {
                    const Box nav { 29, 112 + i * 58, 174, 44 };
                    if (nav.contains(x, y))
                    {
                        selectedNav = i;
                        InvalidateRect(hwnd, nullptr, FALSE);
                        return 0;
                    }
                }

                return 0;
            }

            case WM_PAINT:
            {
                PAINTSTRUCT ps {};
                HDC dc = BeginPaint(hwnd, &ps);
                paintDashboard(hwnd, dc);
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
    wc.style = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
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
        1440,
        900,
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
