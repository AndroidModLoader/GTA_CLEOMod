#include <mod/amlmod.h>
#include <mod/logger.h>
#include <mod/config.h>

#include <stdlib.h>
#include <sys/stat.h>
#include <fstream>
#include <stdint.h>
#include <dlfcn.h>
#include <string.h>
#include <cleohelpers.h>

// CLEO
#include "cleo.h"
cleo_ifs_t* cleo = nullptr;

#include "cleoaddon.h"
cleo_addon_ifs_t cleo_addon_ifs;
uint16_t FreeScriptAddonInfoId = 1; // 0 is "not assigned" (used for dumbo scripts without that info)
ScriptAddonInfo ScriptAddonInfosStorage[ScriptAddonInfo::allocSize];
char ScriptAddonVarStackStorage[ScriptAddonInfo::allocSize][ScriptAddonInfo::scriptStackSize] { 0 };

char g_szSavesPath[256] { 0 };
char szCLEOVer[64] { 0 };

// SAUtils
#include "isautils.h"
ISAUtils* sautils = nullptr;

// Size of array
#define sizeofA(__aVar)  ((int)(sizeof(__aVar)/sizeof(__aVar[0])))

MYMODCFG(net.rusjj.cleolib, CLEO Library, 2.0.1.10, Alexander Blade & RusJJ & XMDS)
BEGIN_DEPLIST()
    ADD_DEPENDENCY_VER(net.rusjj.aml, 1.3.0)
END_DEPLIST()

inline size_t __strlen(const char *str)
{
    const char* s = str;
    while(*s) ++s;
    return (s - str);
}
inline bool __ispathdel(char s)
{
    return (s == '\\' || s == '/');
}
inline void __pathback(char *str)
{
    const char* s = str;
    uint16_t i = 0;
    while(*s) ++s;
    while(s != str)
    {
        if(!__ispathdel(*(--s))) break;
    }
    while(s != str)
    {
        if(__ispathdel(*(--s)))
        {
            i = (uint16_t)(s - str);
        }
        else if(i != 0) break;
    }
    if(i > 0) str[i] = 0;
}

// Pointers
void* pCLEO;
uintptr_t nCLEOAddr, nGameAddr;
Dl_info pDLInfo;
eGameIdent* nGameIdent;
uint8_t g_ScriptBytesBuffer[16 * 1024 * 1024] { 0 };

// Configs
ConfigEntry* pCfgCLEOLocation;
ConfigEntry* pCfgCLEORedArrow;
ConfigEntry* pCfgCLEOMenuColor;
ConfigEntry* pCfgCLEOMenuArrowColor;
ConfigEntry* pCfgCLEOMenuArrowPressedAlpha;

// CLEO 2.0.1 pointers
rgba_t* pCLEOMenuColor; // 1525C
rgba_t* pCLEOMenuArrowColor; // 15250
uint8_t* pCLEOArrowLastAlpha; // 2194FC
int* pScriptsStorage; // 192E0
int* pScriptsStorageEnd; // 192E4
void* CLEOOpcodesStorage; // 219B20
void** (*LookupForOpcodeFunc)(void* storage, uint16_t& opcode); // CE88
void AddGXTLabel(const char* gxtLabel, const char* text);

// Game pointers
void** ppActiveScripts, **ppIdleScripts;
void (*RemoveScriptFromList)(void* handle, void** list);
void (*AddScriptToList)(void* handle, void** list);
void (*ShutdownThisScript)(void* handle);
void (*SetSprite2dTexture)(GTASprite2D&, const char*);
void (*DrawSprite2d)(GTASprite2D&,float*,uint32_t*);
void (*DrawRectSprite2d)(GTASprite2D&,float*,uint32_t*);
void (*DrawRotatedSprite2d)(GTASprite2D&,float,float,float,float,float,float,float,float,uint32_t*);
int (*FindTxdSlot)(const char*);
void (*PushCurrentTxd)();
void (*SetCurrentTxd)(int, const char*);
void (*PopCurrentTxd)();
GTASprite2D *ScriptSprites, *ScriptSpritesOrg;
GTAScriptHandler* m_aDefaultOpcodeFuncs = NULL;
int g_nMaxScriptsCount = 96;

// CLEO itself
extern unsigned char cleoData[100160];

// CLEO crashlogging
#define SCRIPTS_LOG_COUNT 32
bool scriptDebugger = false;
void *lastScriptHandle[SCRIPTS_LOG_COUNT] = { NULL };
uint8_t *lastScriptPC[SCRIPTS_LOG_COUNT] =  { NULL };
uint16_t lastScriptOp[SCRIPTS_LOG_COUNT] =  { 0x0000 };

// Config-functions
const char* pLocations[] = 
{
    "CLEO 2.0.1",
    "Old CLEO",
    "Old CLEO (+cleo)",
    "../files/CLEO",
};
const char* pYesNo[] = 
{
    "FEM_OFF",
    "FEM_ON",
};
void OnLocationChanged(int oldVal, int newVal, void* userdata)
{
    pCfgCLEOLocation->SetInt(newVal);
    cfg->Save();
}
void OnRedArrowChanged(int oldVal, int newVal, void* userdata)
{
    pCfgCLEORedArrow->SetBool(newVal != 0);
    cfg->Save();
}
void RemoveScript(void* handle)
{
    if(!handle) return;

    RemoveScriptFromList(handle, ppActiveScripts);
    if(GetAddonInfo(handle).parentThread)
    {
        // TODO: bring on threads?
    }
    else if(GetAddonInfo(handle).isCustom)
    {
        auto& threads = GetAddonInfo(handle).childThreads;
        if(threads.size() > 0)
        {
            for(auto thread : threads)
            {
                RemoveScript(thread);
            }
            threads.clear();
        }

        GetActiveFlag(handle) = false;
        int size = GetScriptsStorageSize();
        for(int i = 0; i < size; ++i)
        {
            int storageItem = *(int*)(*pScriptsStorage + i * 4);
            if(handle == *(void**)(storageItem + 28))
            {
                // TODO: check if this is enough
                *(bool*)(storageItem + 44) = false; // not launched
                GetWakeTime(handle) = 0xFFFFFFFF;
                return;
            }
        }
    }
    else
    {
        AddScriptToList(handle, ppIdleScripts);
        if(*nGameIdent == GTASA)
        {
            ShutdownThisScript(handle);
        }
        else
        {
            GetActiveFlag(handle) = false;
        }
    }
}
void NoneFunctionLogic(uintptr_t) { return; }

