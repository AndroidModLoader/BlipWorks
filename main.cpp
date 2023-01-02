#include <mod/amlmod.h>
#include <mod/logger.h>
#include <mod/config.h>
#include <dlfcn.h>
// SAUtils
#include "isautils.h"
ISAUtils* sautils = nullptr;

#include <GTASA_STRUCTS.h>

MYMODCFG(net.rusjj.blipworks, BlipWorks, 1.0, RusJJ)
NEEDGAME(com.rockstargames.gtasa)

// Savings
uintptr_t pGTASA;
void* hGTASA;

// Config
ConfigEntry* pCfgMapBlipScale;
ConfigEntry* pCfgRotBlipScale;
ConfigEntry* pCfgTraceBlipScale;
ConfigEntry* pCfgUnhideUnrevealedBlips;
ConfigEntry* pCfgYAHShowTime; unsigned int yahShowTime;
ConfigEntry* pCfgYAHHideTime; unsigned int yahHideTime;

// Game Vars
bool *bDrawYouAreHere;
uint32_t *m_snTimeInMillisecondsPauseMode, *timeNextDrawStart;

// Game Funcs


// Own Funcs
inline void SetMapBlipsScale(float scale)
{
    *(float*)(pGTASA + 0x43F990) = 0.1f * scale;
}
inline void SetRotatingBlipsScale(float scale)
{
    *(float*)(pGTASA + 0x441328) = 0.1f * scale;
}
inline void SetTraceBlipsScale(float scale)
{
    *(float*)(pGTASA + 0x441A04) = 0.01f * scale;
}
inline void SetHideUnrevealedBlips(bool hide)
{
    if(hide)
    {
        aml->Write(pGTASA + 0x43FD0A, (uintptr_t)"\x13\xD0\x6B\x48\x78\x44\x00\x68\x00\x68", 10);
        aml->Write(pGTASA + 0x440710, (uintptr_t)"\xDF\xF8\xD0\x17\x79\x44\x09\x68", 8);
    }
    else
    {
        aml->Redirect(pGTASA + 0x43FD0A + 0x1, pGTASA + 0x43FD30 + 0x1);
        aml->Redirect(pGTASA + 0x440710 + 0x1, pGTASA + 0x440734 + 0x1);
    }
}

// Patch funcs
uintptr_t DrawYAH_BackTo;
__attribute__((optnone)) __attribute__((naked)) void DrawYAH_inject(void)
{
    asm volatile(
        "PUSH {R0}\n");
    asm volatile(
        "MOV R2, %0\n"
    :: "r" (*m_snTimeInMillisecondsPauseMode));
    asm volatile(
        "MOV R3, %0\n"
    :: "r" (*timeNextDrawStart));
    asm volatile(
        "MOV R4, %0\n"
    :: "r" (yahShowTime));
    asm volatile(
        "MOV R5, %0\n"
    :: "r" (*bDrawYouAreHere));
    asm volatile(
        "MOV R7, %0\n"
    :: "r" (yahHideTime));
    asm volatile(
        "MOV R12, %0\n"
        "POP {R0}\n"
        "BX R12\n"
    :: "r" (DrawYAH_BackTo));
}

// Config related
char szRetText[8];
void BSChanged1(int oldVal, int newVal)
{
    pCfgMapBlipScale->SetFloat(0.01f * newVal);
    SetMapBlipsScale(0.01f * newVal);
    cfg->Save();
}
const char* BSDraw1(int newVal)
{
    sprintf(szRetText, "%.1f%%", (float)newVal);
    return szRetText;
}
void BSChanged2(int oldVal, int newVal)
{
    pCfgMapBlipScale->SetFloat(0.01f * newVal);
    SetRotatingBlipsScale(0.01f * newVal);
    cfg->Save();
}
const char* BSDraw2(int newVal)
{
    sprintf(szRetText, "%.1f%%", (float)newVal);
    return szRetText;
}
void BSChanged3(int oldVal, int newVal)
{
    pCfgMapBlipScale->SetFloat(0.01f * newVal);
    SetTraceBlipsScale(0.01f * newVal);
    cfg->Save();
}
const char* BSDraw3(int newVal)
{
    sprintf(szRetText, "%.1f%%", (float)newVal);
    return szRetText;
}

// int main!
extern "C" void OnModLoad()
{
    logger->SetTag("BlipWorks");
    pGTASA = aml->GetLib("libGTASA.so");
    hGTASA = dlopen("libGTASA.so", RTLD_LAZY);
    
    pCfgMapBlipScale = cfg->Bind("MapBlipScale", 0.7f);
    pCfgRotBlipScale = cfg->Bind("RotatingBlipScale", 0.8f);
    pCfgTraceBlipScale = cfg->Bind("TraceBlipScale", 0.8f);
    pCfgUnhideUnrevealedBlips = cfg->Bind("UnhideUnrevealedBlips", false);
    pCfgYAHShowTime = cfg->Bind("YouAreHereShowTime", 500); yahShowTime = pCfgYAHShowTime->GetInt();
    pCfgYAHHideTime = cfg->Bind("YouAreHereHideTime", 201); yahHideTime = pCfgYAHHideTime->GetInt();
    
    aml->Unprot(pGTASA + 0x43F990, sizeof(float));
    aml->Unprot(pGTASA + 0x441328, sizeof(float));
    aml->Unprot(pGTASA + 0x441A04, sizeof(float));
    
    DrawYAH_BackTo = pGTASA + 0x440F82 + 0x1;
    aml->Redirect(pGTASA + 0x440F6E + 0x1, (uintptr_t)DrawYAH_inject);
    aml->Write(pGTASA + 0x440F8A, (uintptr_t)"\x00\xBF\xA3\x42", 4);
    aml->Write(pGTASA + 0x440FA0, (uintptr_t)"\xBB\x42", 2);
    
    SET_TO(bDrawYouAreHere, pGTASA + 0x6AE3C8);
    SET_TO(timeNextDrawStart, pGTASA + 0x994EF0);
    SET_TO(m_snTimeInMillisecondsPauseMode, aml->GetSym(hGTASA, "_ZN6CTimer31m_snTimeInMillisecondsPauseModeE"));
    
    SetMapBlipsScale(pCfgMapBlipScale->GetFloat());
    SetRotatingBlipsScale(pCfgRotBlipScale->GetFloat());
    SetTraceBlipsScale(pCfgTraceBlipScale->GetFloat());
    SetHideUnrevealedBlips(pCfgUnhideUnrevealedBlips->GetBool());
    
    sautils = (ISAUtils*)GetInterface("SAUtils");
    if(sautils)
    {
        sautils->AddSliderItem(SetType_Mods, "Radar Icon Scale", (int)(pCfgMapBlipScale->GetFloat() * 100), 10, 200, BSChanged1, BSDraw1);
        sautils->AddSliderItem(SetType_Mods, "Rotating Icon Scale", (int)(pCfgRotBlipScale->GetFloat() * 100), 10, 200, BSChanged2, BSDraw2);
        sautils->AddSliderItem(SetType_Mods, "Trace Icon Scale", (int)(pCfgTraceBlipScale->GetFloat() * 100), 10, 200, BSChanged3, BSDraw3);
    }
}
