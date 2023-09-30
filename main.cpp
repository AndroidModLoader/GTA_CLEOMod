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

// SAUtils
#include "isautils.h"
ISAUtils* sautils = nullptr;

// Size of array
#define sizeofA(__aVar)  ((int)(sizeof(__aVar)/sizeof(__aVar[0])))

MYMODCFG(net.rusjj.cleolib, CLEO Library, 2.0.1.5, Alexander Blade & RusJJ & XMDS)
BEGIN_DEPLIST()
    ADD_DEPENDENCY_VER(net.rusjj.aml, 1.0.2.2)
END_DEPLIST()

inline size_t __strlen(const char *str)
{
    const char* s = str;
    while(*s) ++s;
    return (s - str);
}

inline void __pathback(char* a)
{
    int len = __strlen(a);
    char* b = a + len;
    while(b != a)
    {
        --b;
        if(*b == '/')
        {
            a[b-a] = 0;
            break;
        }
    }
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
        char szLoadFrom[0xFF];
        snprintf(szLoadFrom, sizeof(szLoadFrom), "%s/libcleo.mod.so", aml->GetDataPath());

        std::ofstream fs(szLoadFrom, std::ios::out | std::ios::binary);
        fs.write((const char*)cleoData, sizeof(cleoData));
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
        char tmp[0xFF];
        snprintf(tmp, sizeof(tmp), "%s", aml->GetConfigPath());
        __pathback(tmp);
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
        char tmp[0xFF];
        snprintf(tmp, sizeof(tmp), "%s", aml->GetConfigPath());
        __pathback(tmp);
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
        char tmp[0xFF];
        snprintf(tmp, sizeof(tmp), "%s", aml->GetConfigPath());
        __pathback(tmp);
        __pathback(tmp);
        snprintf(tmp, sizeof(tmp), "%s/files/CLEO", tmp);
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
    CLEO_RegisterOpcode(0xBA00, AML_HAS_MOD_LOADED); // BA00=2,%2d% = aml_has_mod_loaded %1s% // IF and SET
    CLEO_RegisterOpcode(0xBA01, AML_HAS_MODVER_LOADED); // BA01=3,%3d% = aml_has_mod_loaded %1s% version %2s% // IF and SET

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
