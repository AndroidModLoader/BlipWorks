#include <mod/amlmod.h>
#include <mod/logger.h>
#include <mod/config.h>
#include <dlfcn.h>
// SAUtils
#include "isautils.h"
ISAUtils* sautils = nullptr;

#ifdef AML32
    #include "GTASA_STRUCTS.h"
    #define BYVER(__for32, __for64) (__for32)
#else
    #include "GTASA_STRUCTS_210.h"
    #define BYVER(__for32, __for64) (__for64)
    #define PAGED_ADDRESS(__addr) ((uintptr_t)__addr & 0xFFFFFFFFFFFFF000)
#endif
#define sizeofA(__aVar)  ((int)(sizeof(__aVar)/sizeof(__aVar[0])))

MYMODCFG(net.rusjj.blipworks, BlipWorks, 1.2, RusJJ)
NEEDGAME(com.rockstargames.gtasa)

// Savings
uintptr_t pGTASA;
void* hGTASA;

// Config
ConfigEntry* pCfgMapBlipScale;
ConfigEntry* pCfgRotBlipScale;
ConfigEntry* pCfgTraceBlipScale;
ConfigEntry* pCfgUnhideUnrevealedBlips;
ConfigEntry* pCfgHideNorthBlip;
ConfigEntry* pCfgYAHShowTime; unsigned int yahShowTime;
ConfigEntry* pCfgYAHHideTime; unsigned int yahHideTime;

// Game Vars
bool *bDrawYouAreHere;
uint32_t *m_snTimeInMillisecondsPauseMode, *timeNextDrawStart;
#ifdef AML64
    uintptr_t timeNextDrawStart_Paged, bDrawYouAreHere_Paged;
    float mapBlipsScale = 0.1f, rotatingBlipsScale = 0.1f, tracerBlipsScale = 0.01f;
#endif

// Game Funcs


// Own Funcs
inline void SetMapBlipsScale(float scale)
{
    #ifdef AML32
    *(float*)(pGTASA + 0x43F990) = 0.1f * scale;
    #else
    mapBlipsScale = 0.1f * scale;
    #endif
}
inline void SetRotatingBlipsScale(float scale)
{
    #ifdef AML32
    *(float*)(pGTASA + 0x441328) = 0.1f * scale;
    #else
    rotatingBlipsScale = 0.1f * scale;
    #endif
}
inline void SetTraceBlipsScale(float scale)
{
    #ifdef AML32
    *(float*)(pGTASA + 0x441A04) = 0.01f * scale;
    #else
    tracerBlipsScale = 0.01f * scale;
    #endif
}
inline void ShowUnrevealedBlips(bool show)
{
    if(show)
    {
        aml->PlaceB(pGTASA + BYVER(0x43FD0A + 0x1, 0x52512C), pGTASA + BYVER(0x43FD30 + 0x1, 0x525164));
        aml->PlaceB(pGTASA + BYVER(0x440710 + 0x1, 0x525B90), pGTASA + BYVER(0x440734 + 0x1, 0x525BCC));
    }
    else
    {
        aml->Write(pGTASA + BYVER(0x43FD0A, 0x52512C), BYVER("\x13\xD0\x6B\x48", "\xC8\x01\x00\x34"), 4);
        aml->Write(pGTASA + BYVER(0x440710, 0x525B90), BYVER("\xDF\xF8\xD0\x17", "\xE9\x01\x00\x34"), 4);
    }
}
inline void HideNorthBlip(bool hide)
{
    if(hide)
    {
        aml->PlaceB(pGTASA + BYVER(0x43EB9A + 0x1, 0x5240B4), pGTASA + BYVER(0x43EBAA + 0x1, 0x5240C8));
    }
    else
    {
        aml->Write(pGTASA + BYVER(0x43EB9A, 0x5240B4), BYVER("\x18\xEE\x10\x1A", "\xE1\x03\x40\xB9"), 4);
    }
}

