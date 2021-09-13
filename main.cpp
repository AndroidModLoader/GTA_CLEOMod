#include <mod/amlmod.h>
#include <mod/logger.h>
#include <mod/config.h>

#include <stdlib.h>
#include <filesystem>
#include <fstream>
#include <stdint.h>
#include <dlfcn.h>

#include "icleo.h"
#include "cleo.h"
CLEO cleoLocal;
ICLEO* cleoInterface = &cleoLocal;

#include "isautils.h"
ISAUtils* sautils = nullptr;
cleo_ifs_t* cleo = nullptr;

#define sizeofA(__aVar)  ((int)(sizeof(__aVar)/sizeof(__aVar[0])))

MYMODCFG(net.rusjj.cleolib, CLEO Library, 2.0.1.1, Alexander Blade & RusJJ)
BEGIN_DEPLIST()
    ADD_DEPENDENCY_VER(net.rusjj.aml, 1.0.0.4)
END_DEPLIST()

std::string loadFrom;
void* pCLEO;
Dl_info pDLInfo;
uintptr_t pGTASA;

ConfigEntry* pCLEOLocation;
ConfigEntry* pCLEORedArrow;

extern unsigned char cleoData[100160];
const char* pLocations[] = 
{
    "CLEO 2.0.1",
    "Old CLEO",
    "Old CLEO (+cleo)",
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

    pGTASA = aml->GetLib("libGTASA.so");
    loadFrom = aml->GetDataPath();
    loadFrom += "/cleo_mod.so";

    std::ofstream fs(loadFrom.data(), std::ios::out | std::ios::binary);
	fs.write((const char*)cleoData, sizeof(cleoData));
	fs.close();
    pCLEO = dlopen(loadFrom.data(), RTLD_NOW);
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
        setenv("EXTERNAL_STORAGE", std::filesystem::path(aml->GetConfigPath()).parent_path().parent_path().c_str(), 1);

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
        auto VAR = std::filesystem::path(aml->GetConfigPath()).parent_path().parent_path();
        setenv("EXTERNAL_STORAGE", VAR.c_str(), 1);
        std::filesystem::create_directory(VAR.c_str() + std::string("/cleo"));
        
        aml->Unprot((uintptr_t)pDLInfo.dli_fbase + 0x146A9, 11);
        uintptr_t cleoDir = (uintptr_t)pDLInfo.dli_fbase + 0x146A9;
        *(char*)(cleoDir + 8) = '\0';
    }

    if(!pCLEORedArrow->GetBool())
        aml->PlaceNOP((uintptr_t)pDLInfo.dli_fbase + 0xBD82, 2);
    RegisterInterface("CLEO", cleoInterface);
    libEntry();
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