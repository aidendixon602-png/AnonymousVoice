#include <windows.h>
#include <dwmapi.h>
#include <array>
#include <algorithm>
#include <cmath>
#include <cwchar>

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
    constexpr COLORREF purpleBright = RGB(190, 103, 255);
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
        const wchar_t* tag;
    };

    struct PresetState
    {
        float pitch;
        float formant;
        float mix;
    };

    constexpr std::array<VoicePreset, 10> voices {{
        { L"Anonymous", L"STEALTH" },
        { L"Deep",      L"LOW" },
        { L"Demon",     L"DARK" },
        { L"Robot",     L"CYBER" },
        { L"Alien",     L"SCI-FI" },
        { L"Ethereal",  L"AIRY" },
        { L"Baby",      L"HIGH" },
        { L"Radio",     L"COMMS" },
        { L"Witch",     L"ARCANE" },
        { L"Custom",    L"NEW" }
    }};

    std::array<PresetState, 10> presetStates {{
        { -5.0f, -3.0f, 1.00f },
        { -7.0f, -2.0f, 1.00f },
        { -4.0f, -6.0f, 0.94f },
        { -1.0f, -1.0f, 0.88f },
        {  3.0f,  5.0f, 0.92f },
        {  4.0f,  3.0f, 0.82f },
        {  7.0f,  5.0f, 1.00f },
        { -1.0f, -2.0f, 0.78f },
        { -2.0f,  2.0f, 0.90f },
        {  0.0f,  0.0f, 1.00f }
    }};

    constexpr std::array<const wchar_t*, 9> effects {{
        L"GATE", L"PITCH", L"FORMANT", L"EQ", L"COMP", L"DRIVE", L"CHORUS", L"REVERB", L"LIMITER"
    }};

    std::array<bool, 9> effectEnabled {{ true, true, false, false, true, true, true, true, true }};
    std::array<float, 9> effectAmount {{ 0.24f, 0.38f, 0.34f, 0.42f, 0.48f, 0.31f, 0.25f, 0.18f, 0.56f }};

    std::array<Box, 10> voiceHitBoxes {};
    std::array<Box, 5> navHitBoxes {};
    std::array<Box, 3> sliderHitBoxes {};
    std::array<Box, 9> effectHitBoxes {};
    Box monitorHitBox {};
    Box testVoiceHitBox {};

    int selectedVoice = 0;
    int selectedNav = 0;
    int hoveredVoice = -1;
    int hoveredNav = -1;
    int hoveredSlider = -1;
    int hoveredEffect = -1;
    int draggingSlider = -1;
    bool monitorEnabled = true;
    bool trackingMouse = false;
    bool testVoicePressed = false;
    HICON appIcon = nullptr;

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

    void drawChevronDown(HDC dc, int cx, int cy, COLORREF colour)
    {
        line(dc, cx - 4, cy - 2, cx, cy + 2, colour, 2);
        line(dc, cx, cy + 2, cx + 4, cy - 2, colour, 2);
    }

    void drawToggle(HDC dc, const Box& b, bool on, bool hovered = false)
    {
        const COLORREF outline = on ? (hovered ? purpleBright : purple) : (hovered ? muted : border);
        roundedPanel(dc, b, on ? purpleDark : panelRaised, outline, b.h);
        const int radius = (b.h - 6) / 2;
        const int cx = on ? b.x + b.w - radius - 3 : b.x + radius + 3;
        circle(dc, cx, b.y + b.h / 2, radius, on ? text : muted, outline, 1);
    }

    void drawSlider(HDC dc, const Box& b, const wchar_t* label, const wchar_t* value, float amount, bool hovered)
    {
        textLine(dc, label, Box { b.x, b.y, b.w / 2, 20 }, 13, muted, FW_SEMIBOLD);
        textLine(dc, value, Box { b.x + b.w / 2, b.y, b.w / 2, 20 }, 13, text, FW_SEMIBOLD,
                 DT_RIGHT | DT_SINGLELINE | DT_VCENTER);

        const int trackY = b.y + 31;
        roundedPanel(dc, Box { b.x, trackY, b.w, 6 }, borderSoft, borderSoft, 6);
        const int fillWidth = std::max(4, static_cast<int>(b.w * std::clamp(amount, 0.0f, 1.0f)));
        roundedPanel(dc, Box { b.x, trackY, fillWidth, 6 }, hovered ? purpleBright : purple, hovered ? purpleBright : purple, 6);
        circle(dc, b.x + fillWidth, trackY + 3, hovered ? 8 : 7, text, hovered ? purpleBright : purple, 1);
    }

    void drawKnob(HDC dc, int cx, int cy, int radius, float amount, bool enabled, bool hovered)
    {
        const COLORREF ring = hovered ? muted : border;
        circle(dc, cx, cy, radius, RGB(9, 13, 21), ring, 2);

        const float angle = (-135.0f + std::clamp(amount, 0.0f, 1.0f) * 270.0f) * 3.14159265f / 180.0f;
        const int innerR = radius - 6;
        const int endX = cx + static_cast<int>(std::cos(angle) * innerR);
        const int endY = cy + static_cast<int>(std::sin(angle) * innerR);
        line(dc, cx, cy, endX, endY, enabled ? purple : dim, 3);
        circle(dc, cx, cy, radius - 9, panelRaised, borderSoft, 1);
        circle(dc, cx, cy, 3, enabled ? purple : dim, enabled ? purple : dim, 1);
    }

    void drawVoiceIcon(HDC dc, int index, int cx, int cy, bool selected, bool hovered)
    {
        const COLORREF accent = selected ? purpleBright : (hovered ? teal : muted);
        const COLORREF iconBg = selected ? RGB(40, 26, 63) : RGB(10, 15, 24);
        circle(dc, cx, cy, 20, iconBg, selected ? purple : (hovered ? teal : border), selected ? 2 : 1);

        switch (index)
        {
            case 0:
                line(dc, cx - 9, cy - 3, cx, cy - 11, accent, 2);
                line(dc, cx, cy - 11, cx + 9, cy - 3, accent, 2);
                line(dc, cx - 9, cy - 3, cx - 6, cy + 9, accent, 2);
                line(dc, cx + 9, cy - 3, cx + 6, cy + 9, accent, 2);
                line(dc, cx - 6, cy + 9, cx + 6, cy + 9, accent, 2);
                line(dc, cx - 4, cy + 2, cx - 1, cy + 2, accent, 2);
                line(dc, cx + 1, cy + 2, cx + 4, cy + 2, accent, 2);
                break;
            case 1:
                line(dc, cx - 8, cy - 8, cx, cy, accent, 2);
                line(dc, cx, cy, cx + 8, cy - 8, accent, 2);
                line(dc, cx - 8, cy, cx, cy + 8, accent, 2);
                line(dc, cx, cy + 8, cx + 8, cy, accent, 2);
                break;
            case 2:
                line(dc, cx - 10, cy - 9, cx - 4, cy - 2, accent, 2);
                line(dc, cx + 10, cy - 9, cx + 4, cy - 2, accent, 2);
                circle(dc, cx, cy + 2, 9, iconBg, accent, 2);
                line(dc, cx - 4, cy + 1, cx - 1, cy + 1, accent, 2);
                line(dc, cx + 1, cy + 1, cx + 4, cy + 1, accent, 2);
                break;
            case 3:
            {
                HPEN pen = CreatePen(PS_SOLID, 2, accent);
                HBRUSH brush = GetStockBrush(NULL_BRUSH);
                HGDIOBJ oldPen = SelectObject(dc, pen);
                HGDIOBJ oldBrush = SelectObject(dc, brush);
                Rectangle(dc, cx - 10, cy - 9, cx + 10, cy + 9);
                SelectObject(dc, oldBrush);
                SelectObject(dc, oldPen);
                DeleteObject(pen);
                circle(dc, cx - 4, cy - 1, 2, accent, accent, 1);
                circle(dc, cx + 4, cy - 1, 2, accent, accent, 1);
                line(dc, cx - 5, cy + 5, cx + 5, cy + 5, accent, 1);
                break;
            }
            case 4:
                circle(dc, cx, cy, 10, iconBg, accent, 2);
                circle(dc, cx - 4, cy - 1, 3, accent, accent, 1);
                circle(dc, cx + 4, cy - 1, 3, accent, accent, 1);
                break;
            case 5:
                line(dc, cx, cy - 11, cx, cy + 11, accent, 2);
                line(dc, cx - 11, cy, cx + 11, cy, accent, 2);
                line(dc, cx - 7, cy - 7, cx + 7, cy + 7, accent, 2);
                line(dc, cx + 7, cy - 7, cx - 7, cy + 7, accent, 2);
                break;
            case 6:
                circle(dc, cx, cy, 10, iconBg, accent, 2);
                circle(dc, cx - 4, cy - 2, 2, accent, accent, 1);
                circle(dc, cx + 4, cy - 2, 2, accent, accent, 1);
                line(dc, cx - 4, cy + 5, cx, cy + 7, accent, 1);
                line(dc, cx, cy + 7, cx + 4, cy + 5, accent, 1);
                break;
            case 7:
                line(dc, cx - 10, cy + 8, cx - 10, cy + 3, accent, 2);
                line(dc, cx - 4, cy + 8, cx - 4, cy - 2, accent, 2);
                line(dc, cx + 2, cy + 8, cx + 2, cy - 7, accent, 2);
                line(dc, cx + 8, cy + 8, cx + 8, cy - 11, accent, 2);
                break;
            case 8:
                circle(dc, cx - 2, cy, 10, iconBg, accent, 2);
                circle(dc, cx + 4, cy - 3, 10, iconBg, iconBg, 2);
                break;
            default:
                line(dc, cx - 8, cy, cx + 8, cy, accent, 2);
                line(dc, cx, cy - 8, cx, cy + 8, accent, 2);
                break;
        }
    }

    void drawSidebar(HDC dc, int height)
    {
        const Box side { 14, 14, 218, height - 52 };
        roundedPanel(dc, side, sidebarBg, borderSoft, 20);

        circle(dc, 48, 53, 18, purpleDark, purple, 2);
        textLine(dc, L"A", Box { 34, 37, 28, 30 }, 16, text, FW_BOLD, DT_CENTER | DT_SINGLELINE | DT_VCENTER);
        textLine(dc, L"ANONYMOUS", Box { 76, 29, 142, 27 }, 17, text, FW_BOLD);
        textLine(dc, L"VOICE CHANGER", Box { 76, 54, 142, 20 }, 10, muted, FW_SEMIBOLD);

        line(dc, 34, 91, 211, 91, borderSoft);

        constexpr std::array<const wchar_t*, 5> nav {{ L"VOICEBOX", L"SOUNDBOARD", L"VOICE LAB", L"KEYBINDS", L"SETTINGS" }};
        constexpr std::array<const wchar_t*, 5> glyphs {{ L"V", L"S", L"L", L"K", L"G" }};

        for (int i = 0; i < 5; ++i)
        {
            const int y = 112 + i * 58;
            const Box navBox { 29, y, 188, 44 };
            navHitBoxes[static_cast<size_t>(i)] = navBox;
            const bool selected = i == selectedNav;
            const bool hovered = i == hoveredNav;

            if (selected || hovered)
            {
                roundedPanel(dc, navBox, selected ? purpleDark : panelHover,
                             selected ? purpleDark : borderSoft, 10);
                if (selected)
                    fill(dc, Box { 29, y + 8, 3, 28 }, purple);
            }

            circle(dc, 50, y + 22, 12,
                   selected ? RGB(36, 24, 55) : panelRaised,
                   selected ? purple : (hovered ? teal : borderSoft), 1);
            textLine(dc, glyphs[static_cast<size_t>(i)], Box { 41, y + 12, 18, 20 }, 10,
                     selected ? text : (hovered ? teal : muted), FW_BOLD,
                     DT_CENTER | DT_SINGLELINE | DT_VCENTER);
            textLine(dc, nav[static_cast<size_t>(i)], Box { 76, y, 126, 44 }, 12,
                     selected ? text : (hovered ? text : muted), FW_SEMIBOLD);
        }

        const int userY = height - 124;
        line(dc, 34, userY - 16, 211, userY - 16, borderSoft);
        circle(dc, 50, userY + 13, 9, RGB(13, 48, 34), green, 1);
        textLine(dc, L"Anonymous", Box { 72, userY - 2, 132, 22 }, 13, text, FW_SEMIBOLD);
        textLine(dc, L"ONLINE", Box { 72, userY + 20, 132, 18 }, 10, green, FW_BOLD);
    }

    void drawVoiceBrowser(HDC dc, const Box& p)
    {
        roundedPanel(dc, p, panel, borderSoft, 16);
        textLine(dc, L"VOICE PRESETS", Box { p.x + 20, p.y + 15, 210, 26 }, 17, text, FW_BOLD);
        textLine(dc, L"10 VOICES", Box { p.x + p.w - 104, p.y + 15, 82, 26 }, 10, muted, FW_BOLD,
                 DT_RIGHT | DT_SINGLELINE | DT_VCENTER);

        const Box search { p.x + 20, p.y + 52, p.w - 184, 36 };
        roundedPanel(dc, search, bg, border, 9);
        textLine(dc, L"Search voices...", Box { search.x + 14, search.y, search.w - 20, search.h }, 12, dim);

        const Box category { p.x + p.w - 154, p.y + 52, 134, 36 };
        roundedPanel(dc, category, bg, border, 9);
        textLine(dc, L"ALL", Box { category.x + 12, category.y, category.w - 38, category.h }, 11, muted, FW_SEMIBOLD,
                 DT_CENTER | DT_SINGLELINE | DT_VCENTER);
        drawChevronDown(dc, category.x + category.w - 21, category.y + category.h / 2, muted);

        const int gridX = p.x + 20;
        const int gridY = p.y + 103;
        const int gridW = p.w - 40;
        const int gap = 9;
        const int cols = 5;
        const int cardW = (gridW - gap * (cols - 1)) / cols;
        const int availableH = p.y + p.h - 18 - gridY;
        const int cardH = std::max(74, (availableH - gap) / 2);

        for (int i = 0; i < 10; ++i)
        {
            const int row = i / cols;
            const int col = i % cols;
            const Box card { gridX + col * (cardW + gap), gridY + row * (cardH + gap), cardW, cardH };
            voiceHitBoxes[static_cast<size_t>(i)] = card;
            const bool selected = i == selectedVoice;
            const bool hovered = i == hoveredVoice;

            roundedPanel(dc, card,
                         selected ? RGB(26, 18, 45) : (hovered ? panelHover : panelRaised),
                         selected ? purple : (hovered ? RGB(56, 68, 88) : borderSoft),
                         12, selected ? 2 : 1);

            drawVoiceIcon(dc, i, card.x + card.w / 2, card.y + 34, selected, hovered);

            textLine(dc, voices[static_cast<size_t>(i)].name,
                     Box { card.x + 6, card.y + 58, card.w - 12, 22 }, 11, text, FW_SEMIBOLD,
                     DT_CENTER | DT_SINGLELINE | DT_VCENTER);
            textLine(dc, voices[static_cast<size_t>(i)].tag,
                     Box { card.x + 6, card.y + card.h - 23, card.w - 12, 16 }, 8,
                     selected ? purpleBright : (hovered ? teal : dim), FW_BOLD,
                     DT_CENTER | DT_SINGLELINE | DT_VCENTER);
        }
    }

    void drawPresetDetails(HDC dc, const Box& p)
    {
        roundedPanel(dc, p, panel, borderSoft, 16);
        textLine(dc, L"PRESET DETAILS", Box { p.x + 20, p.y + 15, p.w - 40, 26 }, 17, text, FW_BOLD);

        const auto& voice = voices[static_cast<size_t>(selectedVoice)];
        const auto& state = presetStates[static_cast<size_t>(selectedVoice)];
        textLine(dc, voice.name, Box { p.x + 20, p.y + 50, p.w - 150, 28 }, 18, purpleBright, FW_BOLD);
        textWrap(dc, L"Preset controls are live in the UI. WASAPI capture and DSP processing are the next engine milestone.",
                 Box { p.x + 20, p.y + 84, p.w - 158, 54 }, 11, muted);

        const int visualX = p.x + p.w - 78;
        const int visualY = p.y + 91;
        circle(dc, visualX, visualY, 43, RGB(24, 16, 39), purple, 2);
        circle(dc, visualX, visualY, 34, RGB(9, 13, 21), borderSoft, 1);
        drawVoiceIcon(dc, selectedVoice, visualX, visualY, true, false);

        wchar_t pitchText[32] {};
        wchar_t formantText[32] {};
        wchar_t mixText[32] {};
        swprintf_s(pitchText, L"%+.1f st", state.pitch);
        swprintf_s(formantText, L"%+.1f", state.formant);
        swprintf_s(mixText, L"%.0f%%", state.mix * 100.0f);

        const int sliderTop = p.y + 152;
        sliderHitBoxes[0] = Box { p.x + 20, sliderTop, p.w - 40, 45 };
        sliderHitBoxes[1] = Box { p.x + 20, sliderTop + 56, p.w - 40, 45 };
        sliderHitBoxes[2] = Box { p.x + 20, sliderTop + 112, p.w - 40, 45 };

        drawSlider(dc, sliderHitBoxes[0], L"PITCH", pitchText, (state.pitch + 12.0f) / 24.0f, hoveredSlider == 0 || draggingSlider == 0);
        drawSlider(dc, sliderHitBoxes[1], L"FORMANT", formantText, (state.formant + 10.0f) / 20.0f, hoveredSlider == 1 || draggingSlider == 1);
        drawSlider(dc, sliderHitBoxes[2], L"MIX", mixText, state.mix, hoveredSlider == 2 || draggingSlider == 2);

        const int buttonY = p.y + p.h - 53;
        testVoiceHitBox = Box { p.x + 20, buttonY, 132, 34 };
        roundedPanel(dc, testVoiceHitBox, testVoicePressed ? RGB(60, 34, 91) : purpleDark,
                     testVoicePressed ? purpleBright : purple, 9);
        textLine(dc, L"TEST VOICE", testVoiceHitBox, 10, text, FW_BOLD,
                 DT_CENTER | DT_SINGLELINE | DT_VCENTER);
        textLine(dc, testVoicePressed ? L"ENGINE NEXT" : L"DSP OFFLINE",
                 Box { p.x + p.w - 118, buttonY, 98, 34 }, 9,
                 testVoicePressed ? teal : orange, FW_BOLD,
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
            effectHitBoxes[static_cast<size_t>(i)] = c;
            const bool hovered = hoveredEffect == i;
            const bool enabled = effectEnabled[static_cast<size_t>(i)];

            roundedPanel(dc, c, hovered ? panelHover : panelRaised,
                         enabled ? (hovered ? RGB(76, 62, 102) : borderSoft) : borderSoft, 10);
            textLine(dc, effects[static_cast<size_t>(i)], Box { c.x + 4, c.y + 7, c.w - 8, 19 }, 8,
                     enabled ? text : dim, FW_BOLD, DT_CENTER | DT_SINGLELINE | DT_VCENTER);

            drawKnob(dc, c.x + c.w / 2, c.y + c.h / 2, std::min(23, std::max(15, c.w / 4)),
                     effectAmount[static_cast<size_t>(i)], enabled, hovered);

            const Box toggle { c.x + c.w / 2 - 15, c.y + c.h - 27, 30, 16 };
            drawToggle(dc, toggle, enabled, hovered);
        }
    }

    void drawDeviceBox(HDC dc, const Box& b, const wchar_t* label, const wchar_t* device, COLORREF accent)
    {
        textLine(dc, label, Box { b.x, b.y, b.w, 18 }, 9, muted, FW_BOLD);
        const Box chooser { b.x, b.y + 22, b.w, 34 };
        roundedPanel(dc, chooser, bg, border, 8);
        circle(dc, chooser.x + 17, chooser.y + 17, 5, accent, accent, 1);
        textLine(dc, device, Box { chooser.x + 30, chooser.y, chooser.w - 55, chooser.h }, 10, text, FW_SEMIBOLD);
        drawChevronDown(dc, chooser.x + chooser.w - 18, chooser.y + chooser.h / 2, muted);
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
        monitorHitBox = Box { monitor.x + monitor.w - 42, monitor.y, 42, 20 };
        drawToggle(dc, monitorHitBox, monitorEnabled, false);

        roundedPanel(dc, Box { monitor.x, monitor.y + 29, monitor.w, 7 }, borderSoft, borderSoft, 7);
        const float previewLevel = monitorEnabled ? 0.72f : 0.0f;
        if (previewLevel > 0.0f)
            roundedPanel(dc, Box { monitor.x, monitor.y + 29, static_cast<int>(monitor.w * previewLevel), 7 }, purple, purple, 7);
        textLine(dc, monitorEnabled ? L"MONITOR READY" : L"MONITOR MUTED",
                 Box { monitor.x, monitor.y + 43, monitor.w, 18 }, 9,
                 monitorEnabled ? teal : dim, FW_SEMIBOLD, DT_RIGHT | DT_SINGLELINE | DT_VCENTER);
    }

    void drawFooter(HDC dc, int width, int height)
    {
        const int y = height - 33;
        circle(dc, 32, y + 8, 4, green, green, 1);
        textLine(dc, L"UI ONLINE", Box { 44, y - 2, 100, 20 }, 9, green, FW_BOLD);
        textLine(dc, L"CPU: --", Box { 159, y - 2, 80, 20 }, 9, muted, FW_SEMIBOLD);
        textLine(dc, L"LATENCY: --", Box { 245, y - 2, 110, 20 }, 9, muted, FW_SEMIBOLD);
        textLine(dc, L"v0.3.1-native", Box { width - 150, y - 2, 122, 20 }, 9, muted, FW_SEMIBOLD,
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

        const int contentX = 246;
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

    void updateSliderFromMouse(HWND hwnd, int sliderIndex, int mouseX)
    {
        if (sliderIndex < 0 || sliderIndex >= 3)
            return;

        const Box& b = sliderHitBoxes[static_cast<size_t>(sliderIndex)];
        const float amount = std::clamp(static_cast<float>(mouseX - b.x) / static_cast<float>(std::max(1, b.w)), 0.0f, 1.0f);
        auto& state = presetStates[static_cast<size_t>(selectedVoice)];

        if (sliderIndex == 0)
            state.pitch = -12.0f + amount * 24.0f;
        else if (sliderIndex == 1)
            state.formant = -10.0f + amount * 20.0f;
        else
            state.mix = amount;

        InvalidateRect(hwnd, nullptr, FALSE);
    }

    void updateHoverState(HWND hwnd, int x, int y)
    {
        int newVoice = -1;
        int newNav = -1;
        int newSlider = -1;
        int newEffect = -1;

        for (int i = 0; i < 10; ++i)
            if (voiceHitBoxes[static_cast<size_t>(i)].contains(x, y))
                newVoice = i;

        for (int i = 0; i < 5; ++i)
            if (navHitBoxes[static_cast<size_t>(i)].contains(x, y))
                newNav = i;

        for (int i = 0; i < 3; ++i)
            if (sliderHitBoxes[static_cast<size_t>(i)].contains(x, y))
                newSlider = i;

        for (int i = 0; i < 9; ++i)
            if (effectHitBoxes[static_cast<size_t>(i)].contains(x, y))
                newEffect = i;

        if (newVoice != hoveredVoice || newNav != hoveredNav || newSlider != hoveredSlider || newEffect != hoveredEffect)
        {
            hoveredVoice = newVoice;
            hoveredNav = newNav;
            hoveredSlider = newSlider;
            hoveredEffect = newEffect;
            InvalidateRect(hwnd, nullptr, FALSE);
        }
    }

    HICON createAppIcon(int size)
    {
        HDC screen = GetDC(nullptr);
        HDC dc = CreateCompatibleDC(screen);
        HBITMAP colourBitmap = CreateCompatibleBitmap(screen, size, size);
        HGDIOBJ oldBitmap = SelectObject(dc, colourBitmap);

        RECT r { 0, 0, size, size };
        HBRUSH bgBrush = CreateSolidBrush(RGB(8, 12, 20));
        FillRect(dc, &r, bgBrush);
        DeleteObject(bgBrush);

        HBRUSH purpleBrush = CreateSolidBrush(purple);
        HPEN purplePen = CreatePen(PS_SOLID, std::max(1, size / 16), purpleBright);
        HGDIOBJ oldBrush = SelectObject(dc, purpleBrush);
        HGDIOBJ oldPen = SelectObject(dc, purplePen);
        const int pad = std::max(2, size / 8);
        Ellipse(dc, pad, pad, size - pad, size - pad);
        SelectObject(dc, oldPen);
        SelectObject(dc, oldBrush);
        DeleteObject(purplePen);
        DeleteObject(purpleBrush);

        HFONT font = CreateFontW(-std::max(10, size / 2), 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
                                 DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                                 CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
        HGDIOBJ oldFont = SelectObject(dc, font);
        SetBkMode(dc, TRANSPARENT);
        SetTextColor(dc, RGB(255, 255, 255));
        DrawTextW(dc, L"A", -1, &r, DT_CENTER | DT_SINGLELINE | DT_VCENTER);
        SelectObject(dc, oldFont);
        DeleteObject(font);

        SelectObject(dc, oldBitmap);
        DeleteDC(dc);

        HBITMAP maskBitmap = CreateBitmap(size, size, 1, 1, nullptr);
        HDC maskDc = CreateCompatibleDC(screen);
        HGDIOBJ oldMask = SelectObject(maskDc, maskBitmap);
        PatBlt(maskDc, 0, 0, size, size, BLACKNESS);
        SelectObject(maskDc, oldMask);
        DeleteDC(maskDc);
        ReleaseDC(nullptr, screen);

        ICONINFO info {};
        info.fIcon = TRUE;
        info.hbmColor = colourBitmap;
        info.hbmMask = maskBitmap;
        HICON icon = CreateIconIndirect(&info);

        DeleteObject(colourBitmap);
        DeleteObject(maskBitmap);
        return icon;
    }

    LRESULT CALLBACK windowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
    {
        switch (message)
        {
            case WM_CREATE:
            {
                BOOL dark = TRUE;
                DwmSetWindowAttribute(hwnd, 20, &dark, sizeof(dark));
                if (appIcon != nullptr)
                {
                    SendMessageW(hwnd, WM_SETICON, ICON_BIG, reinterpret_cast<LPARAM>(appIcon));
                    SendMessageW(hwnd, WM_SETICON, ICON_SMALL, reinterpret_cast<LPARAM>(appIcon));
                }
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

            case WM_MOUSEMOVE:
            {
                const int x = static_cast<short>(LOWORD(lParam));
                const int y = static_cast<short>(HIWORD(lParam));

                if (!trackingMouse)
                {
                    TRACKMOUSEEVENT tme {};
                    tme.cbSize = sizeof(tme);
                    tme.dwFlags = TME_LEAVE;
                    tme.hwndTrack = hwnd;
                    TrackMouseEvent(&tme);
                    trackingMouse = true;
                }

                if (draggingSlider >= 0)
                    updateSliderFromMouse(hwnd, draggingSlider, x);
                else
                    updateHoverState(hwnd, x, y);
                return 0;
            }

            case WM_MOUSELEAVE:
                trackingMouse = false;
                hoveredVoice = -1;
                hoveredNav = -1;
                hoveredSlider = -1;
                hoveredEffect = -1;
                InvalidateRect(hwnd, nullptr, FALSE);
                return 0;

            case WM_SETCURSOR:
            {
                if (LOWORD(lParam) == HTCLIENT &&
                    (hoveredVoice >= 0 || hoveredNav >= 0 || hoveredSlider >= 0 || hoveredEffect >= 0))
                {
                    SetCursor(LoadCursorW(nullptr, IDC_HAND));
                    return TRUE;
                }
                break;
            }

            case WM_LBUTTONDOWN:
            {
                const int x = static_cast<short>(LOWORD(lParam));
                const int y = static_cast<short>(HIWORD(lParam));

                for (int i = 0; i < 3; ++i)
                {
                    if (sliderHitBoxes[static_cast<size_t>(i)].contains(x, y))
                    {
                        draggingSlider = i;
                        SetCapture(hwnd);
                        updateSliderFromMouse(hwnd, i, x);
                        return 0;
                    }
                }

                for (int i = 0; i < 10; ++i)
                {
                    if (voiceHitBoxes[static_cast<size_t>(i)].contains(x, y))
                    {
                        selectedVoice = i;
                        testVoicePressed = false;
                        InvalidateRect(hwnd, nullptr, FALSE);
                        return 0;
                    }
                }

                for (int i = 0; i < 5; ++i)
                {
                    if (navHitBoxes[static_cast<size_t>(i)].contains(x, y))
                    {
                        selectedNav = i;
                        InvalidateRect(hwnd, nullptr, FALSE);
                        return 0;
                    }
                }

                for (int i = 0; i < 9; ++i)
                {
                    if (effectHitBoxes[static_cast<size_t>(i)].contains(x, y))
                    {
                        effectEnabled[static_cast<size_t>(i)] = !effectEnabled[static_cast<size_t>(i)];
                        InvalidateRect(hwnd, nullptr, FALSE);
                        return 0;
                    }
                }

                if (monitorHitBox.contains(x, y))
                {
                    monitorEnabled = !monitorEnabled;
                    InvalidateRect(hwnd, nullptr, FALSE);
                    return 0;
                }

                if (testVoiceHitBox.contains(x, y))
                {
                    testVoicePressed = !testVoicePressed;
                    InvalidateRect(hwnd, nullptr, FALSE);
                    return 0;
                }

                return 0;
            }

            case WM_LBUTTONUP:
                if (draggingSlider >= 0)
                {
                    draggingSlider = -1;
                    ReleaseCapture();
                    InvalidateRect(hwnd, nullptr, FALSE);
                }
                return 0;

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
                break;
        }

        return DefWindowProcW(hwnd, message, wParam, lParam);
    }
}

int WINAPI wWinMain(HINSTANCE instance, HINSTANCE, PWSTR, int showCommand)
{
    SetProcessDPIAware();

    appIcon = createAppIcon(32);
    const wchar_t* className = L"AnonymousVoiceMainWindow";

    WNDCLASSEXW wc {};
    wc.cbSize = sizeof(wc);
    wc.style = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
    wc.lpfnWndProc = windowProc;
    wc.hInstance = instance;
    wc.hIcon = appIcon;
    wc.hIconSm = appIcon;
    wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    wc.hbrBackground = nullptr;
    wc.lpszClassName = className;

    if (RegisterClassExW(&wc) == 0)
    {
        if (appIcon != nullptr)
            DestroyIcon(appIcon);
        return 1;
    }

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
    {
        if (appIcon != nullptr)
            DestroyIcon(appIcon);
        return 2;
    }

    ShowWindow(window, showCommand == 0 ? SW_SHOW : showCommand);
    UpdateWindow(window);

    MSG message {};
    while (GetMessageW(&message, nullptr, 0, 0) > 0)
    {
        TranslateMessage(&message);
        DispatchMessageW(&message);
    }

    if (appIcon != nullptr)
        DestroyIcon(appIcon);

    return static_cast<int>(message.wParam);
}