extern "C" __attribute__((target("thumb-mode"))) __attribute__((naked)) void Opcode0DD2_inject()
{
    //see https://github.com/XMDS/OP_0DD2FixAsm_call.git (cleo verison)

    __asm volatile(
    ".thumb\n"
        "PUSH {R4-R7, LR}\n"
        "MOV R4, R0\n"          // R0 = pointers to the first 4 parameters of the function
        "MOV R5, R1\n"          // R1 = function addr 
        "MOVS R6, #0x10\n"      // It starts from R4
        "MOVS R7, #0\n"
        "SUB SP, #0xB8\n"       // The maximum setting of the stack is 46 parameters

        "loc_1:\n"
        "CMP R7, #0xB8\n"
        "BEQ loc_2\n"
        "LDR R1, [R0, R6]\n"    // Read parameters from reg in 0DD3 in cleo. It starts from R4
        "STR.W R1, [SP, R7]\n"  // Write the extracted parameters to the stack
        "ADDS R6, #4\n"         // Next parameter
        "ADDS R7, #4\n"         // Stack +4 to save the next parameter
        "B loc_1\n"

        "loc_2:\n"
        "LDR R0, [R4]\n"        // 0DD3 context_set_reg 0
        "LDR R1, [R4, #4]\n"    // 0DD3 context_set_reg 1
        "LDR R2, [R4, #8]\n"    // 0DD3 context_set_reg 2
        "LDR R3, [R4, #0xC]\n"  // 0DD3 context_set_reg 3
        "BLX R5\n"              // 0DD2 call func
        "STR R0, [R4]\n"        // 0DD4 return value 
        "ADD SP, #0xB8\n"
        "POP {R4-R7, PC}\n"
    );
}

DECL_HOOKb(CLEO_ExecuteCustomOpcode, void* handle, void** pcPointerPtr, uint16_t opcode)
{
    GetNotFlag(handle) = (opcode & 0x8000) != 0;
    return CLEO_ExecuteCustomOpcode(handle, pcPointerPtr, opcode & 0x7FFF);
}
extern int* ScriptParams;
void ScmCleanup();
DECL_HOOKv(CLEO_StartScripts)
{
    // Reset a number of addons.
    FreeScriptAddonInfoId = 1;

    uintptr_t basicScriptHandles = *(uintptr_t*)(nGameAddr + ValueForGame(0, 0x395C48, 0x679658));
    for(int i = 0; i < g_nMaxScriptsCount; ++i)
    {
        void* handle = (void*)(basicScriptHandles + i * ValueForGame(0x88, 0x88, 0x100));
        AssignAddonInfo(handle);
    }
    CLEO_StartScripts();
}
int lastStorageItem = 0;
DECL_HOOKb(CLEO_OnOpcodeCall, int thisStorageItem, uint16_t opcode)
{
    lastStorageItem = thisStorageItem;
    bool ret = CLEO_OnOpcodeCall(thisStorageItem, opcode);
    int param1 = *ScriptParams;
    
    if(opcode == 0x0DF0)
    {
        // Init cleo variables
        ScmCleanup();
    }
    /*if(opcode == 0x0DEF)
    {
        // Launch CSI script from menu
        int len = GetScriptsStorageSize();
        for(int i = 0; i < len; ++i)
        {
            int storageItem = *(int*)(*pScriptsStorage + i * 4);
            if(storageItem && *(int*)(storageItem + 24) != -1 && *(int*)(storageItem + 24) == param1)
            {
                void* handle = *(void**)(storageItem + 28);
                if(handle != NULL)
                {
                    AssignAddonInfo(handle);
                    GetAddonInfo(handle).parentThread = NULL;
                    GetAddonInfo(handle).isCustom = true;
                }
                return ret;
            }
        }
    }*/
    return ret;
}

void* g_pForceInterrupt = NULL;
DECL_HOOK(int8_t, ProcessOneCommand, void* handle)
{
    if(scriptDebugger)
    {
        for(int i = SCRIPTS_LOG_COUNT-2; i >= 0; --i)
        {
            lastScriptHandle[i + 1] = lastScriptHandle[i];
            lastScriptPC[i + 1] = lastScriptPC[i];
            lastScriptOp[i + 1] = lastScriptOp[i];
        }
        lastScriptHandle[0] = handle;
        lastScriptPC[0] = GetPC(handle);
        lastScriptOp[0] = Read2Bytes_NoSkip(handle);// & 0x7FFF;
    }
    
    int siz = pausedScripts.size();
    for (size_t i = 0; i < siz; ++i)
    {
        if (pausedScripts[i].ptr == handle)
        {
            return 1; // script paused, do not process
        }
    }
    
    int8_t retCode = ProcessOneCommand(handle);
    if(g_pForceInterrupt && g_pForceInterrupt == handle)
    {
        g_pForceInterrupt = NULL;
        return 1;
    }
    return retCode;
}

void* g_pLastScriptHandleStarted = NULL;
DECL_HOOK(void*, CLEO_StartSingleCustomScript, uint8_t* pc)
{
    g_pLastScriptHandleStarted = CLEO_StartSingleCustomScript(pc);
    
    AssignAddonInfo(g_pLastScriptHandleStarted);
    GetAddonInfo(g_pLastScriptHandleStarted).isCustom = true;

    return g_pLastScriptHandleStarted;
}

static bool bDontCallDefaultThisFrame = false;
extern void DrawSingleRect(void* handle, CustomScriptRect& rt);
DECL_HOOKv(DrawScriptStuff, uint8_t bBeforeFade)
{
    if(!bDontCallDefaultThisFrame)
    {
        DrawScriptStuff(bBeforeFade);
    }
    bDontCallDefaultThisFrame = false;

    void* foundHandle;
    int size = GetScriptsStorageSize();
    for(int i = 0; i < size; ++i)
    {
        int storageItem = *(int*)(*pScriptsStorage + i * 4);
        foundHandle = *(void**)(storageItem + 28);
        if(foundHandle && GetActiveFlag(foundHandle))
        {
            auto& ai = GetAddonInfo(foundHandle);
            for(int i = ai.scriptRectsThisFrame - 1; i >= 0; --i)
            {
                CustomScriptRect& rt = ai.scriptRects[i];
                if(rt.beforeFade == bBeforeFade) DrawSingleRect(foundHandle, rt);
            }
        }
    }
}
DECL_HOOKv(GTAVC_DrawBeforeFade)
{
    GTAVC_DrawBeforeFade();
    
    bDontCallDefaultThisFrame = true;
    HookOf_DrawScriptStuff(true);
}
DECL_HOOKv(GTAVC_DrawAfterFade, void* a, void* b)
{
    GTAVC_DrawAfterFade(a, b);
    
    bDontCallDefaultThisFrame = true;
    HookOf_DrawScriptStuff(false);
}

DECL_HOOKi(ProcessScript, void* handle)
{
    GetAddonInfo(handle).OnScriptProcess();
    return ProcessScript(handle);
}