// Patch funcs
uintptr_t DrawYAH_BackTo;
#ifdef AML32
__attribute__((optnone)) __attribute__((naked)) void DrawYAH_Inject(void)
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
#else
uintptr_t DrawRadarSprite_BackTo, DrawRotRadarSprite_BackTo, DrawTraceRadarSprite_BackTo;
__attribute__((optnone)) __attribute__((naked)) void DrawYAH_Inject(void)
{
    asm volatile("MOV W10, %w0" :: "r" (*m_snTimeInMillisecondsPauseMode));
    asm volatile("MOV W11, %w0" :: "r" (*timeNextDrawStart));
    asm volatile("MOV W12, %w0" :: "r" (*bDrawYouAreHere));
    asm volatile("MOV W13, %w0" :: "r" (yahHideTime));
    asm volatile("MOV W14, %w0" :: "r" (yahShowTime));

    asm volatile("MOV X16, %0" :: "r" (DrawYAH_BackTo));

    asm volatile("MOV X9, %0" :: "r" (timeNextDrawStart_Paged));
    asm volatile("MOV X8, %0" :: "r" (bDrawYouAreHere_Paged));

    asm volatile("BR X16");
}
__attribute__((optnone)) __attribute__((naked)) void DrawRadarSprite_Inject(void)
{
    asm volatile("LDR S0, [X21,#0x2C]\nLDR S1, [X21,#0x34]");
    asm volatile("FMOV S2, %w0" :: "r" (mapBlipsScale));

    asm volatile("MOV X16, %0" :: "r" (DrawRadarSprite_BackTo));
    asm volatile("BR X16");
}
static float someUnkValue = -0.785398f;
__attribute__((optnone)) __attribute__((naked)) void DrawRotRadarSprite_Inject(void)
{
    asm volatile("FMOV S2, %w0" :: "r" (someUnkValue));
    asm volatile("FMOV S3, %w0" :: "r" (rotatingBlipsScale));

    asm volatile("MOV X16, %0" :: "r" (DrawRotRadarSprite_BackTo));
    asm volatile("BR X16");
}
__attribute__((optnone)) __attribute__((naked)) void DrawTraceRadarSprite_Inject(void)
{
    asm volatile("LDP S0, S1, [X8,#0x2C]\nLDP S2, S3, [X8,#0x34]");
    asm volatile("FMOV S3, %w0" :: "r" (tracerBlipsScale));

    asm volatile("MOV X16, %0" :: "r" (DrawTraceRadarSprite_BackTo));
    asm volatile("BR X16");
}
#endif

// Config related
char szRetText[8];
void BSChanged1(int oldVal, int newVal, void* data)
{
    pCfgMapBlipScale->SetFloat(0.01f * newVal);
    SetMapBlipsScale(0.01f * newVal);
    cfg->Save();
}
void BSChanged2(int oldVal, int newVal, void* data)
{
    pCfgMapBlipScale->SetFloat(0.01f * newVal);
    SetRotatingBlipsScale(0.01f * newVal);
    cfg->Save();
}
void BSChanged3(int oldVal, int newVal, void* data)
{
    pCfgMapBlipScale->SetFloat(0.01f * newVal);
    SetTraceBlipsScale(0.01f * newVal);
    cfg->Save();
}
void BSChanged4(int oldVal, int newVal, void* data)
{
    pCfgUnhideUnrevealedBlips->SetBool(newVal != 0);
    ShowUnrevealedBlips(newVal != 0);
    cfg->Save();
}
void BSChanged5(int oldVal, int newVal, void* data)
{
    pCfgHideNorthBlip->SetBool(newVal != 0);
    HideNorthBlip(newVal != 0);
    cfg->Save();
}
const char* SettingGetTextFormatted(int newVal, void* data)
{
    sprintf(szRetText, "%.1f%%", (float)newVal);
    return szRetText;
}

