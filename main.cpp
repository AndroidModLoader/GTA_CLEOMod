#include <mod/amlmod.h>
#include <mod/logger.h>
#include <mod/config.h>

#include <stdlib.h>
#include <sys/stat.h>
#include <fstream>
#include <stdint.h>
#include <dlfcn.h>

// CLEO
#include "cleo.h"
cleo_ifs_t* cleo = nullptr;

// SAUtils
#include "isautils.h"
ISAUtils* sautils = nullptr;

// Size of array
#define sizeofA(__aVar)  ((int)(sizeof(__aVar)/sizeof(__aVar[0])))

MYMODCFG(net.rusjj.cleolib, CLEO Library, 2.0.1.3, Alexander Blade & RusJJ)
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

extern "C" void OnModPreLoad()
{
    logger->SetTag("CLEO Mod");
    pCLEOLocation = cfg->Bind("CLEO_Location", 0);
    pCLEORedArrow = cfg->Bind("CLEO_RedArrow", true);
    
    char szLoadFrom[0xFF];
    snprintf(szLoadFrom, sizeof(szLoadFrom), "%s/libcleo.mod.so", aml->GetDataPath());

    std::ofstream fs(szLoadFrom, std::ios::out | std::ios::binary);
	fs.write((const char*)cleoData, sizeof(cleoData));
	fs.close();
    pCLEO = dlopen(szLoadFrom, RTLD_NOW);
    if(pCLEO == nullptr)
    {
      OOPSIE:
        logger->Error("Failed to load CLEO library!");
        return;
    }
    auto libEntry = (void(*)())dlsym(pCLEO, "JNI_OnLoad");
    if(libEntry == nullptr) goto OOPSIE;

    dladdr((void*)libEntry, &pDLInfo);
    cleo = (cleo_ifs_t*)((uintptr_t)pDLInfo.dli_fbase + 0x219AA8);
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
        sautils->AddClickableItem(Game, "CLEO Location", pCLEOLocation->GetInt(), 0, sizeofA(pLocations)-1, pLocations, OnLocationChanged);
        sautils->AddClickableItem(Game, "CLEO Red Arrow", pCLEORedArrow->GetInt(), 0, sizeofA(pYesNo)-1, pYesNo, OnRedArrowChanged);
    }
}