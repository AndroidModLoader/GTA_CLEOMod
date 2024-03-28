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

// SAUtils
#include "isautils.h"
ISAUtils* sautils = nullptr;

// Size of array
#define sizeofA(__aVar)  ((int)(sizeof(__aVar)/sizeof(__aVar[0])))

MYMODCFG(net.rusjj.cleolib, CLEO Library, 2.0.1.6, Alexander Blade & RusJJ & XMDS)
BEGIN_DEPLIST()
    ADD_DEPENDENCY_VER(net.rusjj.aml, 1.2.1)
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

void* pCLEO;
uintptr_t nCLEOAddr;
Dl_info pDLInfo;
eGameIdent* nGameIdent;

ConfigEntry* pCfgCLEOLocation;
ConfigEntry* pCfgCLEORedArrow;
ConfigEntry* pCfgCLEOMenuColor;
ConfigEntry* pCfgCLEOMenuArrowColor;
ConfigEntry* pCfgCLEOMenuArrowPressedAlpha;

rgba_t* pCLEOMenuColor; // 1525C
rgba_t* pCLEOMenuArrowColor; // 15250
uint8_t* pCLEOArrowLastAlpha; // 2194FC

extern unsigned char cleoData[100160];
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

extern "C" __attribute__((target("thumb-mode"))) __attribute__((naked)) void Opcode0DD2_inject()
{
    //see https://github.com/XMDS/OP_0DD2FixAsm_call.git (cleo verison)

    __asm volatile(
    ".thumb\n"
        "PUSH {R4-R7,LR}\n"
        "MOV R4, R0\n" //R0 = pointers to the first 4 parameters of the function
        "MOV R5, R1\n" //R1 = function addr 
        "MOVS R6, #0x10\n" //It starts from R4
        "MOVS R7, #0\n"
        "SUB SP, #0xB8\n" //The maximum setting of the stack is 46 parameters

        "loc_1:\n"
        "CMP R7, #0xB8\n"
        "BEQ loc_2\n"
        "LDR R1, [R0, R6]\n" //Read parameters from reg in 0DD3 in cleo. It starts from R4
        "STR.W R1, [SP, R7]\n" //Write the extracted parameters to the stack
        "ADDS R6, #4\n" //Next parameter
        "ADDS R7, #4\n" //Stack +4 to save the next parameter
        "B loc_1\n"

        "loc_2:\n"
        "LDR R0, [R4]\n" //0DD3 context_set_reg 0
        "LDR R1, [R4,#4]\n" //0DD3 context_set_reg 1
        "LDR R2, [R4,#8]\n" //0DD3 context_set_reg 2
        "LDR R3, [R4,#0xC]\n" //0DD3 context_set_reg 3
        "BLX R5\n"           //0DD2 call func
        "STR R0, [R4]\n" //0DD4 return value 
        "ADD SP, #0xB8\n"
        "POP {R4-R7,PC}\n"
    );
}

void* g_pForceInterrupt = NULL;
DECL_HOOK(int8_t, ProcessOneCommand, void* handle)
{
    int8_t retCode = ProcessOneCommand(handle);
    if(g_pForceInterrupt && g_pForceInterrupt == handle)
    {
        g_pForceInterrupt = NULL;
        return 1;
    }
    return retCode;
}