// int main!
const char* pYesNo[] = 
{
    "FEM_OFF",
    "FEM_ON",
};
extern "C" void OnModLoad()
{
    logger->SetTag("BlipWorks");
    pGTASA = aml->GetLib("libGTASA.so");
    hGTASA = dlopen("libGTASA.so", RTLD_LAZY);
    
    pCfgMapBlipScale = cfg->Bind("MapBlipScale", 0.7f);
    pCfgRotBlipScale = cfg->Bind("RotatingBlipScale", 0.8f);
    pCfgTraceBlipScale = cfg->Bind("TraceBlipScale", 0.8f);
    pCfgUnhideUnrevealedBlips = cfg->Bind("UnhideUnrevealedBlips", false);
    pCfgHideNorthBlip = cfg->Bind("HideNorthBlip", false);
    pCfgYAHShowTime = cfg->Bind("YouAreHereShowTime", 500); yahShowTime = pCfgYAHShowTime->GetInt();
    pCfgYAHHideTime = cfg->Bind("YouAreHereHideTime", 200); yahHideTime = pCfgYAHHideTime->GetInt();
    
    #ifdef AML32
    aml->Unprot(pGTASA + 0x43F990, sizeof(float));
    aml->Unprot(pGTASA + 0x441328, sizeof(float));
    aml->Unprot(pGTASA + 0x441A04, sizeof(float));

    DrawYAH_BackTo = pGTASA + 0x440F82 + 0x1;
    aml->Redirect(pGTASA + 0x440F6E + 0x1, (uintptr_t)DrawYAH_Inject);
    aml->Write(pGTASA + 0x440F8A, (uintptr_t)"\x00\xBF\xA3\x42", 4);
    aml->Write(pGTASA + 0x440FA0, (uintptr_t)"\xBB\x42", 2);
    #else
    aml->Redirect(pGTASA + 0x524CB4, (uintptr_t)DrawRadarSprite_Inject); DrawRadarSprite_BackTo = pGTASA + 0x524CC4;
    aml->Redirect(pGTASA + 0x5264CC, (uintptr_t)DrawRotRadarSprite_Inject); DrawRotRadarSprite_BackTo = pGTASA + 0x5264DC;
    aml->Redirect(pGTASA + 0x526A24, (uintptr_t)DrawTraceRadarSprite_Inject); DrawTraceRadarSprite_BackTo = pGTASA + 0x526A34;

    DrawYAH_BackTo = pGTASA + 0x5262D0;
    aml->Redirect(pGTASA + 0x5262B4, (uintptr_t)DrawYAH_Inject);
    aml->Write(pGTASA + 0x5262E0, (uintptr_t)"\x7F\x01\x0E\x6B", 4);
    aml->Write(pGTASA + 0x5262F4, (uintptr_t)"\x7F\x01\x0D\x6B", 4);
    #endif

    SET_TO(bDrawYouAreHere, pGTASA + BYVER(0x6AE3C8, 0x889CA0));
    SET_TO(timeNextDrawStart, pGTASA + BYVER(0x994EF0, 0xC2486C));
    SET_TO(m_snTimeInMillisecondsPauseMode, aml->GetSym(hGTASA, "_ZN6CTimer31m_snTimeInMillisecondsPauseModeE"));
    #ifdef AML64
        bDrawYouAreHere_Paged = PAGED_ADDRESS(bDrawYouAreHere);
        timeNextDrawStart_Paged = PAGED_ADDRESS(timeNextDrawStart);
    #endif
    
    SetMapBlipsScale(pCfgMapBlipScale->GetFloat());
    SetRotatingBlipsScale(pCfgRotBlipScale->GetFloat());
    SetTraceBlipsScale(pCfgTraceBlipScale->GetFloat());
    ShowUnrevealedBlips(pCfgUnhideUnrevealedBlips->GetBool());
    HideNorthBlip(pCfgHideNorthBlip->GetBool());
    
    sautils = (ISAUtils*)GetInterface("SAUtils");
    if(sautils)
    {
        eTypeOfSettings settingTab = sautils->AddSettingsTab("BlipWorks");

        sautils->AddSliderItem(settingTab, "Radar Icon Scale", (int)(pCfgMapBlipScale->GetFloat() * 100), 10, 200, BSChanged1, SettingGetTextFormatted);
        sautils->AddSliderItem(settingTab, "Rotating Icon Scale", (int)(pCfgRotBlipScale->GetFloat() * 100), 10, 200, BSChanged2, SettingGetTextFormatted);
        sautils->AddSliderItem(settingTab, "Trace Icon Scale", (int)(pCfgTraceBlipScale->GetFloat() * 100), 10, 200, BSChanged3, SettingGetTextFormatted);

        sautils->AddClickableItem(settingTab, "Show Unrevealed Blips", pCfgUnhideUnrevealedBlips->GetBool(), 0, sizeofA(pYesNo)-1, pYesNo, BSChanged4);
        sautils->AddClickableItem(settingTab, "Hide North Blip", pCfgHideNorthBlip->GetBool(), 0, sizeofA(pYesNo)-1, pYesNo, BSChanged5);
    }
}