void SAUtilsStarted()
{
    snprintf(szCLEOVer, sizeof(szCLEOVer), "CLEOMod v%s", modinfo->VersionString());
    sautils->AddButton(SetType_Mods, szCLEOVer, NoneFunctionLogic);
    sautils->AddClickableItem(SetType_Game, "CLEO Location", pCfgCLEOLocation->GetInt(), 0, sizeofA(pLocations)-1, pLocations, OnLocationChanged, NULL);
    sautils->AddClickableItem(SetType_Game, "CLEO Red Arrow", pCfgCLEORedArrow->GetInt(), 0, sizeofA(pYesNo)-1, pYesNo, OnRedArrowChanged, NULL);
}

ON_MOD_PRELOAD()
{
    logger->SetTag("CLEO Mod");
    pCfgCLEOLocation = cfg->Bind("CLEO_Location", 1);
    pCfgCLEORedArrow = cfg->Bind("CLEO_RedArrow", true);
    pCfgCLEOMenuColor = cfg->Bind("CLEO_MenuColor", "55 127 175 150");
    pCfgCLEOMenuArrowColor = cfg->Bind("CLEO_MenuArrowColor", "55 127 175 100");
    pCfgCLEOMenuArrowPressedAlpha = cfg->Bind("CLEO_MenuArrowPressedAlpha", "180");
    scriptDebugger = cfg->GetBool("ScriptDebugger", scriptDebugger);
    
    pCLEO = dlopen("libcleo.so", RTLD_LAZY);
    if(!pCLEO)
    {
        char szLoadFrom[256];
        snprintf(szLoadFrom, sizeof(szLoadFrom), "%s/libcleo.mod.so", aml->GetDataPath());

        std::ofstream fs(szLoadFrom, std::ios::out | std::ios::binary);
        fs.write((const char*)cleoData, sizeof(cleoData));
        fs.flush();
        fs.close();
        pCLEO = dlopen(szLoadFrom, RTLD_NOW);
    }
 
    if(!pCLEO)
    {
      OOPSIE:
        logger->Error("Failed to load CLEO library!");
        return;
    }
    
    auto libEntry = (void(*)())dlsym(pCLEO, "JNI_OnLoad");
    if(!libEntry) goto OOPSIE; // How?

    dladdr((void*)libEntry, &pDLInfo);
    nCLEOAddr = (uintptr_t)pDLInfo.dli_fbase;
    cleo = (cleo_ifs_t*)(nCLEOAddr + 0x219AA8); // VTable = 0xC382
    nGameIdent = (eGameIdent*)(nCLEOAddr + 0x19298);
    if(pCfgCLEOLocation->GetInt() == 1)
    {
        char tmp[256];
        snprintf(tmp, sizeof(tmp), "%s", aml->GetAndroidDataPath());
        __pathback(tmp);
        setenv("EXTERNAL_STORAGE", tmp, 1);
        
      SET_LOAD_DIRECTLY:
        aml->Write8(nCLEOAddr + 0x146A9 + 3, 0x00);
        aml->Write(nCLEOAddr + 0x14C2C + 7, ".log", 5);
    }
    else if(pCfgCLEOLocation->GetInt() == 2)
    {
        char tmp[256];
        snprintf(tmp, sizeof(tmp), "%s", aml->GetAndroidDataPath());
        __pathback(tmp);
        setenv("EXTERNAL_STORAGE", tmp, 1);
        snprintf(tmp, sizeof(tmp), "%s/cleo", tmp);
        mkdir(tmp, 0777);
        
        aml->Write8(nCLEOAddr + 0x146A9 + 8, 0x00);
    }
    else if(pCfgCLEOLocation->GetInt() == 3)
    {
        char tmp[256];
        snprintf(tmp, sizeof(tmp), "%s/CLEO", aml->GetAndroidDataPath());
        setenv("EXTERNAL_STORAGE", tmp, 1);
        mkdir(tmp, 0777);
        
        goto SET_LOAD_DIRECTLY;
    }

    if(!pCfgCLEORedArrow->GetBool())
    {
        aml->PlaceNOP(nCLEOAddr + 0xBD82, 2);
    }
        
    // XMDS Part 1
    // Fixed OPCODE 0DD2
    aml->Redirect(nCLEOAddr + 0x4EB8 + 0x1, (uintptr_t)Opcode0DD2_inject);
        
    // CLEO Menu Color
    SET_TO(pCLEOMenuColor, nCLEOAddr + 0x1525C);
    aml->Unprot((uintptr_t)pCLEOMenuColor, sizeof(rgba_t));
    *pCLEOMenuColor = pCfgCLEOMenuColor->ParseColor();
    
    SET_TO(pCLEOMenuArrowColor, nCLEOAddr + 0x15250);
    aml->Unprot((uintptr_t)pCLEOMenuArrowColor, sizeof(rgba_t));
    *pCLEOMenuArrowColor = pCfgCLEOMenuColor->ParseColor();

    SET_TO(pCLEOArrowLastAlpha, nCLEOAddr + 0x2194FC);
    aml->Unprot((uintptr_t)pCLEOArrowLastAlpha, sizeof(uint8_t));
    *pCLEOArrowLastAlpha = pCfgCLEOMenuArrowPressedAlpha->GetInt();

    SET_TO(pScriptsStorage, nCLEOAddr + 0x192E0);
    SET_TO(pScriptsStorageEnd, nCLEOAddr + 0x192E4);
    SET_TO(CLEOOpcodesStorage, nCLEOAddr + 0x219B20);
    SET_TO(LookupForOpcodeFunc, nCLEOAddr + 0xCE88 + 0x1);
    HOOK(CLEO_ExecuteCustomOpcode, nCLEOAddr + 0xD2F8 + 0x1);
    HOOK(CLEO_StartScripts, nCLEOAddr + 0x5CD8 + 0x1);
    HOOK(CLEO_OnOpcodeCall, nCLEOAddr + 0x75B4 + 0x1);
    
    // Start CLEO
    libEntry();
    RegisterInterface("CLEO", cleo);
    logger->Info("CLEO Initialized!");

    // CleoAddon interface == 1
    cleo_addon_ifs.GetInterfaceVersion =    GetAddonIncludeInterfaceVersion;
    cleo_addon_ifs.ReadString =             CLEO_ReadStringEx;
    cleo_addon_ifs.WriteString =            CLEO_WriteStringEx;
    cleo_addon_ifs.GetStringMaxSize =       CLEO_GetStringPtrMaxSize;
    cleo_addon_ifs.GetStringPointer =       CLEO_GetStringPtr;
    cleo_addon_ifs.FormatString =           CLEO_FormatString;
    cleo_addon_ifs.AsciiToGXTChar =         AsciiToGXTChar;
    cleo_addon_ifs.GXTCharToAscii =         GXTCharToAscii;
    cleo_addon_ifs.ValueForGame =           ValueForGame;
    cleo_addon_ifs.ThreadJump =             ThreadJump;
    cleo_addon_ifs.SkipUnusedParameters =   SkipUnusedParameters;
    cleo_addon_ifs.GetScriptPC =            GetRealPC;
    cleo_addon_ifs.PushStack =              PushStack;
    cleo_addon_ifs.PopStack =               PopStack;
    cleo_addon_ifs.GetCond =                GetCond;
    cleo_addon_ifs.GetNotFlag =             GetNotFlag;
    cleo_addon_ifs.GetLogicalOp =           GetLogicalOp;
    cleo_addon_ifs.Interrupt =              [](void *handle)
    {
        g_pForceInterrupt = handle;
    };
    cleo_addon_ifs.Skip1Byte =              Skip1Byte;
    cleo_addon_ifs.Skip2Bytes =             Skip2Bytes;
    cleo_addon_ifs.Skip4Bytes =             Skip4Bytes;
    cleo_addon_ifs.SkipBytes =              SkipBytes;
    cleo_addon_ifs.Read1Byte =              Read1Byte;
    cleo_addon_ifs.Read2Bytes =             Read2Bytes;
    cleo_addon_ifs.Read4Bytes =             Read4Bytes;
    cleo_addon_ifs.Read1Byte_NoSkip =       Read1Byte_NoSkip;
    cleo_addon_ifs.Read2Bytes_NoSkip =      Read2Bytes_NoSkip;
    cleo_addon_ifs.Read4Bytes_NoSkip =      Read4Bytes_NoSkip;
    cleo_addon_ifs.GetLocalVars =           GetLocalVars;
    cleo_addon_ifs.GetPC =                  GetPC;
    cleo_addon_ifs.SkipOpcodeParameters =   SkipOpcodeParameters;
    cleo_addon_ifs.GetVarArgCount =         GetVarArgCount;
    cleo_addon_ifs.GetAddonInfo =           GetAddonInfo;
    cleo_addon_ifs.UpdateCompareFlag =      [](void* handle, uint8_t flag)
    {
        UpdateCompareFlag(handle, flag);
    };
    cleo_addon_ifs.IsOpcodeAlreadyExists =  [](uint16_t opcode) -> bool
    {
        uint16_t clamped_opcode = (opcode & 0x7FFF);
        void** fn = LookupForOpcodeFunc(CLEOOpcodesStorage, clamped_opcode);
        return (fn != NULL && *fn != NULL);
    };
    cleo_addon_ifs.IsValidScriptHandle =    IsValidScriptHandle;
    cleo_addon_ifs.ResolvePath =            ResolvePath;
    cleo_addon_ifs.AddGXTLabel =            AddGXTLabel;

    // CleoAddon interface == 2
    cleo_addon_ifs.GetActiveFlag =          GetActiveFlag;
    cleo_addon_ifs.IsInActiveScripts =      IsInActiveScripts;
    cleo_addon_ifs.IsInPausedScripts =      IsInPausedScripts;
    cleo_addon_ifs.IsInCLEOScripts =        IsInCLEOScripts;
    cleo_addon_ifs.IsParamString =          IsParamString;
    cleo_addon_ifs.GetScriptFilename =      CLEO_GetScriptFilename;
    cleo_addon_ifs.GetVarTypeName =         [](int varType) -> const char*
    {
        return GetVarTypeName((eScriptParameterType)varType);
    };
    cleo_addon_ifs.GetStringPtr =           CLEO_GetStringPtr;
    cleo_addon_ifs.GetStringPtrMaxSize =    CLEO_GetStringPtrMaxSize;
    cleo_addon_ifs.IsMissionScript =        IsMissionScript;
    cleo_addon_ifs.ReadStdString =          CLEO_ReadStdString;
    cleo_addon_ifs.GetLastCustomScriptCreated = []() -> void*
    {
        return g_pLastScriptHandleStarted;
    };
    cleo_addon_ifs.GetWakeTime =            GetWakeTime;
    cleo_addon_ifs.GetScriptTextureByID =   GetCLEOSpriteTexture;
    cleo_addon_ifs.SetScriptTextureByID =   SetCLEOSpriteTexture;
    cleo_addon_ifs.IsScriptCustom =         IsScriptCustom;
    cleo_addon_ifs.CallDefaultOpcode =      CallDefaultOpcode;

    // CleoAddon interface == 3
    cleo_addon_ifs.SetPrivateVar =          SetPrivateVar;
    cleo_addon_ifs.GetPrivateVar =          GetPrivateVar;

    // Finalize
    RegisterInterface("CLEOAddon", &cleo_addon_ifs);
    logger->Info("CLEO Addon Initialized!");
}