extern "C" void OnModPreLoad()
{
    logger->SetTag("CLEO Mod");
    pCfgCLEOLocation = cfg->Bind("CLEO_Location", 1);
    pCfgCLEORedArrow = cfg->Bind("CLEO_RedArrow", true);
    pCfgCLEOMenuColor = cfg->Bind("CLEO_MenuColor", "55 127 175 150");
    pCfgCLEOMenuArrowColor = cfg->Bind("CLEO_MenuArrowColor", "55 127 175 100");
    pCfgCLEOMenuArrowPressedAlpha = cfg->Bind("CLEO_MenuArrowPressedAlpha", "180");
    
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
        aml->Unprot(nCLEOAddr + 0x146A9, 11);
        uintptr_t cleoDir = nCLEOAddr + 0x146A9;
        *(char*)(cleoDir + 3) = '\0';

        aml->Unprot(nCLEOAddr + 0x14C2C, 16);
        uintptr_t cleoLog = nCLEOAddr + 0x14C2C;
        *(char*)(cleoLog + 7) = '.';
        *(char*)(cleoLog + 8) = 'l';
        *(char*)(cleoLog + 9) = 'o';
        *(char*)(cleoLog + 10) = 'g';
        *(char*)(cleoLog + 11) = '\0';
    }
    else if(pCfgCLEOLocation->GetInt() == 2)
    {
        char tmp[256];
        snprintf(tmp, sizeof(tmp), "%s", aml->GetAndroidDataPath());
        __pathback(tmp);
        setenv("EXTERNAL_STORAGE", tmp, 1);
        snprintf(tmp, sizeof(tmp), "%s/cleo", tmp);
        mkdir(tmp, 0777);
        
        aml->Unprot(nCLEOAddr + 0x146A9, 11);
        uintptr_t cleoDir = nCLEOAddr + 0x146A9;
        *(char*)(cleoDir + 8) = '\0';
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
        aml->PlaceNOP(nCLEOAddr + 0xBD82, 2);
        
    // XMDS Part 1
    // Fixed OPCODE 0DD2
    aml->Redirect((nCLEOAddr + 0x4EB8 + 0x1), (uintptr_t)Opcode0DD2_inject);
        
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
    
    // Start CLEO
    libEntry();
    RegisterInterface("CLEO", cleo);
    logger->Info("CLEO Initialized!");

    cleo_addon_ifs.GetInterfaceVersion = []() -> uint32_t
    {
        return 1;
    };
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
    RegisterInterface("CLEOAddon", &cleo_addon_ifs);
    HOOK(ProcessOneCommand, cleo->GetMainLibrarySymbol("_ZN14CRunningScript17ProcessOneCommandEv"));
    logger->Info("CLEO Addon Initialized!");
}

const char* GetCLEODir()
{
    static char gotIt[256];
    bool bGotit = false;
    if(!bGotit)
    {
        char pad[24];
        char* (*CLEO_GetDir)(char*);
        SET_TO(CLEO_GetDir, nCLEOAddr + 0x607D);
        CLEO_GetDir(&pad[0]);
        strcpy(gotIt, *(char**)(pad + 20));
        bGotit = true;
    }
    return gotIt;
}

