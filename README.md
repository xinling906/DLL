#include <windows.h>

//=============================================================================
// 裁判類別：負責辨識卡牌點數與數字是否對應
//=============================================================================
class Referee {
public:
    // 傳入卡牌點數 (1~13) 與 右側目標數字 (1~13)
    // 如果對應正確回傳 true，不正確回傳 false
    bool CheckMatch(int cardValue, int targetValue) {
        if (cardValue == targetValue) {
            return true; // 點數與右側數字對應正確
        }
        return false;
    }
};

//=============================================================================
// 加上 __declspec(dllexport) 就可以直接導出函數，不需要額外的 .def 檔案
//=============================================================================
extern "C" __declspec(dllexport) bool check(int cardValue, int targetValue) {
    Referee referee;
    return referee.CheckMatch(cardValue, targetValue);
}