ON_MOD_LOAD()
{
    sautils = (ISAUtils*)GetInterface("SAUtils");
    if(sautils) SAUtilsStarted();
}

static char sCleoDir[512] { 0 };
const char* GetCLEODir()
{
    if(!sCleoDir[0])
    {
        char pad[24];
        char* (*CLEO_GetDir)(char*);
        SET_TO(CLEO_GetDir, nCLEOAddr + 0x607C + 0x1);
        CLEO_GetDir(&pad[0]);
        strcpy(sCleoDir, *(char**)(pad + 20));
    }
    return sCleoDir;
}

CLEO_Fn(AML_HAS_MOD_LOADED)
{
    char modname[92];
    CLEO_ReadStringEx(handle, modname, sizeof(modname));

    bool hasMod = aml->HasMod(modname);
    cleo->GetPointerToScriptVar(handle)->i = hasMod;
    UpdateCompareFlag(handle, hasMod);
}
CLEO_Fn(AML_HAS_MODVER_LOADED)
{
    char modname[92], modver[24];
    CLEO_ReadStringEx(handle, modname, sizeof(modname));
    CLEO_ReadStringEx(handle, modver, sizeof(modver));

    bool hasMod = aml->HasModOfVersion(modname, modver);
    cleo->GetPointerToScriptVar(handle)->i = hasMod;
    UpdateCompareFlag(handle, hasMod);
}
CLEO_Fn(AML_REDIRECT_CODE)
{
    uintptr_t code1 = cleo->ReadParam(handle)->u;
    if(cleo->ReadParam(handle)->i != 0) code1 += nGameAddr;
    uintptr_t code2 = cleo->ReadParam(handle)->u;
    if(cleo->ReadParam(handle)->i != 0) code2 += nGameAddr;

    aml->Redirect(code1, code2);
}
CLEO_Fn(AML_JUMP_CODE)
{
    uintptr_t code1 = cleo->ReadParam(handle)->u;
    if(cleo->ReadParam(handle)->i != 0) code1 += nGameAddr;
    uintptr_t code2 = cleo->ReadParam(handle)->u;
    if(cleo->ReadParam(handle)->i != 0) code2 += nGameAddr;

    aml->PlaceB(code1, code2);
}
CLEO_Fn(AML_GET_BRANCH_DEST)
{
    uintptr_t code = cleo->ReadParam(handle)->u;
    if(cleo->ReadParam(handle)->i != 0) code += nGameAddr;
    
    cleo->GetPointerToScriptVar(handle)->i = aml->GetBranchDest(code);
}
CLEO_Fn(AML_MLS_SAVE)
{
    aml->MLSSaveFile();
}
CLEO_Fn(AML_MLS_HAS_VALUE)
{
    char key[16];
    CLEO_ReadStringEx(handle, key, sizeof(key));
    UpdateCompareFlag(handle, aml->MLSHasValue(key));
}
CLEO_Fn(AML_MLS_DELETE_VALUE)
{
    char key[16];
    CLEO_ReadStringEx(handle, key, sizeof(key));
    aml->MLSDeleteValue(key);
}
CLEO_Fn(AML_MLS_SET_INT)
{
    char key[16];
    CLEO_ReadStringEx(handle, key, sizeof(key));
    aml->MLSSetInt(key, cleo->ReadParam(handle)->i);
}
CLEO_Fn(AML_MLS_SET_FLOAT)
{
    char key[16];
    CLEO_ReadStringEx(handle, key, sizeof(key));
    aml->MLSSetFloat(key, cleo->ReadParam(handle)->f);
}
CLEO_Fn(AML_MLS_SET_STRING)
{
    char key[16];
    CLEO_ReadStringEx(handle, key, sizeof(key));
    char value[128];
    CLEO_ReadStringEx(handle, value, sizeof(value));
    aml->MLSSetStr(key, value);
}
CLEO_Fn(AML_MLS_GET_INT)
{
    char key[16];
    CLEO_ReadStringEx(handle, key, sizeof(key));
    int value = cleo->ReadParam(handle)->i;
    aml->MLSGetInt(key, &value);
    cleo->GetPointerToScriptVar(handle)->i = value;
}
CLEO_Fn(AML_MLS_GET_FLOAT)
{
    char key[16];
    CLEO_ReadStringEx(handle, key, sizeof(key));
    float value = cleo->ReadParam(handle)->f;
    aml->MLSGetFloat(key, &value);
    cleo->GetPointerToScriptVar(handle)->f = value;
}
CLEO_Fn(AML_MLS_GET_STRING)
{
    char key[16];
    CLEO_ReadStringEx(handle, key, sizeof(key));
    char value[16];
    CLEO_ReadStringEx(handle, value, sizeof(value));
    aml->MLSGetStr(key, value, sizeof(value));
    CLEO_WriteStringEx(handle, value);
}
CLEO_Fn(AML_DO_OPCODE_EXIST)
{
    uint16_t op = ((uint16_t)cleo->ReadParam(handle)->i) & 0x7FFF;
    void** fn = LookupForOpcodeFunc(CLEOOpcodesStorage, op);
    UpdateCompareFlag(handle, fn != NULL && *fn != NULL);
}
CLEO_Fn(AML_PUSH_STRING_TO_VAR)
{
    char buf[MAX_STR_LEN];
    CLEO_ReadStringEx(handle, buf, sizeof(buf));
    CLEO_WriteStringEx(handle, buf);
}
CLEO_Fn(AML_WRITE_FLOAT)
{
    float val = cleo->ReadParam(handle)->f;
    uintptr_t addr = cleo->ReadParam(handle)->u;
    if(cleo->ReadParam(handle)->i != 0) addr += nGameAddr;
    aml->WriteFloat(addr, val);
}
CLEO_Fn(AML_VIBRATE)
{
    int ms = cleo->ReadParam(handle)->i;
    aml->DoVibro(ms);
}
CLEO_Fn(AML_VIBRATE_STOP)
{
    aml->CancelVibro();
}
CLEO_Fn(AML_SHOW_TOAST)
{
    bool longerDur = cleo->ReadParam(handle)->i;
    char buf[MAX_STR_LEN];
    CLEO_ReadStringEx(handle, buf, sizeof(buf));
    aml->ShowToast(longerDur, "%s", buf);
}
CLEO_Fn(AML_BATTERY_LEVEL)
{
    cleo->GetPointerToScriptVar(handle)->f = aml->GetBatteryLevel();
}
CLEO_Fn(AML_ANDROID_SDK_INT)
{
    cleo->GetPointerToScriptVar(handle)->i = aml->GetAndroidVersion();
}
CLEO_Fn(AML_WRITE_HEX)
{
    uintptr_t addr = cleo->ReadParam(handle)->u;
    if(cleo->ReadParam(handle)->i) // add_ib
    {
        addr += nGameAddr;
    }

    int labelOffset = cleo->ReadParam(handle)->i;
    int size = cleo->ReadParam(handle)->i;
    uintptr_t hexAddr = 0;

    if(size > 0) // GET_LABEL_POINTER
    {
        int storageItem = lastStorageItem;//GetCustomHandleFromScriptHandle(handle);
        if(storageItem && *(void**)(storageItem + 28) == handle)
        {
            if(labelOffset < 0) labelOffset = -labelOffset;
            hexAddr = *(uint32_t*)(storageItem + 32) + labelOffset;
        }
        else
        {
            // sadge
            int baseOffset = ValueForGame(0, 0, 16, 20, 20);
            if(baseOffset)
            {
                uint8_t* basePtr = GetBasePC(handle);
                hexAddr = (uint32_t)((labelOffset < 0) ? (basePtr - labelOffset) : (ScriptSpace + labelOffset));
            }
            else
            {
                hexAddr = (uint32_t)((labelOffset < 0) ? (ValueForGame(0x20000, 0x3F9A0, 0) - labelOffset) : labelOffset);
            }
        }
        
        if(hexAddr)
        {
            aml->Write(addr, hexAddr, size);
        }
    }
}
CLEO_Fn(AML_READ_HEX)
{
    uintptr_t addr = cleo->ReadParam(handle)->u;
    if(cleo->ReadParam(handle)->i) // add_ib
    {
        addr += nGameAddr;
    }

    int labelOffset = cleo->ReadParam(handle)->i;
    int size = cleo->ReadParam(handle)->i;
    uintptr_t hexAddr = 0;

    if(size > 0) // GET_LABEL_POINTER
    {
        int storageItem = lastStorageItem;//GetCustomHandleFromScriptHandle(handle);
        if(storageItem && *(void**)(storageItem + 28) == handle)
        {
            if(labelOffset < 0) labelOffset = -labelOffset;
            hexAddr = *(uint32_t*)(storageItem + 32) + labelOffset;
        }
        else
        {
            // sadge
            int baseOffset = ValueForGame(0, 0, 16, 20, 20);
            if(baseOffset)
            {
                uint8_t* basePtr = GetBasePC(handle);
                hexAddr = (uint32_t)((labelOffset < 0) ? (basePtr - labelOffset) : (ScriptSpace + labelOffset));
            }
            else
            {
                hexAddr = (uint32_t)((labelOffset < 0) ? (ValueForGame(0x20000, 0x3F9A0, 0) - labelOffset) : labelOffset);
            }
        }
        
        if(hexAddr)
        {
            aml->Unprot(addr);
            aml->Read(addr, hexAddr, size);
        }
    }
}
CLEO_Fn(AML_SET_PRIVATE_VAR)
{
    cleo_ifs_t::data_t value = *(cleo->ReadParam(handle));
    int idx = cleo->ReadParam(handle)->i;
    SetPrivateVar(handle, idx, value);
}
CLEO_Fn(AML_GET_PRIVATE_VAR)
{
    int idx = cleo->ReadParam(handle)->i;
    *(cleo->GetPointerToScriptVar(handle)) = GetPrivateVar(handle, idx);
}
CLEO_Fn(AML_GET_STACK_POINTER)
{
    cleo->GetPointerToScriptVar(handle)->i = (int)( GetAddonInfo(handle).GetVarStack() );
}
CLEO_Fn(AML_STACK_PUSH)
{
    GetAddonInfo(handle).PushVarToStack( cleo->ReadParam(handle)->i );
}
CLEO_Fn(AML_STACK_POP)
{
    GetAddonInfo(handle).PopVarFromStack( cleo->GetPointerToScriptVar(handle)->i );
}
CLEO_Fn(AML_STACK_ONLYPOP)
{
    GetAddonInfo(handle).PopStack();
}
CLEO_Fn(AML_STACK_ALLOC)
{
    int bytes = cleo->ReadParam(handle)->i;
    cleo->GetPointerToScriptVar(handle)->i = (int)( GetAddonInfo(handle).AllocateFromStack(bytes) );
}
CLEO_Fn(AML_STACK_DEALLOC)
{
    int bytes = cleo->ReadParam(handle)->i;
    GetAddonInfo(handle).DeallocateFromStack(bytes);
}

