#include <windows.h>
#include <string>
#include <vector>
#include <array>
#include <random>
#include <sstream>
#include <algorithm>
#include <tchar.h>

#pragma comment(lib, "gdi32.lib")
#pragma comment(lib, "user32.lib")

//=============================================================================
// 1. 遊戲狀態枚舉與結構定義
//=============================================================================
enum class AppState {
    SHOW_52_CARDS,
    SINGLE_MATCH,
    DOUBLE_MATCH
};

struct Suit {
    const wchar_t* symbol;
    const wchar_t* name;
    COLORREF color;
};

struct CardValue {
    const wchar_t* label;
    int value;
};

struct PipPos {
    int dx; int dy;
};

//=============================================================================
// 2. 文字與花色版面繪製類別
//=============================================================================
class TextRenderer {
public:
    static void Draw(HDC hdc, int x, int y, const std::wstring& text, int fontSize,
        COLORREF color, UINT textAlign, int angle10 = 0,
        const wchar_t* fontName = L"Segoe UI Symbol", int fontWeight = FW_NORMAL)
    {
        LOGFONTW lf = {};
        lf.lfHeight = -fontSize;
        lf.lfEscapement = angle10;
        lf.lfOrientation = angle10;
        lf.lfWeight = fontWeight;
        lf.lfCharSet = DEFAULT_CHARSET;
        lstrcpyW(lf.lfFaceName, fontName);

        HFONT hFont = CreateFontIndirectW(&lf);
        HFONT hOldFont = (HFONT)SelectObject(hdc, hFont);
        COLORREF oldColor = SetTextColor(hdc, color);
        int oldBkMode = SetBkMode(hdc, TRANSPARENT);
        UINT oldAlign = SetTextAlign(hdc, textAlign);

        TextOutW(hdc, x, y, text.c_str(), static_cast<int>(text.length()));

        SetTextAlign(hdc, oldAlign);
        SetBkMode(hdc, oldBkMode);
        SetTextColor(hdc, oldColor);
        SelectObject(hdc, hOldFont);
        DeleteObject(hFont);
    }
};

class PipLayout {
public:
    static std::vector<PipPos> GetPositions(int count) {
        int top = -55, bottom = 55;
        int v31 = top / 2, v32 = -v31;
        int v41 = top / 3, v42 = -v41;
        int v51 = (top + v41) / 2, v52 = -v51;
        int mid = 0, cen = 0, left = -25, right = 25;

        std::vector<std::vector<PipPos>> positions(11);
        positions[1] = { {cen, mid} };
        positions[2] = { {cen, top}, {cen, bottom} };
        positions[3] = { {cen, top}, {cen, mid}, {cen, bottom} };
        positions[4] = { {left, top}, {right, top}, {left, bottom}, {right, bottom} };
        positions[5] = { {left, top}, {right, top}, {cen, mid}, {left, bottom}, {right, bottom} };
        positions[6] = { {left, top}, {right, top}, {left, mid}, {right, mid}, {left, bottom}, {right, bottom} };
        positions[7] = { {left, top}, {right, top}, {left, mid}, {right, mid}, {cen, v31}, {left, bottom}, {right, bottom} };
        positions[8] = { {left, top}, {right, top}, {cen, v31}, {left, mid}, {right, mid}, {cen, v32}, {left, bottom}, {right, bottom} };
        positions[9] = { {left, top}, {right, top}, {cen, v51}, {left, v41}, {right, v41}, {left, v42}, {right, v42}, {left, bottom}, {right, bottom} };
        positions[10] = { {left, top}, {right, top}, {cen, v51}, {left, v41}, {right, v41}, {left, v42}, {right, v42}, {cen, v52}, {left, bottom}, {right, bottom} };

        if (count < 1 || count > 10) return {};
        return positions[count];
    }
};
//=============================================================================
// 3. 撲克牌繪製類別
//=============================================================================
class Card {
private:
    Suit suit_;
    CardValue value_;
    int width_ = 120;
    int height_ = 180;
    int padding_ = 10;

public:
    Card(const CardValue& value, const Suit& suit, float scale = 1.0f)
        : suit_(suit), value_(value) {
        width_ = static_cast<int>(120 * scale);
        height_ = static_cast<int>(180 * scale);
        padding_ = static_cast<int>(10 * scale);
    }

