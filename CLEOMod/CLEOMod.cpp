#include "mod/amlmod.h"
#include "mod/logger.h"
#include "mod/config.h"

#include <stdlib.h>
#include <sys/stat.h>
#include <fstream>
#include <stdint.h>
#include <dlfcn.h>

// CLEO
#include "cleo.h"
cleo_ifs_t* cleo = NULL;

// SAUtils
#include "isautils.h"
ISAUtils* sautils = NULL;

// Size of array
#define sizeofA(__aVar)  ((int)(sizeof(__aVar)/sizeof(__aVar[0])))

MYMODCFG(net.rusjj.cleomod, CLEO Mod, 2.0.1.3, Alexander Blade & RusJJ & XMDS)
BEGIN_DEPLIST()
    ADD_DEPENDENCY_VER(net.rusjj.aml, 1.0.0.6)
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
Dl_info pDLInfo;

ConfigEntry* pCLEOLocation;
ConfigEntry* pCLEORedArrow;

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
void OnLocationChanged(int oldVal, int newVal)
{
    pCLEOLocation->SetInt(newVal);
    cfg->Save();
}
void OnRedArrowChanged(int oldVal, int newVal)
{
    pCLEORedArrow->SetBool(newVal != 0);
    cfg->Save();
}

extern "C" TARGET_THUMB ASM_NAKED void fix_command_0DD2_asm_call()
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
    pCLEOLocation = cfg->Bind("CLEO_Location", 0);
    pCLEORedArrow = cfg->Bind("CLEO_RedArrow", true);
    pCLEO = dlopen("libcleo.so", RTLD_NOLOAD);
    if (!pCLEO)
    {
        char szLoadFrom[0xFF];
        snprintf(szLoadFrom, sizeof(szLoadFrom), "%s/libcleo.mod.so", aml->GetDataPath());

        std::ofstream fs(szLoadFrom, std::ios::out | std::ios::binary);
        fs.write((const char*)cleoData, sizeof(cleoData));
        fs.close();
        pCLEO = dlopen(szLoadFrom, RTLD_NOW);
    }
    if (pCLEO == NULL)
    {
    OOPSIE:
        logger->Error("Failed to load CLEO library!");
        return;
    }
    auto libEntry = (void(*)())dlsym(pCLEO, "JNI_OnLoad");
    if(libEntry == NULL) goto OOPSIE;

    dladdr((void*)libEntry, &pDLInfo);
    int32_t cleover;
    aml->Unprot((uintptr_t)pDLInfo.dli_fbase + 0x19218, 4);
    memcpy(&cleover, (void*)((uintptr_t)pDLInfo.dli_fbase + 0x19218), sizeof(cleover));
    if (cleover != 0x00014A97) //check cleolib ver
        return logger->Error("Unknown cleo library version, CLEOMod only supports cleo library version 2.0.1!!!");

    cleo = (cleo_ifs_t*)((uintptr_t)pDLInfo.dli_fbase + 0x219AA8);
    aml->Redirect(((uintptr_t)pDLInfo.dli_fbase + 0x4EB8 + 1), (uintptr_t)&fix_command_0DD2_asm_call);//fix 0DD2

    if(pCLEOLocation->GetInt() == 1)
    {
        char tmp[0xFF];
        snprintf(tmp, sizeof(tmp), "%s", aml->GetConfigPath());
        __pathback(tmp);
        __pathback(tmp);
        setenv("EXTERNAL_STORAGE", tmp, 1);
        
      SET_LOAD_DIRECTLY:
        aml->Unprot((uintptr_t)pDLInfo.dli_fbase + 0x146A9, 11);
        uintptr_t cleoDir = (uintptr_t)pDLInfo.dli_fbase + 0x146A9;
        *(char*)(cleoDir + 3) = '\0';

        aml->Unprot((uintptr_t)pDLInfo.dli_fbase + 0x14C2C, 16);
        uintptr_t cleoLog = (uintptr_t)pDLInfo.dli_fbase + 0x14C2C;
        *(char*)(cleoLog + 7) = '.';
        *(char*)(cleoLog + 8) = 'l';
        *(char*)(cleoLog + 9) = 'o';
        *(char*)(cleoLog + 10) = 'g';
        *(char*)(cleoLog + 11) = '\0';
    }
    else if(pCLEOLocation->GetInt() == 2)
    {
        char tmp[0xFF];
        snprintf(tmp, sizeof(tmp), "%s", aml->GetConfigPath());
        __pathback(tmp);
        __pathback(tmp);
        setenv("EXTERNAL_STORAGE", tmp, 1);
        snprintf(tmp, sizeof(tmp), "%s/cleo", tmp);
        mkdir(tmp, 0777);
        
        aml->Unprot((uintptr_t)pDLInfo.dli_fbase + 0x146A9, 11);
        uintptr_t cleoDir = (uintptr_t)pDLInfo.dli_fbase + 0x146A9;
        *(char*)(cleoDir + 8) = '\0';
    }
    else if(pCLEOLocation->GetInt() == 3)
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

    if(!pCLEORedArrow->GetBool())
        aml->PlaceNOP((uintptr_t)pDLInfo.dli_fbase + 0xBD82, 2);
    libEntry();
    RegisterInterface("CLEO", cleo);
    logger->Info("CLEO initialized!");
}

extern "C" void OnModLoad()
{
    sautils = (ISAUtils*)GetInterface("SAUtils");
    if(sautils)
    {
        sautils->AddClickableItem(SetType_Game, "CLEO Location", pCLEOLocation->GetInt(), 0, sizeofA(pLocations)-1, pLocations, OnLocationChanged);
        sautils->AddClickableItem(SetType_Game, "CLEO Red Arrow", pCLEORedArrow->GetInt(), 0, sizeofA(pYesNo)-1, pYesNo, OnRedArrowChanged);
    }
}