void Init201Opcodes();
void Init4Opcodes();
void Init5Opcodes();
void InitMathOpcodes();
char g_ScriptStore[256 * 0x100]; // 0x100 is the size of script in GTA:SA
                                 // (VC has smaller size=0x88 so it's fine to use BIGGER static value)
char g_ScriptSpritesStore[4 * 1024] { 0 }; // 
char g_ScriptRectsStore[60 * 1024] { 0 }; // 
ON_ALL_MODS_LOAD()
{
    if(!cleo) return;

    nGameAddr = (uintptr_t)cleo->GetMainLibraryLoadAddress();
    
    CLEO_RegisterOpcode(0x3A00, AML_HAS_MOD_LOADED); // 3A00=2,%2d% = aml_has_mod_loaded %1s% // IF and SET
    CLEO_RegisterOpcode(0x3A01, AML_HAS_MODVER_LOADED); // 3A01=3,%3d% = aml_has_mod_loaded %1s% version %2s% // IF and SET
    CLEO_RegisterOpcode(0x3A02, AML_REDIRECT_CODE); // 3A02=4,aml_redirect_code %1d% add_ib %2d% to %3d% add_ib %4d%
    CLEO_RegisterOpcode(0x3A03, AML_JUMP_CODE); // 3A03=4,aml_jump_code %1d% add_ib %2d% to %3d% add_ib %4d%
    CLEO_RegisterOpcode(0x3A04, AML_GET_BRANCH_DEST); // 3A04=3,%3d% = aml_get_branch_dest %1d% add_ib %2d%
    CLEO_RegisterOpcode(0x3A05, AML_MLS_SAVE); // 3A05=0,aml_mls_save
    CLEO_RegisterOpcode(0x3A06, AML_MLS_HAS_VALUE); // 3A06=1,aml_mls_has_value %1s% // IF and SET
    CLEO_RegisterOpcode(0x3A07, AML_MLS_DELETE_VALUE); // 3A07=1,aml_mls_delete_value %1s%
    CLEO_RegisterOpcode(0x3A08, AML_MLS_SET_INT); // 3A08=2,aml_mls_set_int %1s% to %2d%
    CLEO_RegisterOpcode(0x3A09, AML_MLS_SET_FLOAT); // 3A09=2,aml_mls_set_float %1s% to %2d%
    CLEO_RegisterOpcode(0x3A0A, AML_MLS_SET_STRING); // 3A0A=2,aml_mls_set_string %1s% to %2s%
    CLEO_RegisterOpcode(0x3A0B, AML_MLS_GET_INT); // 3A0B=3,%3d% = aml_mls_get_int %1s% default %2d%
    CLEO_RegisterOpcode(0x3A0C, AML_MLS_GET_FLOAT); // 3A0C=3,%3d% = aml_mls_get_float %1s% default %2d%
    CLEO_RegisterOpcode(0x3A0D, AML_MLS_GET_STRING); // 3A0D=3,%3s% = aml_mls_get_string %1s% default %2s%
    CLEO_RegisterOpcode(0x3A0E, AML_DO_OPCODE_EXIST); // 3A0E=1,do_opcode_exist %1d% // IF and SET
    CLEO_RegisterOpcode(0x3A0F, AML_PUSH_STRING_TO_VAR); // 3A0F=2,push_string %1d% to_var %2d%
    CLEO_RegisterOpcode(0x3A10, AML_WRITE_FLOAT); // 3A10=3,write_float %1d% to %2d% add_ib %3d%
    CLEO_RegisterOpcode(0x3A11, AML_VIBRATE); // 3A11=1,aml_vibrate %1d% ms
    CLEO_RegisterOpcode(0x3A12, AML_VIBRATE_STOP); // 3A12=0,aml_stop_vibro
    CLEO_RegisterOpcode(0x3A13, AML_SHOW_TOAST); // 3A13=2,aml_show_toast %2s% longer %1d%
    CLEO_RegisterOpcode(0x3A14, AML_BATTERY_LEVEL); // 3A14=1,%1d% = aml_get_battery_percentage // float
    CLEO_RegisterOpcode(0x3A15, AML_ANDROID_SDK_INT); // 3A15=1,%1d% = aml_get_android_ver
    CLEO_RegisterOpcode(0x3A16, AML_WRITE_HEX); // 3A16=4,aml_write_hex_at %1d% add_ib %2d% from_label %3d% size %4d%
    CLEO_RegisterOpcode(0x3A17, AML_READ_HEX); // 3A17=4,aml_read_hex_at %1d% add_ib %2d% to_label %3d% size %4d%
    CLEO_RegisterOpcode(0x3A18, AML_SET_PRIVATE_VAR); // 3A18=2,aml_set_private_var %2d% = %1d%
    CLEO_RegisterOpcode(0x3A19, AML_GET_PRIVATE_VAR); // 3A19=2,%2d% = aml_get_private_var %1d%
    CLEO_RegisterOpcode(0x3A1A, AML_GET_STACK_POINTER); // 3A1A=1,%1d% = aml_get_stack_ptr
    CLEO_RegisterOpcode(0x3A1B, AML_STACK_PUSH); // 3A1B=1,aml_push_stack %1d%
    CLEO_RegisterOpcode(0x3A1C, AML_STACK_POP); // 3A1C=1,%1d% = aml_pop_stack
    CLEO_RegisterOpcode(0x3A1D, AML_STACK_ONLYPOP); // 3A1D=0,aml_only_pop_stack
    CLEO_RegisterOpcode(0x3A1E, AML_STACK_ALLOC); // 3A1E=2,%2d% = aml_alloc_stack_bytes %1d%
    CLEO_RegisterOpcode(0x3A1F, AML_STACK_DEALLOC); // 3A1F=1,aml_dealloc_stack_bytes %1d%

    // Fix Alexander Blade's ass code (returns NULL!!! BRUH)
    cleo->GetCleoStorageDir = GetCLEODir;
    cleo->GetCleoPluginLoadDir = GetCLEODir;

    // CLEO Scripts limit
    if(*nGameIdent == GTASA)
    {
        // 96 to 256
        g_nMaxScriptsCount = 96;
        if(cfg->GetBool("BumpScriptsLimit", true) &&
           *(uintptr_t*)(nGameAddr + 0x679658) == (nGameAddr + 0x7B778C))
        {
            aml->WriteAddr(nGameAddr + 0x679658, &g_ScriptStore[0]);
            aml->Write32(nGameAddr + 0x329F88, 0x3F80F5B5);
            g_nMaxScriptsCount = 256;
        }

        // We're gonna force that patch. More textures, more possibilities (before we get stuff per script)
        /*if(cfg->GetBool("BumpScriptTexturesLimit", true) &&
           *(uintptr_t*)(nGameAddr + 0x678EAC) == (nGameAddr + 0x8194DC))*/
        {
            aml->WriteAddr(nGameAddr + 0x678EAC, &g_ScriptSpritesStore[0]);
            aml->WriteAddr(nGameAddr + 0x67915C, &g_ScriptRectsStore[0]);
            aml->Write32(nGameAddr + 0x327E6E, 0x5F80F5B6); // CMissionCleanup::Process
            aml->Write32(nGameAddr + 0x328298, 0x5F80F5B4); // CTheScripts::RemoveScriptTextureDictionary
            aml->Write32(nGameAddr + 0x32A638, 0x5F80F5B4); // CTheScripts::Init
            aml->Write32(nGameAddr + 0x1A3736, 0x74FCF640); // sub_1A3730
            aml->Write32(nGameAddr + 0x1A37F4, 0x5F80F5B5); // sub_1A3750
            aml->Write32(nGameAddr + 0x329E6C, 0x4F70F5B8); // CTheScripts::DrawScriptSpritesAndRectangles
            aml->Write32(nGameAddr + 0x32A5FC, 0x4F70F5B6); // CTheScripts::Init
            aml->Write32(nGameAddr + 0x32B040, 0x4F70F5B2); // CTheScripts::Process
        }
    }
    else if(*nGameIdent == GTAVC)
    {
        // 128 to 256
        g_nMaxScriptsCount = 128;
        if(cfg->GetBool("BumpScriptsLimit", true) &&
           *(uintptr_t*)(nGameAddr + 0x395C48) == (nGameAddr + 0x58F018))
        {
            aml->WriteAddr(nGameAddr + 0x395C48, &g_ScriptStore[0]);
            aml->Write32(nGameAddr + 0x10B658, 0x4708F504);
            g_nMaxScriptsCount = 256;
        }
    }

    // CLEO Scripts binary storage limit
    // from 2 megabytes to 16
    aml->WriteAddr(nCLEOAddr + 0x193AC, (uintptr_t)&g_ScriptBytesBuffer[0]);
    aml->Write32(nCLEOAddr + 0x6422, 0x9118F8D3);

    // CLEO4+5 Opcodes
    sprintf(g_szSavesPath, "%s/sav", cleo->GetCleoStorageDir());
    Init201Opcodes();
    Init4Opcodes();
    Init5Opcodes();

    HOOK(ProcessOneCommand, cleo->GetMainLibrarySymbol("_ZN14CRunningScript17ProcessOneCommandEv"));
    HOOKPLT(CLEO_StartSingleCustomScript, nCLEOAddr + 0x1933C);

    SET_TO(RemoveScriptFromList, cleo->GetMainLibrarySymbol("_ZN14CRunningScript20RemoveScriptFromListEPPS_"));
    SET_TO(AddScriptToList, cleo->GetMainLibrarySymbol("_ZN14CRunningScript15AddScriptToListEPPS_"));
    SET_TO(ShutdownThisScript, cleo->GetMainLibrarySymbol("_ZN14CRunningScript18ShutdownThisScriptEv"));
    SET_TO(SetSprite2dTexture, cleo->GetMainLibrarySymbol("_ZN9CSprite2d10SetTextureEPc"));
    SET_TO(DrawSprite2d, cleo->GetMainLibrarySymbol("_ZN9CSprite2d4DrawERK5CRectRK5CRGBA"));
    SET_TO(DrawRectSprite2d, cleo->GetMainLibrarySymbol("_ZN9CSprite2d8DrawRectERK5CRectRK5CRGBA"));
    SET_TO(DrawRotatedSprite2d, cleo->GetMainLibrarySymbol("_ZN9CSprite2d4DrawEffffffffRK5CRGBA"));
    SET_TO(FindTxdSlot, cleo->GetMainLibrarySymbol("_ZN9CTxdStore11FindTxdSlotEPKc"));
    SET_TO(PushCurrentTxd, cleo->GetMainLibrarySymbol("_ZN9CTxdStore14PushCurrentTxdEv"));
    SET_TO(SetCurrentTxd, cleo->GetMainLibrarySymbol("_ZN9CTxdStore13SetCurrentTxdEiPKc"));
    if(!SetCurrentTxd) SET_TO(SetCurrentTxd, cleo->GetMainLibrarySymbol("_ZN9CTxdStore13SetCurrentTxdEi"));
    SET_TO(PopCurrentTxd, cleo->GetMainLibrarySymbol("_ZN9CTxdStore13PopCurrentTxdEv"));

    SET_TO(ppActiveScripts, cleo->GetMainLibrarySymbol("_ZN11CTheScripts14pActiveScriptsE"));
    SET_TO(ppIdleScripts, cleo->GetMainLibrarySymbol("_ZN11CTheScripts12pIdleScriptsE"));
    SET_TO(ScriptSprites, *(void**)(nGameAddr + (*nGameIdent == GTASA ? 0x678EAC : 0x3945A4)));
    ScriptSpritesOrg = ScriptSprites;
    if(*nGameIdent == GTASA)
    {
        SET_TO(m_aDefaultOpcodeFuncs, nGameAddr + 0x665594);
      #ifdef SCRIPTS_UNIQUE_SPRITE_IDS
        HOOK(DrawScriptStuff, cleo->GetMainLibrarySymbol("_ZN4CHud14DrawScriptTextEh"));
      #endif
        HOOKPLT(ProcessScript, nGameAddr + 0x670A9C);
    }
    else if(*nGameIdent == GTAVC)
    {
        static GTAScriptHandler customHandler[15];
        m_aDefaultOpcodeFuncs = &customHandler[0];

        SET_TO(customHandler[0], cleo->GetMainLibrarySymbol("_ZN14CRunningScript20ProcessCommands0To99Ei"));
        SET_TO(customHandler[1], cleo->GetMainLibrarySymbol("_ZN14CRunningScript23ProcessCommands100To199Ei"));
        SET_TO(customHandler[2], cleo->GetMainLibrarySymbol("_ZN14CRunningScript23ProcessCommands200To299Ei"));
        SET_TO(customHandler[3], cleo->GetMainLibrarySymbol("_ZN14CRunningScript23ProcessCommands300To399Ei"));
        SET_TO(customHandler[4], cleo->GetMainLibrarySymbol("_ZN14CRunningScript23ProcessCommands400To499Ei"));
        SET_TO(customHandler[5], cleo->GetMainLibrarySymbol("_ZN14CRunningScript23ProcessCommands500To599Ei"));
        SET_TO(customHandler[6], cleo->GetMainLibrarySymbol("_ZN14CRunningScript23ProcessCommands600To699Ei"));
        SET_TO(customHandler[7], cleo->GetMainLibrarySymbol("_ZN14CRunningScript23ProcessCommands700To799Ei"));
        SET_TO(customHandler[8], cleo->GetMainLibrarySymbol("_ZN14CRunningScript23ProcessCommands800To899Ei"));
        SET_TO(customHandler[9], cleo->GetMainLibrarySymbol("_ZN14CRunningScript23ProcessCommands900To999Ei"));
        SET_TO(customHandler[10], cleo->GetMainLibrarySymbol("_ZN14CRunningScript25ProcessCommands1000To1099Ei"));
        SET_TO(customHandler[11], cleo->GetMainLibrarySymbol("_ZN14CRunningScript25ProcessCommands1100To1199Ei"));
        SET_TO(customHandler[12], cleo->GetMainLibrarySymbol("_ZN14CRunningScript25ProcessCommands1200To1299Ei"));
        SET_TO(customHandler[13], cleo->GetMainLibrarySymbol("_ZN14CRunningScript25ProcessCommands1300To1399Ei"));
        SET_TO(customHandler[14], cleo->GetMainLibrarySymbol("_ZN14CRunningScript25ProcessCommands1400To1499Ei"));

      #ifdef SCRIPTS_UNIQUE_SPRITE_IDS
        HOOKBL(GTAVC_DrawBeforeFade, nGameAddr + 0x1E9112);
        HOOKBL(GTAVC_DrawAfterFade, nGameAddr + 0x1ECA46);
      #endif
        HOOK(ProcessScript, cleo->GetMainLibrarySymbol("_ZN14CRunningScript7ProcessEv"));
    }

    // MathOperations Opcodes
    InitMathOpcodes();

    // DMA Fix (only in GTA:SA!)
    if(*nGameIdent == GTASA)
    {
        aml->Write8(nGameAddr + 0x32950A + 0x1, 0x68);
    }
}