    void Draw(HDC hdc, int left, int top, float scale = 1.0f) const {
        HBRUSH hWhite = CreateSolidBrush(RGB(255, 255, 255));
        HBRUSH hOldBrush = (HBRUSH)SelectObject(hdc, hWhite);
        HPEN hPen = CreatePen(PS_SOLID, 1, RGB(0, 0, 0));
        HPEN hOldPen = (HPEN)SelectObject(hdc, hPen);

        RoundRect(hdc, left, top, left + width_, top + height_, (int)(10 * scale), (int)(10 * scale));

        SelectObject(hdc, hOldBrush); SelectObject(hdc, hOldPen);
        DeleteObject(hWhite); DeleteObject(hPen);

        int innerLeft = left + padding_;
        int innerTop = top + padding_;
        int innerW = width_ - 2 * padding_;
        int innerH = height_ - 2 * padding_;
        int innerCx = innerLeft + innerW / 2;
        int innerCy = innerTop + innerH / 2;

        TextRenderer::Draw(hdc, innerLeft, innerTop + (int)(16 * scale), value_.label, (int)(16 * scale), suit_.color, TA_LEFT | TA_BASELINE, 0, L"Segoe UI");
        TextRenderer::Draw(hdc, innerLeft, innerTop + (int)(32 * scale), suit_.symbol, (int)(16 * scale), suit_.color, TA_LEFT | TA_BASELINE, 0, L"Segoe UI Symbol");

        TextRenderer::Draw(hdc, left + width_ - padding_ * 2, top + height_ - padding_ * 2 - (int)(16 * scale), suit_.symbol, (int)(16 * scale), suit_.color, TA_RIGHT | TA_BASELINE, 1800, L"Segoe UI Symbol");
        TextRenderer::Draw(hdc, left + width_ - padding_ * 2, top + height_ - padding_ * 2, value_.label, (int)(16 * scale), suit_.color, TA_RIGHT | TA_BASELINE, 1800, L"Segoe UI");

        if (value_.value >= 1 && value_.value <= 10) {
            auto positions = PipLayout::GetPositions(value_.value);
            for (const auto& p : positions) {
                int x = innerCx + (int)(p.dx * scale);
                int y = innerCy + (int)(p.dy * scale);
                if (p.dy < 5) {
                    TextRenderer::Draw(hdc, x, y, suit_.symbol, (int)(24 * scale), suit_.color, TA_CENTER | TA_BASELINE, 0, L"Segoe UI Symbol");
                }
                else {
                    TextRenderer::Draw(hdc, x, y, suit_.symbol, (int)(24 * scale), suit_.color, TA_CENTER | TA_BASELINE, 1800, L"Segoe UI Symbol");
                }
            }
        }
        else {
            TextRenderer::Draw(hdc, innerCx, innerCy + (int)(14 * scale), value_.label, (int)(40 * scale), suit_.color, TA_CENTER | TA_BASELINE, 0, L"Segoe UI", FW_BOLD);
        }
    }
};
//=============================================================================
// 4. 主視窗視窗應用程式核心
//=============================================================================
class PokerWindowApp {
private:
    HINSTANCE hInstance_;
    HWND hwnd_;
    AppState state_ = AppState::SHOW_52_CARDS;

    bool running_ = false;
    bool hasCurrentItem_ = false;
    int countdownValue_ = 0;
    ULONGLONG gameStartTick_ = 0;
    std::mt19937 rng_;
    std::vector<int> deckOrder_;
    size_t deckPos_ = 0;

    int currentSuitIndex_ = 0;
    int currentValueIndex_ = 0;
    int currentTargetValue_ = 0;

    static constexpr UINT ID_SHOW_52 = 40001;
    static constexpr UINT ID_SINGLE_GAME = 40002;
    static constexpr UINT ID_DOUBLE_GAME = 40003;
    static constexpr UINT ID_GAME_STOP = 40004;
    static constexpr UINT ID_EXIT = 40005;