extern void (*UpdateCompareFlag)(void*, uint8_t);
void AML_HAS_MOD_LOADED(void *handle, uint32_t *ip, uint16_t opcode, const char *name)
{
    char modname[128];
    CLEO_ReadStringEx(handle, modname, sizeof(modname));

    bool hasMod = aml->HasMod(modname);
    cleo->GetPointerToScriptVar(handle)->i = hasMod;
    UpdateCompareFlag(handle, hasMod);
}
void AML_HAS_MODVER_LOADED(void *handle, uint32_t *ip, uint16_t opcode, const char *name)
{
    char modname[128], modver[24];
    CLEO_ReadStringEx(handle, modname, sizeof(modname));
    CLEO_ReadStringEx(handle, modver, sizeof(modver));

    bool hasMod = aml->HasModOfVersion(modname, modver);
    cleo->GetPointerToScriptVar(handle)->i = hasMod;
    UpdateCompareFlag(handle, hasMod);
}
void AML_REDIRECT_CODE(void *handle, uint32_t *ip, uint16_t opcode, const char *name)
{
    uintptr_t code1 = cleo->ReadParam(handle)->u;
    if(cleo->ReadParam(handle)->i != 0) code1 += (uintptr_t)cleo->GetMainLibraryLoadAddress();
    uintptr_t code2 = cleo->ReadParam(handle)->u;
    if(cleo->ReadParam(handle)->i != 0) code2 += (uintptr_t)cleo->GetMainLibraryLoadAddress();

    aml->Redirect(code1, code2);
}
void AML_JUMP_CODE(void *handle, uint32_t *ip, uint16_t opcode, const char *name)
{
    uintptr_t code1 = cleo->ReadParam(handle)->u;
    if(cleo->ReadParam(handle)->i != 0) code1 += (uintptr_t)cleo->GetMainLibraryLoadAddress();
    uintptr_t code2 = cleo->ReadParam(handle)->u;
    if(cleo->ReadParam(handle)->i != 0) code2 += (uintptr_t)cleo->GetMainLibraryLoadAddress();

    aml->PlaceB(code1, code2);
}
void AML_GET_BRANCH_DEST(void *handle, uint32_t *ip, uint16_t opcode, const char *name)
{
    uintptr_t code = cleo->ReadParam(handle)->u;
    if(cleo->ReadParam(handle)->i != 0) code += (uintptr_t)cleo->GetMainLibraryLoadAddress();
    
    cleo->GetPointerToScriptVar(handle)->i = aml->GetBranchDest(code);
}
void AML_MLS_SAVE(void *handle, uint32_t *ip, uint16_t opcode, const char *name)
{
    aml->MLSSaveFile();
}
void AML_MLS_HAS_VALUE(void *handle, uint32_t *ip, uint16_t opcode, const char *name)
{
    char key[16];
    CLEO_ReadStringEx(handle, key, sizeof(key));
    UpdateCompareFlag(handle, aml->MLSHasValue(key));
}
void AML_MLS_DELETE_VALUE(void *handle, uint32_t *ip, uint16_t opcode, const char *name)
{
    char key[16];
    CLEO_ReadStringEx(handle, key, sizeof(key));
    aml->MLSDeleteValue(key);
}
void AML_MLS_SET_INT(void *handle, uint32_t *ip, uint16_t opcode, const char *name)
{
    char key[16];
    CLEO_ReadStringEx(handle, key, sizeof(key));
    aml->MLSSetInt(key, cleo->ReadParam(handle)->i);
}
void AML_MLS_SET_FLOAT(void *handle, uint32_t *ip, uint16_t opcode, const char *name)
{
    char key[16];
    CLEO_ReadStringEx(handle, key, sizeof(key));
    aml->MLSSetFloat(key, cleo->ReadParam(handle)->f);
}
void AML_MLS_SET_STRING(void *handle, uint32_t *ip, uint16_t opcode, const char *name)
{
    char key[16];
    CLEO_ReadStringEx(handle, key, sizeof(key));
    char value[128];
    CLEO_ReadStringEx(handle, value, sizeof(value));
    aml->MLSSetStr(key, value);
}
void AML_MLS_GET_INT(void *handle, uint32_t *ip, uint16_t opcode, const char *name)
{
    char key[16];
    CLEO_ReadStringEx(handle, key, sizeof(key));
    int value = cleo->ReadParam(handle)->i;
    aml->MLSGetInt(key, &value);
    cleo->GetPointerToScriptVar(handle)->i = value;
}
void AML_MLS_GET_FLOAT(void *handle, uint32_t *ip, uint16_t opcode, const char *name)
{
    char key[16];
    CLEO_ReadStringEx(handle, key, sizeof(key));
    float value = cleo->ReadParam(handle)->f;
    aml->MLSGetFloat(key, &value);
    cleo->GetPointerToScriptVar(handle)->f = value;
}
void AML_MLS_GET_STRING(void *handle, uint32_t *ip, uint16_t opcode, const char *name)
{
    char key[16];
    CLEO_ReadStringEx(handle, key, sizeof(key));
    char value[16];
    CLEO_ReadStringEx(handle, value, sizeof(value));
    aml->MLSGetStr(key, value, sizeof(value));
    CLEO_WriteStringEx(handle, value);
}

#define CLEO_RegisterOpcode(x, h) cleo->RegisterOpcode(x, h); cleo->RegisterOpcodeFunction(#h, h)
void Init4Opcodes();
extern "C" void OnAllModsLoaded()
{
    if(!cleo) return;

    sautils = (ISAUtils*)GetInterface("SAUtils");
    if(sautils)
    {
        sautils->AddClickableItem(SetType_Game, "CLEO Location", pCfgCLEOLocation->GetInt(), 0, sizeofA(pLocations)-1, pLocations, OnLocationChanged, NULL);
        sautils->AddClickableItem(SetType_Game, "CLEO Red Arrow", pCfgCLEORedArrow->GetInt(), 0, sizeofA(pYesNo)-1, pYesNo, OnRedArrowChanged, NULL);
    }
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

    // Fix Alexander Blade's ass code (returns NULL!!! BRUH)
    cleo->GetCleoStorageDir = GetCLEODir;
    cleo->GetCleoPluginLoadDir = GetCLEODir;

    // CLEO4 Opcodes
    char savpath[256];
    sprintf(savpath, "%s/sav", cleo->GetCleoStorageDir());
    mkdir(savpath, 0777);
    Init4Opcodes();

    // DMA Fix
    if(*nGameIdent == GTASA)
    {
        uintptr_t pGTASA = aml->GetLib("libGTASA.so");
        aml->Write8(pGTASA + 0x32950A + 0x1, 0x68);
    }
}