ON_MOD_UNLOAD()
{
    delete pCfgCLEOLocation;
    delete pCfgCLEORedArrow;
    delete pCfgCLEOMenuColor;
    delete pCfgCLEOMenuArrowColor;
    delete pCfgCLEOMenuArrowPressedAlpha;
}

ON_GAME_CRASH()
{
    // Print lastScript* data to the cleo logging!
    if(!cleo) return;
    cleo->PrintToCleoLog("[ The game crashed ]");

    if(scriptDebugger)
    {
        char buf[512], defName[8], custName[128];
        int callNum = 0;

        cleo->PrintToCleoLog("The data below is not guaranteed to be correct!");
        cleo->PrintToCleoLog("CLEO callstack list:");
        for(int i = SCRIPTS_LOG_COUNT-1; i >= 0; --i)
        {
            if(!lastScriptHandle[i] || !lastScriptPC[i]) continue;
        
            // Check if this script handle is still correct
            // If it is, we have a name, filename, a complete script code and more!
            if(!IsValidScriptHandle(lastScriptHandle[i]))
            {
                // It does not contain a valid data anymore: was deleted or something like that.
                snprintf(buf, sizeof(buf), "CALL #%d, Unknown Script 0x%08X, OpCode %04X", ++callNum, (uintptr_t)lastScriptHandle[i], lastScriptOp[i]);
                cleo->PrintToCleoLog(buf);
                continue;
            }

            uint8_t *backupPC = GetPC(lastScriptHandle[i]);
            GetPC(lastScriptHandle[i]) = lastScriptPC[i];

            bool isCustom = GetAddonInfo(lastScriptHandle[i]).isCustom;
            uint16_t lastScriptOpcode = Read2Bytes(lastScriptHandle[i]);
            strncpy(defName, ((GTAScript*)lastScriptHandle[i])->name, sizeof(defName)); defName[sizeof(defName)-1] = 0;
            custName[0] = 0;
            if(isCustom)
            {
                void* parentThread = GetAddonInfo(lastScriptHandle[i]).parentThread;
                if(parentThread)
                {
                    const char* filename = CLEO_GetScriptFilename(parentThread);
                    if(filename)
                    {
                        snprintf(custName, sizeof(custName), "thread of \"%s\"", filename);
                    }
                    else
                    {
                        strncpy(custName, "thread of \"unknown script\"", sizeof(custName));
                    }
                    custName[sizeof(custName)-1] = 0;
                }
                else
                {
                    const char* filename = CLEO_GetScriptFilename(lastScriptHandle[i]);
                    if(filename) strncpy(custName, filename, sizeof(custName)); custName[sizeof(custName)-1] = 0;
                }
            }
            
            snprintf(buf, sizeof(buf), "CALL #%d, %s Script '%s', OpCode %04X", ++callNum, isCustom ? "CLEO" : "Game", custName[0] != 0 ? custName : defName, lastScriptOpcode);
            cleo->PrintToCleoLog(buf);

            GetPC(lastScriptHandle[i]) = backupPC;
        }
        cleo->PrintToCleoLog("[ Crashlog Ending ]");
    }
    else
    {
        cleo->PrintToCleoLog("[ Script debugging was not enabled ]");
    }
}

//ON_NEW_INTERFACE()
//{
//    if(!strcmp(name, "SAUtils"))
//    {
//        sautils = (ISAUtils*)ptr;
//        SAUtilsStarted();
//    }
//}