// DLL.cpp : 定義 DLL 的匯出函式。
//

#include "framework.h"
#include "DLL.h"


// 這是匯出變數的範例
DLL_API int nDLL=0;

// 這是匯出函式的範例。
DLL_API int fnDLL(void)
{
    return 0;
}

// 這是已匯出的類別建構函式。
CDLL::CDLL()
{
    return;
}