    static constexpr UINT TIMER_ID = 1001;

public:
    explicit PokerWindowApp(HINSTANCE hInst) : hInstance_(hInst), hwnd_(nullptr), rng_(std::random_device{}()) {}

    bool Create(int nCmdShow) {
        WNDCLASSW wc = {};
        wc.lpfnWndProc = PokerWindowApp::WndProc;
        wc.hInstance = hInstance_;
        wc.lpszClassName = L"IntegratedPokerAppClass";
        wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
        wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);

        RegisterClassW(&wc);

        hwnd_ = CreateWindowExW(
            0, wc.lpszClassName, L"撲克牌整合自動辨識裁判系統",
            WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, 1200, 800,
            nullptr, nullptr, hInstance_, this
        );

        if (!hwnd_) return false;

        SetMenu(hwnd_, CreateAppMenu());
        ShowWindow(hwnd_, nCmdShow);
        UpdateWindow(hwnd_);
        return true;
    }

    int Run() {
        MSG msg = {};
        while (GetMessageW(&msg, nullptr, 0, 0)) {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
        return static_cast<int>(msg.wParam);
    }

private:
    HMENU CreateAppMenu() {
        HMENU hMenuBar = CreateMenu();
        AppendMenuW(hMenuBar, MF_STRING, ID_SHOW_52, L"【功能一】畫52張牌");
        AppendMenuW(hMenuBar, MF_STRING, ID_SINGLE_GAME, L"【功能二】單人計時賽");
        AppendMenuW(hMenuBar, MF_STRING, ID_DOUBLE_GAME, L"【功能三】雙人計時賽");
        AppendMenuW(hMenuBar, MF_STRING, ID_GAME_STOP, L"停止遊戲");
        AppendMenuW(hMenuBar, MF_STRING, ID_EXIT, L"結束");
        return hMenuBar;
    }

    static const std::array<Suit, 4>& GetSuits() {
        static const std::array<Suit, 4> suits = {
            Suit{ L"♠", L"spade",  RGB(0, 0, 0) },
            Suit{ L"♥", L"heart",  RGB(220, 0, 0) },
            Suit{ L"♦", L"diamond",RGB(220, 0, 0) },
            Suit{ L"♣", L"club",   RGB(0, 0, 0) }
        };
        return suits;
    }

    static const std::array<CardValue, 13>& GetCardValues() {
        static const std::array<CardValue, 13> values = {
            CardValue{ L"A", 1 }, CardValue{ L"2", 2 }, CardValue{ L"3", 3 },
            CardValue{ L"4", 4 }, CardValue{ L"5", 5 }, CardValue{ L"6", 6 },
            CardValue{ L"7", 7 }, CardValue{ L"8", 8 }, CardValue{ L"9", 9 },
            CardValue{ L"10", 10 }, CardValue{ L"J", 11 }, CardValue{ L"Q", 12 },
            CardValue{ L"K", 13 }
        };
        return values;
    }

    void StartGame(AppState mode) {
        StopGame();
        state_ = mode;
        running_ = true;
        hasCurrentItem_ = false;
        countdownValue_ = 3;
        currentTargetValue_ = 0;

        deckOrder_.clear();
        for (int i = 0; i < 52; ++i) deckOrder_.push_back(i);
        std::shuffle(deckOrder_.begin(), deckOrder_.end(), rng_);
        deckPos_ = 0;

        SetTimer(hwnd_, TIMER_ID, 1000, nullptr);
        InvalidateRect(hwnd_, nullptr, TRUE);
    }

    void StopGame() {
        KillTimer(hwnd_, TIMER_ID);
        running_ = false;
        hasCurrentItem_ = false;
        countdownValue_ = 0;
        InvalidateRect(hwnd_, nullptr, TRUE);
    }

    bool CallDLLReferee(int cardVal, int targetVal) {
        HMODULE hDll = LoadLibrary(TEXT("Project5.dll"));
        if (hDll != NULL) {
            typedef bool (*CheckFunc)(int, int);
            CheckFunc dllCheck = (CheckFunc)GetProcAddress(hDll, "check");
            if (dllCheck != NULL) {
                // 🌟 核心修正：將參數嚴格對應卡牌值與目標值
                bool res = dllCheck(cardVal, targetVal);
                FreeLibrary(hDll);
                return res;
            }
            FreeLibrary(hDll);
        }
        return (cardVal == targetVal);
    }

    void ShowNextCard() {
        KillTimer(hwnd_, TIMER_ID);

        if (deckPos_ >= deckOrder_.size()) {
            StopGame();
            MessageBoxW(hwnd_, L"52 張牌全部發完！遊戲結束。", L"提示", MB_OK);
            return;
        }
        // 1. 先決定下一組的數據
        currentTargetValue_ = (currentTargetValue_ % 13) + 1;
        int cardId = deckOrder_[deckPos_++];
        currentSuitIndex_ = cardId / 13;
        currentValueIndex_ = cardId % 13;
        hasCurrentItem_ = true;

        // 2. 【關鍵修正】強制讓視窗立刻繪製最新畫面，讓玩家眼睛看到的跟 DLL 判斷的完全一致
        InvalidateRect(hwnd_, nullptr, TRUE);
        UpdateWindow(hwnd_);

        // 3. 取得目前畫面顯示的點數
        int card_val = GetCardValues()[currentValueIndex_].value;

        // 4. 丟給 DLL 進行自動辨識
        bool isMatched = CallDLLReferee(card_val, currentTargetValue_);

        if (isMatched) {
            ULONGLONG elapsed = GetTickCount64() - gameStartTick_;
            StopGame();

            std::wstringstream ss;
            ss << L"【DLL 自動辨識成功！】\n\n"
                << L"發出卡牌：" << GetCardValues()[currentValueIndex_].label << L"\n"
                << L"目標數字：" << currentTargetValue_ << L"\n"
                << L"狀態：【兩者完全對應】\n\n"
                << L"系統已為您自動停下計時！\n"
                << L"花費總時間：" << elapsed << L" 毫秒";

            MessageBoxW(hwnd_, ss.str().c_str(), L"自動停下通知", MB_OK | MB_ICONINFORMATION);
            return;
        }

        if (running_) {
            SetTimer(hwnd_, TIMER_ID, 1000, nullptr);
        }
    }

    void OnPaint() {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd_, &ps);
        RECT rc; GetClientRect(hwnd_, &rc);

        HBRUSH bg = CreateSolidBrush(RGB(255, 255, 255));
        FillRect(hdc, &rc, bg); DeleteObject(bg);

        int centerX = (rc.left + rc.right) / 2;

        if (state_ == AppState::SHOW_52_CARDS) {
            float scale = 0.65f;
            int startX = 25, startY = 40;
            int cardW = (int)(120 * scale), cardH = (int)(180 * scale);
            int gapX = cardW + 10, gapY = cardH + 15;

            for (int suit = 0; suit < 4; suit++) {
                for (int val = 0; val < 13; val++) {
                    Card card(GetCardValues()[val], GetSuits()[suit], scale);
                    card.Draw(hdc, startX + val * gapX, startY + suit * gapY, scale);
                }
            }
            TextRenderer::Draw(hdc, centerX, 10, L"功能一：完整 52 張撲克牌展示花色旋轉", 18, RGB(0, 0, 0), TA_CENTER | TA_TOP, 0, L"Microsoft JhengHei UI", FW_BOLD);
        }
        else {
            if (countdownValue_ > 0) {
                std::wstringstream ss; ss << countdownValue_;
                TextRenderer::Draw(hdc, centerX, 300, ss.str(), 100, RGB(230, 40, 40), TA_CENTER | TA_BASELINE, 0, L"Segoe UI", FW_BOLD);
                TextRenderer::Draw(hdc, centerX, 360, L"自動辨識計時賽即將開始...", 20, RGB(100, 100, 100), TA_CENTER | TA_BASELINE, 0, L"Microsoft JhengHei UI");
            }
            else if (hasCurrentItem_) {
                Card card(GetCardValues()[currentValueIndex_], GetSuits()[currentSuitIndex_], 1.2f);
                card.Draw(hdc, centerX - 72, 250, 1.2f);

                std::wstringstream ssTarget;
                if (currentTargetValue_ == 1) ssTarget << L"A";
                else if (currentTargetValue_ == 11) ssTarget << L"J";
                else if (currentTargetValue_ == 12) ssTarget << L"Q";
                else if (currentTargetValue_ == 13) ssTarget << L"K";
                else ssTarget << currentTargetValue_;

                TextRenderer::Draw(hdc, centerX + 200, 360, ssTarget.str(), 80, RGB(0, 100, 220), TA_CENTER | TA_BASELINE, 0, L"Segoe UI", FW_BOLD);
                TextRenderer::Draw(hdc, centerX + 200, 410, L"目標配對數字", 16, RGB(120, 120, 120), TA_CENTER | TA_BASELINE, 0, L"Microsoft JhengHei UI");

                if (state_ == AppState::SINGLE_MATCH) {
                    TextRenderer::Draw(hdc, centerX, 50, L"【單人自動賽】DLL 正在自動進行辨識對應...", 22, RGB(0, 120, 0), TA_CENTER | TA_TOP, 0, L"Microsoft JhengHei UI", FW_BOLD);
                }
                else {
                    TextRenderer::Draw(hdc, centerX, 50, L"【雙人自動賽】DLL 正在自動進行辨識對應...", 22, RGB(180, 50, 0), TA_CENTER | TA_TOP, 0, L"Microsoft JhengHei UI", FW_BOLD);
                }

                std::wstringstream ssProgress; ssProgress << L"發牌進度: " << deckPos_ << L" / 52";
                TextRenderer::Draw(hdc, 30, rc.bottom - 40, ssProgress.str(), 16, RGB(100, 100, 100), TA_LEFT | TA_BASELINE, 0, L"Microsoft JhengHei UI");
            }
        }

        EndPaint(hwnd_, &ps);
    }

    LRESULT HandleMessage(UINT msg, WPARAM wParam, LPARAM lParam) {
        switch (msg) {
        case WM_COMMAND:
            switch (LOWORD(wParam)) {
            case ID_SHOW_52:
                StopGame();
                state_ = AppState::SHOW_52_CARDS;
                InvalidateRect(hwnd_, nullptr, TRUE);
                break;
            case ID_SINGLE_GAME:
                StartGame(AppState::SINGLE_MATCH);
                break;
            case ID_DOUBLE_GAME:
                StartGame(AppState::DOUBLE_MATCH);
                break;
            case ID_GAME_STOP:
                StopGame();
                break;
            case ID_EXIT:
                DestroyWindow(hwnd_);
                break;
            }
            return 0;

        case WM_TIMER:
            if (wParam == TIMER_ID && running_) {
                if (countdownValue_ > 0) {
                    countdownValue_--;
                    if (countdownValue_ == 0) {
                        gameStartTick_ = GetTickCount64();
                        ShowNextCard();
                    }
                    else {
                        InvalidateRect(hwnd_, nullptr, TRUE);
                    }
                }
                else {
                    ShowNextCard();
                }
            }
            return 0;
        case WM_PAINT:
            OnPaint();
            return 0;

        case WM_DESTROY:
            StopGame();
            PostQuitMessage(0);
            return 0;
        }
        return DefWindowProcW(hwnd_, msg, wParam, lParam);
    }

public:
    static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
        PokerWindowApp* pThis = nullptr;
        if (msg == WM_NCCREATE) {
            CREATESTRUCTW* cs = reinterpret_cast<CREATESTRUCTW*>(lParam);
            pThis = static_cast<PokerWindowApp*>(cs->lpCreateParams);
            SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pThis));
            pThis->hwnd_ = hwnd;
        }
        else {
            pThis = reinterpret_cast<PokerWindowApp*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
        }
        if (pThis) return pThis->HandleMessage(msg, wParam, lParam);
        return DefWindowProcW(hwnd, msg, wParam, lParam);
    }
};

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR intCmdShow, int nShowCmd) {
    PokerWindowApp app(hInstance);
    if (!app.Create(nShowCmd)) return 0;
    return app.Run();
}