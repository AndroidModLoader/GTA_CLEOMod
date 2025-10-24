#include <mod/amlmod.h>
#include <mod/logger.h>
#include <cleohelpers.h>

#include <dirent.h>
#include <list>
#include <string>
#include <sys/stat.h>

// mini-scanf
#include <mini-scanf-cleo/c_scan.h>

// CLEO
#include "cleo.h"

extern uintptr_t nCLEOAddr, nGameAddr;

// Class Declarations
class GTAScript;
union GXTChar;
struct CLEO_STD_String;

// Own Vars
int CleoVariables[0x400];
std::set<void*> gAllocationsMap;
std::set<FILE*> gFilesMap;

// Game Vars
uint8_t* ScriptSpace;
GTAScript **pActiveScripts, **pIdleScripts;
uint8_t* LocalVariablesForCurrentMission;
int* ScriptParams;
uintptr_t gMobileMenu;
uintptr_t ms_modelInfoPtrs;
uintptr_t ms_RadarTrace;
uintptr_t* pedPool;
uintptr_t* vehiclePool;
uintptr_t* objectPool;
void* TheText;
void** curCheatCar_VC;
char* m_CheatString;
int *keys;

// Game Funcs
void (*UpdateCompareFlag)(void*, uint8_t);
int (*GetPedFromRef)(int);
int (*GetVehicleFromRef)(int);
int (*GetObjectFromRef)(int);
void (*CollectParameters_SA)(void*, int16_t);
void (*StoreParameters_SA)(void*, int16_t);
void (*CollectParameters_VC)(void*, void*, uint16_t);
void (*StoreParameters_VC)(void*, void*, uint16_t);
float (*FindGroundZForCoord)(float, float);
void (*SetHelpMessage_SA)(const char*, uint16_t*, bool, bool, bool, uint32_t);
void (*SetHelpMessage_VC)(uint16_t*, int, bool, bool);
uintptr_t (*FindPlayerPed)(int);
int (*GetPedRef)(int);
int (*GetVehicleRef)(int);
int (*GetObjectRef)(int);
void (*AddBigMessage)(uint16_t*, unsigned int, uint16_t);
void (*AddMessage_SA)(const char*, uint16_t*, unsigned int, uint16_t, bool);
void (*AddMessage_VC)(uint16_t*, unsigned int, uint16_t);
void (*AddMessageJumpQ_SA)(const char*, uint16_t*, unsigned int, uint16_t, bool);
void (*AddMessageJumpQ_VC)(uint16_t*, unsigned int, uint16_t);
void (*CLEO_STD_PutStrToAlloced)(CLEO_STD_String*, const char*);
void (*CLEO_STD_AddToGxtStorage)(CLEO_STD_String*, CLEO_STD_String*);
void (*CLEO_STD_DeallocStorage)(CLEO_STD_String*);
GXTChar* (*TextGet)(void*, const char*);
void* (*SpawnCar)(int);
bool (*IsHIDPressed)(int hidMapping, float* valOut);
void (*ClearAllCrosshairs)();
void (*SetWeaponLockOnTarget)(uintptr_t, void*);

// By MatiDragon
uintptr_t* TouchInterfaceWidgets;
//void (*TouchInterfaceTouchDown)(bool);
//void (*TouchInterfaceCachedPos)(float*, float*);

inline bool IsEndSlash(const char* str)
{
    char *s = (char*)str;
    while(*s != 0)
    {
        if(*s == '/' && *(s + 1) == 0) return true;
        ++s;
    }
    return false;
}
inline char *strrev(char *str)
{
    int i, n, len = 0;
    char temp;
    len = strlen(str);
    n = len-1;
    for(i = 0; i <= (len / 2); ++i)
    {
        temp = str[i];
        str[i] = str[n];
        str[n] = temp;
        --n;
    }
    return str;
}

// CLEO Structs
struct CLEO_STD_String // prob. just std::string..?
{
    char padding[24];
};
struct CLEO_DirScan
{
    DIR *dir;
    char path[256];
};

// ----------------------------------- OPCODES!

CLEO_Fn(INT_ADD)
{
    int a = cleo->ReadParam(handle)->i;
    int b = cleo->ReadParam(handle)->i;
    cleo->GetPointerToScriptVar(handle)->i = a + b;
}

CLEO_Fn(INT_SUB)
{
    if(GetVarArgCount(handle) > 1)
    {
        int a = cleo->ReadParam(handle)->i;
        int b = cleo->ReadParam(handle)->i;
        cleo->GetPointerToScriptVar(handle)->i = a - b;
    }
    else // default opcode (screw you WarDrum)
    {
        ClearAllCrosshairs();
        SetWeaponLockOnTarget(FindPlayerPed(-1), NULL);
    }
}

CLEO_Fn(INT_MUL)
{
    if(GetVarArgCount(handle) > 1)
    {
        int a = cleo->ReadParam(handle)->i;
        int b = cleo->ReadParam(handle)->i;
        cleo->GetPointerToScriptVar(handle)->i = a * b;
    }
    else // default opcode (screw you WarDrum)
    {
        int hidNum = cleo->ReadParam(handle)->i;
        UpdateCompareFlag(handle, IsHIDPressed(hidNum, NULL));
    }
}

void RemoveScript(void* handle);
CLEO_Fn(TERMINATE_THIS_CUSTOM_SCRIPT)
{
    char defName[8], custName[128], buf[256];
    bool isCustom = GetAddonInfo(handle).isCustom;

    custName[0] = 0;
    strncpy(defName, ((GTAScript*)handle)->name, sizeof(defName)); defName[sizeof(defName)-1] = 0;
    if(isCustom)
    {
        void* parentThread = GetAddonInfo(handle).parentThread;
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
            const char* filename = CLEO_GetScriptFilename(handle);
            if(filename) strncpy(custName, filename, sizeof(custName)); custName[sizeof(custName)-1] = 0;
        }
    }

    snprintf(buf, sizeof(buf), "[CLEOMod] terminating script '%s'", custName[0] != 0 ? custName : defName);
    cleo->PrintToCleoLog(buf);

    RemoveScript(handle);
}

CLEO_Fn(INT_DIV)
{
    int a = cleo->ReadParam(handle)->i;
    int b = cleo->ReadParam(handle)->i;
    cleo->GetPointerToScriptVar(handle)->i = a / b;
}

CLEO_Fn(SAVE_THIS_CUSTOM_SCRIPT)
{
    GetAddonInfo(handle).enableThreadSaving = true;
}

CLEO_Fn(GET_PED_POINTER)
{
    int ref = cleo->ReadParam(handle)->i;
    cleo->GetPointerToScriptVar(handle)->i = GetPedFromRef(ref);
}

CLEO_Fn(GET_VEHICLE_POINTER)
{
    int ref = cleo->ReadParam(handle)->i;
    cleo->GetPointerToScriptVar(handle)->i = GetVehicleFromRef(ref);
}

CLEO_Fn(GET_OBJECT_POINTER)
{
    int ref = cleo->ReadParam(handle)->i;
    cleo->GetPointerToScriptVar(handle)->i = GetObjectFromRef(ref);
}

CLEO_Fn(SET_CURRENT_DIRECTORY)
{
    const char* path;
    char buf[MAX_STR_LEN];
    if(IsParamString(handle))
    {
        path = CLEO_ReadStringEx(handle, buf, sizeof(buf));
    }
    else
    {
        int idx = cleo->ReadParam(handle)->i;
        switch(idx)
        {
            default: path = DIR_GAME;   break;
            case 1:  path = DIR_USER;   break;
            case 2:  path = DIR_SCRIPT; break;
        }
    }
    GetAddonInfo(handle).workDir = path;
}

CLEO_Fn(OPEN_FILE)
{
    char filename[MAX_STR_LEN], mode[10];
    CLEO_ReadStringEx(handle, filename, sizeof(filename));
    CLEO_ReadStringEx(handle, mode, sizeof(mode));

    int i = 0; while(filename[i] != 0) // A little hack (cheeseburger is like WHAAAA)
    {
        if(filename[i] == '\\') filename[i] = '/';
        ++i;
    }
    std::string str = ResolvePath(handle, filename);
    FILE* file = DoFile(str.c_str(), mode);
    cleo->GetPointerToScriptVar(handle)->i = (int)file;
    UpdateCompareFlag(handle, file != NULL);
}

CLEO_Fn(CLOSE_FILE)
{
    FILE* file = (FILE*)cleo->ReadParam(handle)->i;
    FreeFile(file);
}

CLEO_Fn(GET_FILE_SIZE)
{
    FILE* file = (FILE*)cleo->ReadParam(handle)->i;
    int filesize = 0;
    if(file)
    {
        auto savedPos = ftell(file);
        fseek(file, 0, SEEK_END);
        filesize = (int)ftell(file);
        fseek(file, savedPos, SEEK_SET);
    }
    cleo->GetPointerToScriptVar(handle)->i = filesize;
}

CLEO_Fn(READ_FROM_FILE)
{
    FILE* file = (FILE*)cleo->ReadParam(handle)->i;
    int size = cleo->ReadParam(handle)->i;
    void* dst = (void*)(&cleo->GetPointerToScriptVar(handle)->i);
    
    if(file)
    {
        fread(dst, size, 1, file);
    }
    else
    {
        *(int*)dst = 0;
    }
}

CLEO_Fn(WRITE_TO_FILE)
{
    FILE* file = (FILE*)cleo->ReadParam(handle)->i;
    int size = cleo->ReadParam(handle)->i;
    if(IsParamNum(handle))
    {
        int num = cleo->ReadParam(handle)->i;
        void* buf = (void*)(&num);
        fwrite(buf, size, 1, file);
        return;
    }
    else if(IsParamVar(handle))
    {
        void* buf = (void*)(&cleo->GetPointerToScriptVar(handle)->i);
        fwrite(buf, size, 1, file);
        return;
    }
    char buf[256];
    CLEO_ReadStringEx(handle, buf, sizeof(buf));
    fwrite(buf, size, 1, file);
}

CLEO_Fn(GET_THIS_SCRIPT_STRUCT)
{
    cleo->GetPointerToScriptVar(handle)->i = (int)handle;
}

CLEO_Fn(GOSUB_IF_FALSE)
{
    int offset = cleo->ReadParam(handle)->i;
    bool condition = GetCond(handle);
    if(!condition)
    {
        PushStack(handle);
        ThreadJump(handle, offset);
    }
}

CLEO_Fn(RETURN_IF_FALSE)
{
    if(!GetCond(handle))
    {
        PopStack(handle);
    }
}

CLEO_Fn(LOAD_DYNAMIC_LIBRARY)
{
    char buf[MAX_STR_LEN]; CLEO_ReadStringEx(handle, buf, sizeof(buf));
    std::string str = ResolvePath(handle, buf);
    void* libHandle = dlopen(str.c_str(), RTLD_NOW);

    cleo->GetPointerToScriptVar(handle)->i = (int)libHandle;
    UpdateCompareFlag(handle, libHandle != NULL);
}

CLEO_Fn(FREE_DYNAMIC_LIBRARY)
{
    dlclose((void*)cleo->ReadParam(handle)->i);
}

CLEO_Fn(GET_DYNAMIC_LIBRARY_PROCEDURE)
{
    char funcName[128]; CLEO_ReadStringEx(handle, funcName, sizeof(funcName));
    void* libHandle = (void*)cleo->ReadParam(handle)->i;
    uintptr_t symAddr = aml->GetSym(libHandle, funcName);

    cleo->GetPointerToScriptVar(handle)->i = (int)symAddr;
    UpdateCompareFlag(handle, symAddr != 0);
}

CLEO_Fn(IS_GAME_VERSION_ORIGINAL)
{
    // We are not GTA:SA PC 1.0 US/EU
    UpdateCompareFlag(handle, false);
}

CLEO_Fn(GET_SCRIPT_STRUCT_NAMED)
{
    char threadName[8];
    CLEO_ReadStringEx(handle, threadName, sizeof(threadName)); threadName[sizeof(threadName)-1] = 0;
    for (GTAScript* script = *pActiveScripts; script != NULL; script = script->next)
    {
        if (strcasecmp(threadName, script->name) == 0)
        {
            cleo->GetPointerToScriptVar(handle)->i = (int)script;
            UpdateCompareFlag(handle, true);
            return;
        }
    }
    cleo->GetPointerToScriptVar(handle)->i = 0;
    UpdateCompareFlag(handle, false);
}

CLEO_Fn(DOES_FILE_EXIST)
{
    char filepath[128];
    CLEO_ReadStringEx(handle, filepath, sizeof(filepath)); filepath[sizeof(filepath)-1] = 0;
    int i = 0; while(filepath[i] != 0) // A little hack (cheeseburger is like WHAAAA)
    {
        if(filepath[i] == '\\') filepath[i] = '/';
        ++i;
    }
    
    char path[256];
    snprintf(path, sizeof(path), "%s/%s", aml->GetAndroidDataPath(), filepath);

    FILE *file = fopen(path, "r");
    UpdateCompareFlag(handle, file != NULL);
    if(file) fclose(file);
}

CLEO_Fn(IS_KEY_PRESSED)
{
    int key = cleo->ReadParam(handle)->i;
    UpdateCompareFlag(handle, keys[key] == 2);
}

#include "cleo4scmfunc.h"
void ScmCleanup()
{
    ScmFunction::CleanAll();
}
CLEO_Fn(CLEO_CALL)
{
    int label = cleo->ReadParam(handle)->i;
    int nParams = (Read1Byte_NoSkip(handle) != 0) ? cleo->ReadParam(handle)->i : 0;
    ScmFunction* scmFunc = new ScmFunction(handle);

    char buf[MAX_STR_LEN];
    static int arguments[40];
    int maxParams = ValueForSA(40, 16);
    int* scope = GetLocalVars(handle);
    if(*nGameIdent == GTASA && IsMissionScript(handle)) scope = (int*)LocalVariablesForCurrentMission;
    int* scopeEnd = scope + maxParams;
    int* storedLocals = scmFunc->savedTls;
    scmFunc->callArgCount = nParams;

    // collect arguments
    uint8_t max_i = nParams < maxParams ? nParams : maxParams;
    for (uint8_t i = 0; i < max_i; ++i)
    {
        int* val = &arguments[i];
        switch(Read1Byte_NoSkip(handle))
        {
            case DT_FLOAT:
            case DT_DWORD:
            case DT_WORD:
            case DT_BYTE:
            case DT_VAR:
            case DT_LVAR:
            case DT_VAR_ARRAY:
            case DT_LVAR_ARRAY:
                *val = cleo->ReadParam(handle)->i;
                break;

            case DT_VAR_STRING:
            case DT_LVAR_STRING:
            case DT_VAR_TEXTLABEL:
            case DT_LVAR_TEXTLABEL:
                *val = (int)cleo->GetPointerToScriptVar(handle);
                if (val >= scope && val < scopeEnd) // correct scoped variable's pointer
                {
                    *val -= (int)scope;
                    *val += (int)storedLocals;
                }
                break;

            case DT_STRING:
            case DT_TEXTLABEL:
            case DT_VARLEN_STRING:
                scmFunc->stringParams.emplace_back(CLEO_ReadStringEx(handle, buf, sizeof(buf))); // those texts exists in script code, but without terminator character. Copy is necessary
                *val = (int)(scmFunc->stringParams.back().c_str());
                break;
        }
    }

    // EXPERIMENTAL
    int i = -1;
    while(Read1Byte_NoSkip(handle) != 0)
    {
        scmFunc->savedRets[++i] = &cleo->GetPointerToScriptVar(handle)->i;
    }

    // EXPERIMENTAL - COMMENTED
    if (nParams > maxParams)
    {
        (*nGameIdent == GTASA) ? CollectParameters_SA(handle, nParams - maxParams) : CollectParameters_VC(handle, &GetPC(handle), nParams - maxParams);
    }
    //SkipUnusedParameters(handle); // TODO: RECHECK!!!!!!!
    scmFunc->retnAddress = GetPC(handle);
    memcpy(scope, arguments, 4 * nParams);
    
    int* localVars = GetLocalVars(handle);
    for(int i = nParams; i < maxParams; ++i) localVars[i] = 0;

    ThreadJump(handle, label);
}

inline void CleoReturnGeneric(void* handle, bool returnArgs, int returnArgCount)
{
    ScmFunction *scmFunc = ScmFunction::Store[GetScmFunc(handle)];
    if(*nGameIdent == GTASA)
    {
        if(returnArgs && returnArgCount) CollectParameters_SA(handle, returnArgCount);
        scmFunc->Return(handle);
        //if(returnArgCount) StoreParameters_SA(handle, returnArgCount);
        // EXPERIMENTAL
        for(uint8_t i = 0; returnArgs && i < returnArgCount; ++i)
        {
            *(scmFunc->savedRets[i]) = ScriptParams[i];
        }
    }
    else
    {
        if(returnArgs && returnArgCount) CollectParameters_VC(handle, &GetPC(handle), returnArgCount);
        scmFunc->Return(handle);
        if(returnArgs && returnArgCount) StoreParameters_VC(handle, &GetPC(handle), returnArgCount);
    }
    SkipUnusedParameters(handle);
    delete scmFunc;
}

CLEO_Fn(CLEO_RETURN)
{
    int nRetParams = 0;
    if(Read1Byte_NoSkip(handle))
    {
        nRetParams = cleo->ReadParam(handle)->i;
    }
    CleoReturnGeneric(handle, true, nRetParams);
}

CLEO_Fn(SET_CLEO_SHARED_VAR)
{
    int varId = cleo->ReadParam(handle)->i;
    int value = cleo->ReadParam(handle)->i;
    if(varId >= 0 && varId < 1024) CleoVariables[varId] = value;
}

CLEO_Fn(GET_CLEO_SHARED_VAR)
{
    int varId = cleo->ReadParam(handle)->i;
    cleo->GetPointerToScriptVar(handle)->i = (varId >= 0 && varId < 1024) ? CleoVariables[varId] : 0;
}

CLEO_Fn(STORE_CLOSEST_ENTITIES)
{
    int pPlayerPed = GetPedFromRef(cleo->ReadParam(handle)->i); // But it's not 100% player ped?
    uintptr_t pedintel;
    if(pPlayerPed && (pedintel = *(uintptr_t*)(pPlayerPed + 0x440)))
    {
        #define NUM_SCAN_ENTITIES 16

        int vehicle = 0;
        for(int i = 0; i < NUM_SCAN_ENTITIES; ++i)
        {
            vehicle = *(int*)(pedintel + 12 + 4 * i + 212);
            if(vehicle && *(char*)(vehicle + 1192) != 2 && (((*(uint8_t*)(vehicle + 1070) & 4) >> 2) ^ 1)) break;
            vehicle = 0;
        }

        int ped = 0;
        for(int i = 0; i < NUM_SCAN_ENTITIES; ++i)
        {
            ped = *(int*)(pedintel + 12 + 4 * i + 292);
            if(ped && ped != pPlayerPed && (((*(int*)(ped + 1160) & 8u) >> 3) ^ 1)) break;
            ped = 0;
        }

        cleo->GetPointerToScriptVar(handle)->i = vehicle ? GetVehicleRef(vehicle) : -1;
        cleo->GetPointerToScriptVar(handle)->i = ped     ? GetPedRef(ped)         : -1;
    }
    else
    {
        cleo->GetPointerToScriptVar(handle)->i = -1;
        cleo->GetPointerToScriptVar(handle)->i = -1;
    }
}

CLEO_Fn(GET_TARGET_BLIP_COORDS)
{
    int blipHndl = *(int*)(gMobileMenu + 72);
    if(blipHndl)
    {
        float x = *(float*)(ms_RadarTrace + *(uint16_t*)&blipHndl * 0x28 + 0x8);
        float y = *(float*)(ms_RadarTrace + *(uint16_t*)&blipHndl * 0x28 + 0xC);
        cleo->GetPointerToScriptVar(handle)->f = x;
        cleo->GetPointerToScriptVar(handle)->f = y;
        cleo->GetPointerToScriptVar(handle)->f = FindGroundZForCoord(x, y);
        UpdateCompareFlag(handle, true);
    }
    else
    {
        cleo->GetPointerToScriptVar(handle)->f = 0.0f;
        cleo->GetPointerToScriptVar(handle)->f = 0.0f;
        cleo->GetPointerToScriptVar(handle)->f = 0.0f;
        UpdateCompareFlag(handle, false);
    }
}

CLEO_Fn(GET_CAR_NUMBER_OF_GEARS)
{
    int ref = cleo->ReadParam(handle)->i;
    int vehiclePtr = GetVehicleFromRef(ref);
    if(*nGameIdent == GTASA)
    {
        cleo->GetPointerToScriptVar(handle)->i = *(uint8_t*)(*(int*)(vehiclePtr + 904) + 118);
    }
    else
    {
        cleo->GetPointerToScriptVar(handle)->i = *(uint8_t*)(*(int*)(vehiclePtr + 292) + 126);
    }
}

CLEO_Fn(GET_CAR_CURRENT_GEAR)
{
    int ref = cleo->ReadParam(handle)->i;
    int vehiclePtr = GetVehicleFromRef(ref);
    cleo->GetPointerToScriptVar(handle)->i = *(uint8_t*)(vehiclePtr + ValueForGame(0, 524, 1216));
}

CLEO_Fn(TERMINATE_ALL_CUSTOM_SCRIPTS_WITH_THIS_NAME)
{
    char buf[128];
    CLEO_ReadStringEx(handle, buf, sizeof(buf));

    void* foundHandle = NULL;
    while(true)
    {
        int size = GetScriptsStorageSize();
        for(int i = 0; i < size; ++i)
        {
            int storageItem = *(int*)(*pScriptsStorage + i * 4);
            foundHandle = *(void**)(storageItem + 28);
            if(foundHandle && GetActiveFlag(foundHandle))
            {
                RemoveScript(foundHandle);
                continue;
            }
        }
        break;
    }
}

CLEO_Fn(IS_CAR_SIREN_ON)
{
    int ref = cleo->ReadParam(handle)->i;
    int vehiclePtr = GetVehicleFromRef(ref);
    if(*nGameIdent == GTASA) UpdateCompareFlag(handle, *(uint8_t*)(vehiclePtr + 1073) >> 7);
    else UpdateCompareFlag(handle, *(bool*)(vehiclePtr + 585));
}

CLEO_Fn(IS_CAR_ENGINE_ON)
{
    int ref = cleo->ReadParam(handle)->i;
    int vehiclePtr = GetVehicleFromRef(ref);
    if(*nGameIdent == GTASA) UpdateCompareFlag(handle, (*(uint8_t *)(vehiclePtr + 1068) >> 4) & 1);
    else UpdateCompareFlag(handle, *(uint8_t *)(vehiclePtr + 509) & 0x10);
}

CLEO_Fn(CLEO_SET_CAR_ENGINE_ON)
{
    int vehiclePtr = GetVehicleFromRef(cleo->ReadParam(handle)->i);
    bool state = cleo->ReadParam(handle)->i != 0;
    if(*nGameIdent == GTASA) 
    {
        *(int*)(vehiclePtr + 1068) = *(int*)(vehiclePtr + 1068) & 0xFFFFFFEF | (16 * (state & 1));
    }
    else
    {
        if(state) *(uint8_t *)(vehiclePtr + 509) |= 0x10;
        else      *(uint8_t *)(vehiclePtr + 509) &= ~0x10;
    }
}

// cleo201_refactor.cpp, GET_LABEL_ADDR
extern int lastStorageItem;
CLEO_Fn(GET_LABEL_POINTER)
{
    int labelOffset = cleo->ReadParam(handle)->i;
    uint32_t* pLabelAddr = &cleo->GetPointerToScriptVar(handle)->u;

    int storageItem = lastStorageItem;//GetCustomHandleFromScriptHandle(handle);
    if(storageItem && *(void**)(storageItem + 28) == handle)
    {
        if(labelOffset < 0) labelOffset = -labelOffset;
        *pLabelAddr = *(uint32_t*)(storageItem + 32) + labelOffset;
    }
    else
    {
        // sadge
        int baseOffset = ValueForGame(0, 0, 16, 20, 20);
        if(baseOffset)
        {
            uint8_t* basePtr = GetBasePC(handle);
            *pLabelAddr = (uint32_t)((labelOffset < 0) ? (basePtr - labelOffset) : (ScriptSpace + labelOffset));
        }
        else
        {
            *pLabelAddr = (uint32_t)((labelOffset < 0) ? (ValueForGame(0x20000, 0x3F9A0, 0) - labelOffset) : labelOffset);
        }
    }
}

CLEO_Fn(GET_VAR_POINTER)
{
    int varPtr = (int)cleo->GetPointerToScriptVar(handle);
    cleo->GetPointerToScriptVar(handle)->i = varPtr;
}

CLEO_Fn(ALLOCATE_MEMORY)
{
    int size = cleo->ReadParam(handle)->i;
    void* mem = AllocMem(size);
    cleo->GetPointerToScriptVar(handle)->i = (int)mem;
    UpdateCompareFlag(handle, mem != NULL);
}

CLEO_Fn(FREE_MEMORY)
{
    void* mem = (void*)cleo->ReadParam(handle)->i;
    FreeMem(mem);
}

CLEO_Fn(PRINT_HELP_STRING)
{
    static uint16_t helpGxt[MAX_STR_LEN];
    char buf[MAX_STR_LEN];
    CLEO_ReadStringEx(handle, buf, sizeof(buf));
    AsciiToGXTChar(buf, helpGxt);

    if(*nGameIdent == GTASA)
    {
        SetHelpMessage_SA(buf, helpGxt, true, false, false, 0);
    }
    else if(*nGameIdent == GTAVC)
    {
        SetHelpMessage_VC(helpGxt, 0, true, false);
    }
}

CLEO_Fn(PRINT_BIG_STRING)
{
    static uint16_t helpGxt[MAX_STR_LEN];
    char buf[MAX_STR_LEN];
    CLEO_ReadStringEx(handle, buf, sizeof(buf));
    AsciiToGXTChar(buf, helpGxt);
    int time = cleo->ReadParam(handle)->i;
    int style = cleo->ReadParam(handle)->i;
    AddBigMessage(helpGxt, time, style - 1);
}

CLEO_Fn(PRINT_STRING)
{
    static uint16_t helpGxt[MAX_STR_LEN];
    char buf[MAX_STR_LEN];
    CLEO_ReadStringEx(handle, buf, sizeof(buf));
    AsciiToGXTChar(buf, helpGxt);
    int time = cleo->ReadParam(handle)->i;

    if(*nGameIdent == GTASA)
    {
        AddMessage_SA(buf, helpGxt, time, 0, false);
    }
    else if(*nGameIdent == GTAVC)
    {
        AddMessage_VC(helpGxt, time, false);
    }
}

CLEO_Fn(PRINT_STRING_NOW)
{
    static uint16_t helpGxt[MAX_STR_LEN];
    char buf[MAX_STR_LEN];
    CLEO_ReadStringEx(handle, buf, sizeof(buf));
    AsciiToGXTChar(buf, helpGxt);
    int time = cleo->ReadParam(handle)->i;

    if(*nGameIdent == GTASA)
    {
        AddMessageJumpQ_SA(buf, helpGxt, time, 0, false);
    }
    else if(*nGameIdent == GTAVC)
    {
        AddMessageJumpQ_VC(helpGxt, time, false);
    }
}

CLEO_Fn(PRINT_HELP_FORMATTED)
{
    static uint16_t helpGxt[MAX_STR_LEN];
    char text[MAX_STR_LEN], fmt[MAX_STR_LEN];
    CLEO_ReadStringEx(handle, fmt, sizeof(fmt));
    CLEO_FormatString(handle, text, sizeof(text), fmt);
    AsciiToGXTChar(text, helpGxt);

    if(*nGameIdent == GTASA)
    {
        SetHelpMessage_SA(text, helpGxt, true, false, false, 0);
    }
    else if(*nGameIdent == GTAVC)
    {
        SetHelpMessage_VC(helpGxt, 0, true, false);
    }
    SkipUnusedParameters(handle);
}

CLEO_Fn(PRINT_BIG_FORMATTED)
{
    static uint16_t helpGxt[MAX_STR_LEN];
    char text[MAX_STR_LEN], fmt[MAX_STR_LEN];
    CLEO_ReadStringEx(handle, fmt, sizeof(fmt));
    int time = cleo->ReadParam(handle)->i;
    int style = cleo->ReadParam(handle)->i;
    CLEO_FormatString(handle, text, sizeof(text), fmt);
    AsciiToGXTChar(text, helpGxt);
    AddBigMessage(helpGxt, time, style - 1);
    SkipUnusedParameters(handle);
}

CLEO_Fn(PRINT_FORMATTED)
{
    static uint16_t helpGxt[MAX_STR_LEN];
    char text[MAX_STR_LEN], fmt[MAX_STR_LEN];
    CLEO_ReadStringEx(handle, fmt, sizeof(fmt));
    int time = cleo->ReadParam(handle)->i;
    CLEO_FormatString(handle, text, sizeof(text), fmt);
    AsciiToGXTChar(text, helpGxt);

    if(*nGameIdent == GTASA)
    {
        AddMessage_SA(text, helpGxt, time, 0, false);
    }
    else if(*nGameIdent == GTAVC)
    {
        AddMessage_VC(helpGxt, time, false);
    }
    SkipUnusedParameters(handle);
}

CLEO_Fn(PRINT_FORMATTED_NOW)
{
    static uint16_t helpGxt[MAX_STR_LEN];
    char text[MAX_STR_LEN], fmt[MAX_STR_LEN];
    CLEO_ReadStringEx(handle, fmt, sizeof(fmt));
    int time = cleo->ReadParam(handle)->i;
    CLEO_FormatString(handle, text, sizeof(text), fmt);
    AsciiToGXTChar(text, helpGxt);

    if(*nGameIdent == GTASA)
    {
        AddMessageJumpQ_SA(text, helpGxt, time, 0, false);
    }
    else if(*nGameIdent == GTAVC)
    {
        AddMessageJumpQ_VC(helpGxt, time, false);
    }
    SkipUnusedParameters(handle);
}

CLEO_Fn(GET_CHAR_PLAYER_IS_TARGETING)
{
    uintptr_t playerPed = FindPlayerPed(cleo->ReadParam(handle)->i);
    if(!playerPed)
    {
      not_ok:
        cleo->GetPointerToScriptVar(handle)->i = 0;
        UpdateCompareFlag(handle, false);
        return;
    }

    int target;
    if(*nGameIdent == GTASA)
    {
        target = *(int*)(playerPed + 1824);
        if(target == 0 || (*(uint8_t*)(target + 58) & 7) != 3) goto not_ok;
    }
    else if(*nGameIdent == GTAVC)
    {
        target = *(int*)(playerPed + 322);
        if(target == 0 || (*(uint8_t*)(target + 84) & 7) != 3) goto not_ok;
    }

    cleo->GetPointerToScriptVar(handle)->i = GetPedRef(target);
    UpdateCompareFlag(handle, true);
}

CLEO_Fn(STRING_FORMAT)
{
    char fmt[MAX_STR_LEN], *dst = CLEO_GetStringPtr(handle);
    CLEO_ReadStringEx(handle, fmt, sizeof(fmt));
    CLEO_FormatString(handle, dst, -1, fmt);
    SkipUnusedParameters(handle);
}

CLEO_Fn(SCAN_STRING)
{
    char fmt[MAX_STR_LEN], *format, src[MAX_STR_LEN];
    CLEO_ReadStringEx(handle, src, sizeof(src));
    format = CLEO_ReadStringEx(handle, fmt, sizeof(fmt));

    size_t cExParams = 0;
    int* ExParams[35];
    int *result = (int*)cleo->GetPointerToScriptVar(handle);

    for (int i = 0; i < 35; ++i)
    {
        if(*GetPC(handle))
        {
            ExParams[i] = (int*)cleo->GetPointerToScriptVar(handle);
            ++cExParams;
        }
        else
        {
            ExParams[i] = NULL;
        }
    }
    ++GetPC(handle);

    *result = c_sscanf(src, format,
        /* extra parameters (will be aligned automatically, but the limit of 35 elements maximum exists) */
        ExParams[0], ExParams[1], ExParams[2], ExParams[3], ExParams[4], ExParams[5],
        ExParams[6], ExParams[7], ExParams[8], ExParams[9], ExParams[10], ExParams[11],
        ExParams[12], ExParams[13], ExParams[14], ExParams[15], ExParams[16], ExParams[17],
        ExParams[18], ExParams[19], ExParams[20], ExParams[21], ExParams[22], ExParams[23],
        ExParams[24], ExParams[25], ExParams[26], ExParams[27], ExParams[28], ExParams[29],
        ExParams[30], ExParams[31], ExParams[32], ExParams[33], ExParams[34]
    );
    UpdateCompareFlag(handle, cExParams == *result);
}

CLEO_Fn(FILE_SEEK)
{
    FILE *file = (FILE*)cleo->ReadParam(handle)->i;
    int seek = cleo->ReadParam(handle)->i;
    int origin = cleo->ReadParam(handle)->i;

    if(!file)
    {
        UpdateCompareFlag(handle, false);
        return;
    }
    UpdateCompareFlag(handle, fseek(file, seek, origin) == 0);
}

CLEO_Fn(IS_END_OF_FILE_REACHED)
{
    FILE *file = (FILE*)cleo->ReadParam(handle)->i;
    if(!file)
    {
        UpdateCompareFlag(handle, true);
        return;
    }
    UpdateCompareFlag(handle, ferror(file) || feof(file) != 0);
}

CLEO_Fn(READ_STRING_FROM_FILE)
{
    FILE *file = (FILE*)cleo->ReadParam(handle)->i;
    int maxsize = CLEO_GetStringPtrMaxSize(handle);
    char *buf = CLEO_GetStringPtr(handle);
    int scriptmaxsize = cleo->ReadParam(handle)->i;

    if(!file)
    {
        UpdateCompareFlag(handle, false);
        return;
    }
    int size = scriptmaxsize;
    if(size > maxsize) size = maxsize;
    UpdateCompareFlag(handle, fgets(buf, size, file) == buf);
}

CLEO_Fn(WRITE_STRING_TO_FILE)
{
    FILE *file = (FILE*)cleo->ReadParam(handle)->i;
    char buf[MAX_STR_LEN];
    CLEO_ReadStringEx(handle, buf, sizeof(buf));

    if(!file)
    {
        UpdateCompareFlag(handle, false);
        return;
    }
    UpdateCompareFlag(handle, fputs(buf, file) > 0);
    fflush(file);
}

CLEO_Fn(WRITE_FORMATTED_STRING_TO_FILE)
{
    FILE *file = (FILE*)cleo->ReadParam(handle)->i;
    char fmt[MAX_STR_LEN], text[MAX_STR_LEN];
    CLEO_ReadStringEx(handle, fmt, sizeof(fmt));
    CLEO_FormatString(handle, text, sizeof(text), fmt);

    if(!file) return;
    fputs(text, file);
    fflush(file);
}

CLEO_Fn(SCAN_FILE)
{
    FILE *file = (FILE*)cleo->ReadParam(handle)->i;
    char fmt[MAX_STR_LEN];
    CLEO_ReadStringEx(handle, fmt, sizeof(fmt));
    
    size_t cExParams = 0;
    int* ExParams[35];
    int *result = (int*)cleo->GetPointerToScriptVar(handle);

    for (int i = 0; i < 35; ++i)
    {
        if(*GetPC(handle))
        {
            ExParams[i] = (int*)cleo->GetPointerToScriptVar(handle);
            ++cExParams;
        }
        else
        {
            ExParams[i] = NULL;
        }
    }
    ++GetPC(handle);

    *result = fscanf(file, fmt,
        /* extra parameters (will be aligned automatically, but the limit of 35 elements maximum exists) */
        ExParams[0], ExParams[1], ExParams[2], ExParams[3], ExParams[4], ExParams[5],
        ExParams[6], ExParams[7], ExParams[8], ExParams[9], ExParams[10], ExParams[11],
        ExParams[12], ExParams[13], ExParams[14], ExParams[15], ExParams[16], ExParams[17],
        ExParams[18], ExParams[19], ExParams[20], ExParams[21], ExParams[22], ExParams[23],
        ExParams[24], ExParams[25], ExParams[26], ExParams[27], ExParams[28], ExParams[29],
        ExParams[30], ExParams[31], ExParams[32], ExParams[33], ExParams[34]
    );
    UpdateCompareFlag(handle, cExParams == *result);
}

CLEO_Fn(GET_NAME_OF_VEHICLE_MODEL)
{
    int model = cleo->ReadParam(handle)->i;
    CLEO_WriteStringEx(handle, (char*)(*(uintptr_t*)(ms_modelInfoPtrs + model * 4) + ValueForSA(74, 86)));
}

CLEO_Fn(TEST_CHEAT)
{
    char buf[CHEAT_STRING_SIZE];
    CLEO_ReadStringEx(handle, buf, sizeof(buf));
    char *s = strrev(buf);
    char *c = m_CheatString;
    UpdateCompareFlag(handle, strncmp(s, c, strlen(s)) == 0);
}

CLEO_Fn(SPAWN_VEHICLE_BY_CHEATING)
{
    int model = cleo->ReadParam(handle)->i;
    if(*nGameIdent == GTASA)
    {
        int mi = *(int*)(ms_modelInfoPtrs + model * 4);
        UpdateCompareFlag(handle, mi && *(int*)(mi + 84) != -1 && *(int*)(mi + 84) != 5 && SpawnCar(model) != NULL);
    }
    else
    {
        SpawnCar(model);
        UpdateCompareFlag(handle, *curCheatCar_VC != NULL);
    }
}

CLEO_Fn(GET_TEXT_LABEL_STRING)
{
    char gxtLabel[8];
    CLEO_ReadStringEx(handle, gxtLabel, sizeof(gxtLabel));
    GXTChar* keyvalue = TextGet(TheText, gxtLabel);
    CLEO_WriteStringEx(handle, GXTCharToAscii(keyvalue, 0));
}

void AddGXTLabel(const char* gxtLabel, const char* text)
{
    CLEO_STD_String key, keytext;
    CLEO_STD_PutStrToAlloced(&key, gxtLabel);
    CLEO_STD_PutStrToAlloced(&keytext, text);

    CLEO_STD_AddToGxtStorage(&key, &keytext);

    CLEO_STD_DeallocStorage(&key);
    CLEO_STD_DeallocStorage(&keytext);
}
CLEO_Fn(ADD_TEXT_LABEL)
{
    char gxtLabel[8], text[MAX_STR_LEN];
    CLEO_ReadStringEx(handle, gxtLabel, sizeof(gxtLabel));
    CLEO_ReadStringEx(handle, text, sizeof(text));

    if(IsCLEORelatedGXTKey(gxtLabel)) return; // NUH-UH

    AddGXTLabel(gxtLabel, text);
}

CLEO_Fn(REMOVE_TEXT_LABEL)
{
    char gxtLabel[8];
    CLEO_ReadStringEx(handle, gxtLabel, sizeof(gxtLabel));

    if(IsCLEORelatedGXTKey(gxtLabel)) return; // NUH-UH

    CLEO_STD_String key;
    CLEO_STD_PutStrToAlloced(&key, gxtLabel);

    CLEO_STD_AddToGxtStorage(&key, &key);

    CLEO_STD_DeallocStorage(&key);
}

CLEO_Fn(GET_RANDOM_CHAR_IN_SPHERE_NO_SAVE_RECURSIVE)
{
    GTAVector3D center;
    center.x = cleo->ReadParam(handle)->f;
    center.y = cleo->ReadParam(handle)->f;
    center.z = cleo->ReadParam(handle)->f;
    float radius = cleo->ReadParam(handle)->f, sqrradius = radius*radius;
    int next = cleo->ReadParam(handle)->i, passDeads = cleo->ReadParam(handle)->i;

    static int lastFound = 0;
    if(!next) lastFound = 0;

    auto objects = *(GTAPedSA**)(*pedPool + 0);
    tByteFlag* flags = *(tByteFlag**)(*pedPool + 4);
    int size = *(int*)(*pedPool + 8);

    if(radius >= 1000.0f)
    {
        for(int i = lastFound; i < size; ++i)
        {
            if(flags[i].bEmpty) continue;
            auto& ent = objects[i];
            if(passDeads != -1 && (ent.Player() || (passDeads && !((ent.IntAt(1100) & 0xFFFFFFFE) != 54)) || (ent.IntAt(1160) >> 3) & 1)) continue;
            
            lastFound = i + 1;
            cleo->GetPointerToScriptVar(handle)->i = GetPedRef(ent.AsInt());
            UpdateCompareFlag(handle, true);
            return;
        }
    }
    else
    {
        for(int i = lastFound; i < size; ++i)
        {
            if(flags[i].bEmpty) continue;
            auto& ent = objects[i];
            if(passDeads != -1 && (ent.Player() || (passDeads && !((ent.IntAt(1100) & 0xFFFFFFFE) != 54)) || (ent.IntAt(1160) >> 3) & 1)) continue;
            if((ent.GetPos() - center).SqrMagnitude() <= sqrradius)
            {
                lastFound = i + 1;
                cleo->GetPointerToScriptVar(handle)->i = GetPedRef(ent.AsInt());
                UpdateCompareFlag(handle, true);
                return;
            }
        }
    }
    cleo->GetPointerToScriptVar(handle)->i = -1;
    UpdateCompareFlag(handle, false);
    lastFound = 0;
}

CLEO_Fn(GET_RANDOM_CAR_IN_SPHERE_NO_SAVE_RECURSIVE)
{
    GTAVector3D center;
    center.x = cleo->ReadParam(handle)->f;
    center.y = cleo->ReadParam(handle)->f;
    center.z = cleo->ReadParam(handle)->f;
    float radius = cleo->ReadParam(handle)->f, sqrradius = radius*radius;
    int next = cleo->ReadParam(handle)->i, passWrecked = cleo->ReadParam(handle)->i;

    static int lastFound = 0;
    if(!next) lastFound = 0;

    auto objects = *(GTAVehicleSA**)(*vehiclePool + 0);
    tByteFlag* flags = *(tByteFlag**)(*vehiclePool + 4);
    int size = *(int*)(*vehiclePool + 8);

    if(radius >= 1000.0f)
    {
        for(int i = lastFound; i < size; ++i)
        {
            if(flags[i].bEmpty) continue;
            auto& ent = objects[i];
            if((passWrecked && ((ent.UInt8At(58) & 0xF8) == 40 || (ent.UInt8At(1071) >> 6) & 1)) || ((ent.UInt8At(1070) >> 2) & 1)) continue;
            
            lastFound = i + 1;
            cleo->GetPointerToScriptVar(handle)->i = GetVehicleRef(ent.AsInt());
            UpdateCompareFlag(handle, true);
            return;
        }
    }
    else
    {
        for(int i = lastFound; i < size; ++i)
        {
            if(flags[i].bEmpty) continue;
            auto& ent = objects[i];
            if((passWrecked && ((ent.UInt8At(58) & 0xF8) == 40 || (ent.UInt8At(1071) >> 6) & 1)) || ((ent.UInt8At(1070) >> 2) & 1)) continue;
            if((ent.GetPos() - center).SqrMagnitude() <= sqrradius)
            {
                lastFound = i + 1;
                cleo->GetPointerToScriptVar(handle)->i = GetVehicleRef(ent.AsInt());
                UpdateCompareFlag(handle, true);
                return;
            }
        }
    }

    cleo->GetPointerToScriptVar(handle)->i = -1;
    UpdateCompareFlag(handle, false);
    lastFound = 0;
}

CLEO_Fn(GET_RANDOM_OBJECT_IN_SPHERE_NO_SAVE_RECURSIVE)
{
    GTAVector3D center;
    center.x = cleo->ReadParam(handle)->f;
    center.y = cleo->ReadParam(handle)->f;
    center.z = cleo->ReadParam(handle)->f;
    float radius = cleo->ReadParam(handle)->f, sqrradius = radius*radius;
    int next = cleo->ReadParam(handle)->i;

    static int lastFound = 0;
    if(!next) lastFound = 0;

    auto objects = *(GTAObjectSA**)(*objectPool + 0);
    tByteFlag* flags = *(tByteFlag**)(*objectPool + 4);
    int size = *(int*)(*objectPool + 8);

    if(radius >= 1000.0f)
    {
        for(int i = lastFound; i < size; ++i)
        {
            if(flags[i].bEmpty) continue;
            auto& ent = objects[i];
            if((ent.UInt8At(326) >> 6) & 1) continue;
            
            lastFound = i + 1;
            cleo->GetPointerToScriptVar(handle)->i = GetObjectRef(ent.AsInt());
            UpdateCompareFlag(handle, true);
            return;
        }
    }
    else
    {
        for(int i = lastFound; i < size; ++i)
        {
            if(flags[i].bEmpty) continue;
            auto& ent = objects[i];
            if((ent.UInt8At(326) >> 6) & 1) continue;
            if((ent.GetPos() - center).SqrMagnitude() <= sqrradius)
            {
                lastFound = i + 1;
                cleo->GetPointerToScriptVar(handle)->i = GetObjectRef(ent.AsInt());
                UpdateCompareFlag(handle, true);
                return;
            }
        }
    }

    cleo->GetPointerToScriptVar(handle)->i = -1;
    UpdateCompareFlag(handle, false);
    lastFound = 0;
}

CLEO_Fn(GET_PED_REF)
{
    int ref = cleo->ReadParam(handle)->i;
    cleo->GetPointerToScriptVar(handle)->i = GetPedRef(ref);
}

CLEO_Fn(DOES_DIRECTORY_EXIST)
{
    char filepath[128];
    CLEO_ReadStringEx(handle, filepath, sizeof(filepath)); filepath[sizeof(filepath)-1] = 0;
    int i = 0; while(filepath[i] != 0) // A little hack (cheeseburger is like WHAAAA)
    {
        if(filepath[i] == '\\') filepath[i] = '/';
        ++i;
    }
    
    std::string str = ResolvePath(handle, filepath);

    DIR* dir = opendir(str.c_str());
    UpdateCompareFlag(handle, dir != NULL);
    if(dir) closedir(dir);
}

CLEO_Fn(CREATE_DIRECTORY)
{
    char filepath[128];
    CLEO_ReadStringEx(handle, filepath, sizeof(filepath)); filepath[sizeof(filepath)-1] = 0;
    int i = 0; while(filepath[i] != 0) // A little hack (cheeseburger is like WHAAAA)
    {
        if(filepath[i] == '\\') filepath[i] = '/';
        ++i;
    }
    
    std::string str = ResolvePath(handle, filepath);

    int result = mkdir(str.c_str(), 0777);
    UpdateCompareFlag(handle, result == 0);
}

CLEO_Fn(FIND_FIRST_FILE)
{
    char filepath[128];
    CLEO_ReadStringEx(handle, filepath, sizeof(filepath)); filepath[sizeof(filepath)-1] = 0;
    int i = 0; while(filepath[i] != 0) // A little hack (cheeseburger is like WHAAAA)
    {
        if(filepath[i] == '\\') filepath[i] = '/';
        ++i;
    }
    
    std::string str = ResolvePath(handle, filepath);

    DIR* dir = opendir(str.c_str());
    if(dir)
    {
        struct dirent *entry;
        struct stat buf;
        char filecheckpath[256];
        while((entry = readdir(dir)) != NULL)
        {
            snprintf(filecheckpath, sizeof(filecheckpath), "%s/%s", str.c_str(), entry->d_name);
            lstat(filecheckpath, &buf);

            if(!S_ISDIR(buf.st_mode))
            {
                CLEO_DirScan *scan = new CLEO_DirScan;
                strncpy(scan->path, str.c_str(), sizeof(scan->path));
                scan->dir = dir;

                CLEO_WriteStringEx(handle, entry->d_name);
                cleo->GetPointerToScriptVar(handle)->i = (int)scan;
                UpdateCompareFlag(handle, true);
                return;
            }
        }
        closedir(dir);
    }

    cleo->GetPointerToScriptVar(handle)->i = 0;
    CLEO_WriteStringEx(handle, "");
    UpdateCompareFlag(handle, false);
}

CLEO_Fn(FIND_NEXT_FILE)
{
    CLEO_DirScan *scan = (CLEO_DirScan*)cleo->ReadParam(handle)->i;

    if(scan && scan->dir)
    {
        struct dirent *entry;
        struct stat buf;
        char filecheckpath[256];
        while((entry = readdir(scan->dir)) != NULL)
        {
            snprintf(filecheckpath, sizeof(filecheckpath), "%s/%s", scan->path, entry->d_name);
            lstat(filecheckpath, &buf);

            if(!S_ISDIR(buf.st_mode))
            {
                CLEO_WriteStringEx(handle, entry->d_name);
                UpdateCompareFlag(handle, true);
                return;
            }
        }
    }

    CLEO_WriteStringEx(handle, "");
    UpdateCompareFlag(handle, false);
}

CLEO_Fn(FIND_CLOSE)
{
    CLEO_DirScan *scan = (CLEO_DirScan*)cleo->ReadParam(handle)->i;
    closedir(scan->dir);
    delete scan;
}

CLEO_Fn(POP_FLOAT)
{
    cleo->GetPointerToScriptVar(handle)->f = 0.0f;
}

CLEO_Fn(GET_VEHICLE_REF)
{
    int ref = cleo->ReadParam(handle)->i;
    cleo->GetPointerToScriptVar(handle)->i = GetVehicleRef(ref);
}

CLEO_Fn(GET_OBJECT_REF)
{
    int ref = cleo->ReadParam(handle)->i;
    cleo->GetPointerToScriptVar(handle)->i = GetObjectRef(ref);
}

CLEO_Fn(STRING_FLOAT_FORMAT)
{
    // added to support old-ass scripts
    float val = cleo->ReadParam(handle)->f;
    
    char fmt[MAX_STR_LEN], str[MAX_STR_LEN];
    CLEO_ReadStringEx(handle, fmt, sizeof(fmt));
    snprintf(str, sizeof(str), fmt, val);
}

CLEO_Fn(POW)
{
    float base = cleo->ReadParam(handle)->f;
    float arg = cleo->ReadParam(handle)->f;
    cleo->GetPointerToScriptVar(handle)->f = powf(base, arg);
}

CLEO_Fn(LOG)
{
    float arg = cleo->ReadParam(handle)->f;
    float base = cleo->ReadParam(handle)->f;
    cleo->GetPointerToScriptVar(handle)->f = (float)(logf(arg) / logf(base));
}

/////////////////////////////////////////////////////
/////////// BEGIN OPCODES by MatiDragon /////////////
/////////////////////////////////////////////////////

/*
CLEO_Fn(SET_WIDGET_TRANSFORM)
{
    // Widget ID
    int buttonId = cleo->ReadParam(handle)->i;

    // Transform
    float x = cleo->ReadParam(handle)->f;      // Coord X
    float y = cleo->ReadParam(handle)->f;      // Coord Y
    float width = cleo->ReadParam(handle)->f;  // Width
    float height = cleo->ReadParam(handle)->f; // Height

    uintptr_t widgetsAddr = (*TouchInterfaceWidgets);

    // Calculate button address
    widgetsAddr += buttonId * 4;
    uintptr_t buttonPtr = *(uintptr_t*)widgetsAddr;

    if (buttonPtr)
    {
        buttonPtr += 12; // Offset to access position
        *(float*)buttonPtr = x;      // Write X position
        buttonPtr += 4;
        *(float*)buttonPtr = y;      // Write Y position
        buttonPtr += 4;
        *(float*)buttonPtr = width;  // Write width
        buttonPtr += 4;
        *(float*)buttonPtr = height; // Write height
    }

    UpdateCompareFlag(handle, buttonPtr != 0); // Update compare flag
}

CLEO_Fn(IS_TOUCH_PRESSED)
{
    uintptr_t touchDownAddr = cleo->TouchInterfaceTouchDown();

    uint8_t isPressed = *(uint8_t*)touchDownAddr; // Read touch state

    // Update compare flag
    UpdateCompareFlag(handle, isPressed != 0);

    // Optional return: if the script requests to store the value
    if (GetVarArgCount(handle) > 0)
    {
        cleo->GetPointerToScriptVar(handle)->i = isPressed;
    }
}

CLEO_Fn(GET_TOUCH_XY)
{
    uintptr_t touchPosAddr = cleo->TouchInterfaceCachedPos();

    float x = *(float*)touchPosAddr;       // X
    float y = *(float*)(touchPosAddr + 4); // Y

    cleo->GetPointerToScriptVar(handle)->i = (int)x; // Convert to integer
    cleo->GetPointerToScriptVar(handle)->i = (int)y; // Convert to integer
}
*/
CLEO_Fn(CREATE_FILE_OR_DIRECTORY)
{
    char filepath[256];
    CLEO_ReadStringEx(handle, filepath, sizeof(filepath));

    // Validar que la ruta no esté vacía
    if (strlen(filepath) == 0)
    {
        UpdateCompareFlag(handle, false);
        return;
    }

    // Resolver ruta
    std::string path = ResolvePath(handle, filepath);

    // Verificar si ya existe
    struct stat info;
    if (stat(path.c_str(), &info) == 0)
    {
        UpdateCompareFlag(handle, true); // Ya existe
        return;
    }

    // Crear archivo o carpeta
    if (filepath[strlen(filepath) - 1] == '/') // Si termina en '/', es una carpeta
    {
        int result = mkdir(path.c_str(), 0777);
        UpdateCompareFlag(handle, result == 0);
    }
    else // Si no, es un archivo
    {
        FILE* file = fopen(path.c_str(), "w");
        if (file)
        {
            fclose(file);
            UpdateCompareFlag(handle, true);
        }
        else
        {
            UpdateCompareFlag(handle, false);
        }
    }
}

CLEO_Fn(NORMALIZE_ANGLE_DEGREES)
{
    float* angle = &cleo->GetPointerToScriptVar(handle)->f;
    while (*angle >= 360.0f) *angle -= 360.0f;
    while (*angle < 0.0f)    *angle += 360.0f;
}

CLEO_Fn(NORMALIZE_ANGLE_RADIANS)
{
    float* rad = &cleo->GetPointerToScriptVar(handle)->f;
    const float TWO_PI = 6.2831853072f;
    while (*rad >= TWO_PI) *rad -= TWO_PI;
    while (*rad < 0.0f)     *rad += TWO_PI;
}

CLEO_Fn(TOGGLE_BOOLEAN_VAR)
{
    int* var = &cleo->GetPointerToScriptVar(handle)->i;
    *var = (*var == 0) ? 1 : 0;
}

CLEO_Fn(FLOAT_DIV)
{
    float a = cleo->ReadParam(handle)->f;
    float b = cleo->ReadParam(handle)->f;
    cleo->GetPointerToScriptVar(handle)->f = a / b;
}

CLEO_Fn(FLOAT_MUL)
{
    float a = cleo->ReadParam(handle)->f;
    float b = cleo->ReadParam(handle)->f;
    cleo->GetPointerToScriptVar(handle)->f = a * b;
}

CLEO_Fn(FLOAT_SUM)
{
    float a = cleo->ReadParam(handle)->f;
    float b = cleo->ReadParam(handle)->f;
    cleo->GetPointerToScriptVar(handle)->f = a + b;
}

CLEO_Fn(FLOAT_SUB)
{
    float a = cleo->ReadParam(handle)->f;
    float b = cleo->ReadParam(handle)->f;
    cleo->GetPointerToScriptVar(handle)->f = a - b;
}

CLEO_Fn(SPLIT_FLOAT_TO_SIGNED_PARTS)
{
    float v = cleo->ReadParam(handle)->f;
    int decimals = cleo->ReadParam(handle)->i;

    // Clamp decimals to [0,9] to avoid overflow on large multipliers
    if (decimals < 0) decimals = 0;
    if (decimals > 9) decimals = 9;

    // Sign and absolute value
    int sign = (v < 0.0f) ? -1 : 1;
    float absv = fabsf(v);

    // Integer part (trunc toward zero)
    int intPart = (int)absv;
    intPart *= sign; // restore sign for integer part

    // Fractional part: take absolute fractional, scale, then truncate (no rounding)
    float frac = absv - (float)((int)absv);
    int multiplier = 1;
    for (int i = 0; i < decimals; ++i) multiplier *= 10;

    int fracPart = 0;
    if (multiplier > 1)
    {
        fracPart = (int)(frac * (float)multiplier); // truncates toward zero
    }
    else
    {
        fracPart = 0;
    }

    fracPart *= sign; // make fraction share the sign of original value

    // Return values: first int (integer part), second int (fractional scaled)
    cleo->GetPointerToScriptVar(handle)->i = intPart;
    cleo->GetPointerToScriptVar(handle)->i = fracPart;

    UpdateCompareFlag(handle, true);
}

CLEO_Fn(FILE_RENAME)
{
    char srcPathBuf[256];
    char dstPathBuf[256];
    CLEO_ReadStringEx(handle, srcPathBuf, sizeof(srcPathBuf));
    CLEO_ReadStringEx(handle, dstPathBuf, sizeof(dstPathBuf));
    srcPathBuf[sizeof(srcPathBuf)-1] = 0;
    dstPathBuf[sizeof(dstPathBuf)-1] = 0;

    // normalizar slashes
    for (int i = 0; srcPathBuf[i]; ++i) if (srcPathBuf[i] == '\\') srcPathBuf[i] = '/';
    for (int i = 0; dstPathBuf[i]; ++i) if (dstPathBuf[i] == '\\') dstPathBuf[i] = '/';

    std::string src = ResolvePath(handle, srcPathBuf);
    std::string dst = ResolvePath(handle, dstPathBuf);

    // Intentar rename directo (mover/renombrar)
    int res = rename(src.c_str(), dst.c_str());
    if (res == 0)
    {
        UpdateCompareFlag(handle, true);
        return;
    }

    // Si falla el rename, intentar fallback copy + remove (útil entre dispositivos/FS distintos)
    FILE* fin = fopen(src.c_str(), "rb");
    if (!fin)
    {
        UpdateCompareFlag(handle, false);
        return;
    }
    FILE* fout = fopen(dst.c_str(), "wb");
    if (!fout)
    {
        fclose(fin);
        UpdateCompareFlag(handle, false);
        return;
    }

    const size_t BUF_SIZE = 8192;
    char *buffer = (char*)malloc(BUF_SIZE);
    if (!buffer)
    {
        fclose(fin);
        fclose(fout);
        UpdateCompareFlag(handle, false);
        return;
    }

    bool ok = true;
    size_t n;
    while ((n = fread(buffer, 1, BUF_SIZE, fin)) > 0)
    {
        if (fwrite(buffer, 1, n, fout) != n)
        {
            ok = false;
            break;
        }
    }

    free(buffer);
    fclose(fin);
    fflush(fout);
    fclose(fout);

    if (!ok)
    {
        // intento fallido: borrar destino parcial
        remove(dst.c_str());
        UpdateCompareFlag(handle, false);
        return;
    }

    // borrar origen original si la copia fue exitosa
    if (remove(src.c_str()) != 0)
    {
        // copia exitosa pero no se pudo borrar origen => consideramos fallo
        // opcional: podríamos retornar true y dejar origen; aquí devolvemos false
        UpdateCompareFlag(handle, false);
        return;
    }

    UpdateCompareFlag(handle, true);
}

// --- helpers: conversions (simple, comentadas) ------------------------

// Helpers clamp
static float clampf(float v, float a, float b) { if (v < a) return a; if (v > b) return b; return v; }
static int clampi(int v, int a, int b) { if (v < a) return a; if (v > b) return b; return v; }

// RGB(0..255) -> HSV(H:0..360 int, S:0..100 int, V:0..100 int)
static void RGB_to_HSV_int_scale(int r, int g, int b, int &outH, int &outS, int &outV)
{
    float rf = r / 255.0f;
    float gf = g / 255.0f;
    float bf = b / 255.0f;

    float maxv = rf; if (gf > maxv) maxv = gf; if (bf > maxv) maxv = bf;
    float minv = rf; if (gf < minv) minv = gf; if (bf < minv) minv = bf;
    float delta = maxv - minv;

    float h = 0.0f;
    if (delta <= 1e-6f) h = 0.0f;
    else if (maxv == rf) h = 60.0f * fmodf(((gf - bf) / delta), 6.0f);
    else if (maxv == gf) h = 60.0f * (((bf - rf) / delta) + 2.0f);
    else h = 60.0f * (((rf - gf) / delta) + 4.0f);
    if (h < 0.0f) h += 360.0f;

    float s = (maxv <= 1e-6f) ? 0.0f : (delta / maxv);
    float v = maxv;

    outH = clampi((int)roundf(h), 0, 360);
    outS = clampi((int)roundf(s * 100.0f), 0, 100);
    outV = clampi((int)roundf(v * 100.0f), 0, 100);
}

// HSV(H:0..360, S:0..100, V:0..100) -> RGB(0..255)
static void HSV_to_RGB_int_scale(int H, int S, int V, int &outR, int &outG, int &outB)
{
    float h = (float)H;
    float s = (float)S / 100.0f;
    float v = (float)V / 100.0f;

    if (s <= 0.0f) {
        int val = clampi((int)roundf(v * 255.0f), 0, 255);
        outR = outG = outB = val;
        return;
    }

    float hh = fmodf(h, 360.0f) / 60.0f;
    if (hh < 0.0f) hh += 6.0f;
    int i = (int)floorf(hh);
    float f = hh - i;
    float p = v * (1.0f - s);
    float q = v * (1.0f - s * f);
    float t = v * (1.0f - s * (1.0f - f));

    float rf=0, gf=0, bf=0;
    switch(i) {
        case 0: rf = v; gf = t; bf = p; break;
        case 1: rf = q; gf = v; bf = p; break;
        case 2: rf = p; gf = v; bf = t; break;
        case 3: rf = p; gf = q; bf = v; break;
        case 4: rf = t; gf = p; bf = v; break;
        default: rf = v; gf = p; bf = q; break;
    }

    outR = clampi((int)roundf(rf * 255.0f), 0, 255);
    outG = clampi((int)roundf(gf * 255.0f), 0, 255);
    outB = clampi((int)roundf(bf * 255.0f), 0, 255);
}

// RGB -> HSL (H:0..360, S:0..100, L:0..100)
static void RGB_to_HSL_int_scale(int r, int g, int b, int &outH, int &outS, int &outL)
{
    float rf = r / 255.0f;
    float gf = g / 255.0f;
    float bf = b / 255.0f;

    float maxv = rf; if (gf > maxv) maxv = gf; if (bf > maxv) maxv = bf;
    float minv = rf; if (gf < minv) minv = gf; if (bf < minv) minv = bf;
    float l = (maxv + minv) * 0.5f;
    float delta = maxv - minv;

    float h = 0.0f;
    float s = 0.0f;

    if (delta <= 1e-6f) {
        h = 0.0f; s = 0.0f;
    } else {
        if (l < 0.5f) s = delta / (maxv + minv);
        else s = delta / (2.0f - maxv - minv);

        if (maxv == rf) h = 60.0f * fmodf(((gf - bf) / delta), 6.0f);
        else if (maxv == gf) h = 60.0f * (((bf - rf) / delta) + 2.0f);
        else h = 60.0f * (((rf - gf) / delta) + 4.0f);
        if (h < 0.0f) h += 360.0f;
    }

    outH = clampi((int)roundf(h), 0, 360);
    outS = clampi((int)roundf(s * 100.0f), 0, 100);
    outL = clampi((int)roundf(l * 100.0f), 0, 100);
}

// HSL -> RGB
static void HSL_to_RGB_int_scale(int H, int S, int L, int &outR, int &outG, int &outB)
{
    float h = fmodf((float)H, 360.0f);
    if (h < 0.0f) h += 360.0f;
    float s = (float)S / 100.0f;
    float l = (float)L / 100.0f;

    float rf, gf, bf;
    if (s <= 0.0f) {
        rf = gf = bf = l;
    } else {
        float q = (l < 0.5f) ? (l * (1.0f + s)) : (l + s - l * s);
        float p = 2.0f * l - q;
        float hk = h / 360.0f;
        auto hue_to_rgb_local = [](float p, float q, float t)->float {
            if (t < 0.0f) t += 1.0f;
            if (t > 1.0f) t -= 1.0f;
            if (t < 1.0f/6.0f) return p + (q - p) * 6.0f * t;
            if (t < 1.0f/2.0f) return q;
            if (t < 2.0f/3.0f) return p + (q - p) * (2.0f/3.0f - t) * 6.0f;
            return p;
        };
        rf = hue_to_rgb_local(p, q, hk + 1.0f/3.0f);
        gf = hue_to_rgb_local(p, q, hk);
        bf = hue_to_rgb_local(p, q, hk - 1.0f/3.0f);
    }

    outR = clampi((int)roundf(rf * 255.0f), 0, 255);
    outG = clampi((int)roundf(gf * 255.0f), 0, 255);
    outB = clampi((int)roundf(bf * 255.0f), 0, 255);
}

// ---------------- CLEO opcodes: ALPHA OBLIGATORIA --------------------

// Firma obligatoria: todos reciben y devuelven A (0..255).

// r g b a = CONV_RGB_TO_HSV_INT r g b a
CLEO_Fn(CONV_RGB_TO_HSV_INT)
{
    int r = cleo->ReadParam(handle)->i;
    int g = cleo->ReadParam(handle)->i;
    int b = cleo->ReadParam(handle)->i;
    int a = cleo->ReadParam(handle)->i; // OBLIGATORIO

    int H,S,V;
    RGB_to_HSV_int_scale(r,g,b,H,S,V);

    cleo->GetPointerToScriptVar(handle)->i = H;
    cleo->GetPointerToScriptVar(handle)->i = S;
    cleo->GetPointerToScriptVar(handle)->i = V;
    cleo->GetPointerToScriptVar(handle)->i = a;
}

// r g b a = CONV_HSV_TO_RGB_INT H S V A
CLEO_Fn(CONV_HSV_TO_RGB_INT)
{
    int H = cleo->ReadParam(handle)->i;
    int S = cleo->ReadParam(handle)->i;
    int V = cleo->ReadParam(handle)->i;
    int a = cleo->ReadParam(handle)->i; // OBLIGATORIO

    int r,g,b;
    HSV_to_RGB_int_scale(H,S,V,r,g,b);

    cleo->GetPointerToScriptVar(handle)->i = r;
    cleo->GetPointerToScriptVar(handle)->i = g;
    cleo->GetPointerToScriptVar(handle)->i = b;
    cleo->GetPointerToScriptVar(handle)->i = a;
}

// H S L a = CONV_RGB_TO_HSL_INT r g b a
CLEO_Fn(CONV_RGB_TO_HSL_INT)
{
    int r = cleo->ReadParam(handle)->i;
    int g = cleo->ReadParam(handle)->i;
    int b = cleo->ReadParam(handle)->i;
    int a = cleo->ReadParam(handle)->i; // OBLIGATORIO

    int H,S,L;
    RGB_to_HSL_int_scale(r,g,b,H,S,L);

    cleo->GetPointerToScriptVar(handle)->i = H;
    cleo->GetPointerToScriptVar(handle)->i = S;
    cleo->GetPointerToScriptVar(handle)->i = L;
    cleo->GetPointerToScriptVar(handle)->i = a;
}

// r g b a = CONV_HSL_TO_RGB_INT H S L A
CLEO_Fn(CONV_HSL_TO_RGB_INT)
{
    int H = cleo->ReadParam(handle)->i;
    int S = cleo->ReadParam(handle)->i;
    int L = cleo->ReadParam(handle)->i;
    int a = cleo->ReadParam(handle)->i; // OBLIGATORIO

    int r,g,b;
    HSL_to_RGB_int_scale(H,S,L,r,g,b);

    cleo->GetPointerToScriptVar(handle)->i = r;
    cleo->GetPointerToScriptVar(handle)->i = g;
    cleo->GetPointerToScriptVar(handle)->i = b;
    cleo->GetPointerToScriptVar(handle)->i = a;
}

// Helpers mínimos (asume ya tienes clampi, HSV/RGB/HSL helpers previos)
static int lerp_int(int a, int b, int tInt) {
    if (tInt <= 0) return a;
    if (tInt >= 100) return b;
    return clampi((int)roundf(a + (b - a) * (tInt / 100.0f)), 0, 255);
}
static int lerp_percent_int(int a, int b, int tInt, int lo, int hi) {
    int v = clampi((int)roundf(a + (b - a) * (tInt / 100.0f)), lo, hi);
    return v;
}

// r g b a = RGB_LERP_INT sr sg sb sa er eg eb ea tInt
CLEO_Fn(RGB_LERP_INT)
{
    int sr = cleo->ReadParam(handle)->i;
    int sg = cleo->ReadParam(handle)->i;
    int sb = cleo->ReadParam(handle)->i;
    int sa = cleo->ReadParam(handle)->i;

    int er = cleo->ReadParam(handle)->i;
    int eg = cleo->ReadParam(handle)->i;
    int eb = cleo->ReadParam(handle)->i;
    int ea = cleo->ReadParam(handle)->i;

    int tInt = cleo->ReadParam(handle)->i; // 0..100
    if (tInt < 0) tInt = 0; if (tInt > 100) tInt = 100;

    int r = lerp_int(sr, er, tInt);
    int g = lerp_int(sg, eg, tInt);
    int b = lerp_int(sb, eb, tInt);
    int a = lerp_int(sa, ea, tInt);

    cleo->GetPointerToScriptVar(handle)->i = r;
    cleo->GetPointerToScriptVar(handle)->i = g;
    cleo->GetPointerToScriptVar(handle)->i = b;
    cleo->GetPointerToScriptVar(handle)->i = a;
}

// r g b a = HSV_LERP_INT sr sg sb sa er eg eb ea tInt mode
CLEO_Fn(HSV_LERP_INT)
{
    int sr = cleo->ReadParam(handle)->i;
    int sg = cleo->ReadParam(handle)->i;
    int sb = cleo->ReadParam(handle)->i;
    int sa = cleo->ReadParam(handle)->i;

    int er = cleo->ReadParam(handle)->i;
    int eg = cleo->ReadParam(handle)->i;
    int eb = cleo->ReadParam(handle)->i;
    int ea = cleo->ReadParam(handle)->i;

    int tInt = cleo->ReadParam(handle)->i;
    int mode = cleo->ReadParam(handle)->i; // 0 short-arc, 1 direct
    if (tInt < 0) tInt = 0; if (tInt > 100) tInt = 100;
    float t = tInt / 100.0f;

    int sH,sS,sV;
    int eH,eS,eV;
    RGB_to_HSV_int_scale(sr,sg,sb,sH,sS,sV);
    RGB_to_HSV_int_scale(er,eg,eb,eH,eS,eV);

    float outHf;
    if (mode == 0) {
        float diff = (float)eH - (float)sH;
        if (diff > 180.0f) diff -= 360.0f;
        else if (diff < -180.0f) diff += 360.0f;
        outHf = (float)sH + diff * t;
    } else {
        outHf = (float)sH + ((float)eH - (float)sH) * t;
    }
    int outH = ((int)roundf(fmodf(outHf, 360.0f)));
    if (outH < 0) outH += 360;
    int outS = clampi((int)roundf(sS + (eS - sS) * t), 0, 100);
    int outV = clampi((int)roundf(sV + (eV - sV) * t), 0, 100);

    int r,g,b;
    HSV_to_RGB_int_scale(outH, outS, outV, r, g, b);
    int outA = clampi((int)roundf(sa + (ea - sa) * t), 0, 255);

    cleo->GetPointerToScriptVar(handle)->i = r;
    cleo->GetPointerToScriptVar(handle)->i = g;
    cleo->GetPointerToScriptVar(handle)->i = b;
    cleo->GetPointerToScriptVar(handle)->i = outA;
}

// r g b a = HSL_LERP_INT sr sg sb sa er eg eb ea tInt
CLEO_Fn(HSL_LERP_INT)
{
    int sr = cleo->ReadParam(handle)->i;
    int sg = cleo->ReadParam(handle)->i;
    int sb = cleo->ReadParam(handle)->i;
    int sa = cleo->ReadParam(handle)->i;

    int er = cleo->ReadParam(handle)->i;
    int eg = cleo->ReadParam(handle)->i;
    int eb = cleo->ReadParam(handle)->i;
    int ea = cleo->ReadParam(handle)->i;

    int tInt = cleo->ReadParam(handle)->i;
    if (tInt < 0) tInt = 0; if (tInt > 100) tInt = 100;
    float t = tInt / 100.0f;

    int sH,sS,sL;
    int eH,eS,eL;
    RGB_to_HSL_int_scale(sr,sg,sb,sH,sS,sL);
    RGB_to_HSL_int_scale(er,eg,eb,eH,eS,eL);

    float diff = (float)eH - (float)sH;
    if (diff > 180.0f) diff -= 360.0f;
    else if (diff < -180.0f) diff += 360.0f;
    float outHf = (float)sH + diff * t;
    int outH = ((int)roundf(fmodf(outHf, 360.0f)));
    if (outH < 0) outH += 360;
    int outS = clampi((int)roundf(sS + (eS - sS) * t), 0, 100);
    int outL = clampi((int)roundf(sL + (eL - sL) * t), 0, 100);

    int r,g,b;
    HSL_to_RGB_int_scale(outH, outS, outL, r, g, b);
    int outA = clampi((int)roundf(sa + (ea - sa) * t), 0, 255);

    cleo->GetPointerToScriptVar(handle)->i = r;
    cleo->GetPointerToScriptVar(handle)->i = g;
    cleo->GetPointerToScriptVar(handle)->i = b;
    cleo->GetPointerToScriptVar(handle)->i = outA;
}

// r g b a = BLEND_RGBA_INT srcR srcG srcB srcA dstR dstG dstB dstA mode
CLEO_Fn(BLEND_RGBA_INT)
{
    int sR = cleo->ReadParam(handle)->i;
    int sG = cleo->ReadParam(handle)->i;
    int sB = cleo->ReadParam(handle)->i;
    int sA = cleo->ReadParam(handle)->i;

    int dR = cleo->ReadParam(handle)->i;
    int dG = cleo->ReadParam(handle)->i;
    int dB = cleo->ReadParam(handle)->i;
    int dA = cleo->ReadParam(handle)->i;

    int mode = cleo->ReadParam(handle)->i;

    // Convert to premultiplied floats 0..1
    float sa = sA / 255.0f, da = dA / 255.0f;
    float sr = (sR / 255.0f), sg = (sG / 255.0f), sb = (sB / 255.0f);
    float dr = (dR / 255.0f), dg = (dG / 255.0f), db = (dB / 255.0f);

    float outRf=0, outGf=0, outBf=0;
    switch(mode) {
        case 0: // add (clamped)
            outRf = sr*sa + dr*da;
            outGf = sg*sa + dg*da;
            outBf = sb*sa + db*da;
            break;
        case 1: // multiply
            outRf = (sr*dr);
            outGf = (sg*dg);
            outBf = (sb*db);
            break;
        case 2: // screen: 1 - (1-a)*(1-b)
            outRf = 1.0f - (1.0f - sr)*(1.0f - dr);
            outGf = 1.0f - (1.0f - sg)*(1.0f - dg);
            outBf = 1.0f - (1.0f - sb)*(1.0f - db);
            break;
        case 3: // overlay (approx)
            outRf = (dr <= 0.5f) ? (2.0f * sr * dr) : (1.0f - 2.0f * (1.0f - sr) * (1.0f - dr));
            outGf = (dg <= 0.5f) ? (2.0f * sg * dg) : (1.0f - 2.0f * (1.0f - sg) * (1.0f - dg));
            outBf = (db <= 0.5f) ? (2.0f * sb * db) : (1.0f - 2.0f * (1.0f - sb) * (1.0f - db));
            break;
        default:
            // fallback simple alpha-over (src over dst)
            outRf = sr*sa + dr*(1.0f - sa);
            outGf = sg*sa + dg*(1.0f - sa);
            outBf = sb*sa + db*(1.0f - sa);
            break;
    }

    // simple alpha composite: outA = lerp(dstA, srcA, srcA) approx src-over
    float outAf = sa + da * (1.0f - sa);
    // convert back to 0..255 clamped
    int orr = clampi((int)roundf(outRf * 255.0f), 0, 255);
    int org = clampi((int)roundf(outGf * 255.0f), 0, 255);
    int orb = clampi((int)roundf(outBf * 255.0f), 0, 255);
    int oa = clampi((int)roundf(outAf * 255.0f), 0, 255);

    cleo->GetPointerToScriptVar(handle)->i = orr;
    cleo->GetPointerToScriptVar(handle)->i = org;
    cleo->GetPointerToScriptVar(handle)->i = orb;
    cleo->GetPointerToScriptVar(handle)->i = oa;
}

// Firm: result = INT_RULE_OF_THREE A B C
CLEO_Fn(INT_RULE_OF_THREE)
{
    int A = cleo->ReadParam(handle)->i;
    int B = cleo->ReadParam(handle)->i;
    int C = cleo->ReadParam(handle)->i;

    int result = 0;
    if (A != 0)
        result = (B * C) / A;

    cleo->GetPointerToScriptVar(handle)->i = result;
}

// Firm: result = FLOAT_RULE_OF_THREE A B C
CLEO_Fn(FLOAT_RULE_OF_THREE)
{
    float A = cleo->ReadParam(handle)->f;
    float B = cleo->ReadParam(handle)->f;
    float C = cleo->ReadParam(handle)->f;

    float result = 0.0f;
    if (fabsf(A) > 1e-6f)
        result = (B * C) / A;

    cleo->GetPointerToScriptVar(handle)->f = result;
}

// Helpers (float)
static inline float DegToRadF(float deg) { return deg * (3.14159265358979323846f / 180.0f); }
static inline bool IsFiniteFloat(float v) { return std::isfinite(v); }

// ORBIT_2D (float)
CLEO_Fn(ORBIT_2D)
{
    int angleMode = cleo->ReadParam(handle)->i;
    float angle = cleo->ReadParam(handle)->f;
    float radius = cleo->ReadParam(handle)->f;
    float cx = cleo->ReadParam(handle)->f;
    float cy = cleo->ReadParam(handle)->f;

    if (angleMode == 0) angle = DegToRadF(angle);

    if (!IsFiniteFloat(angle) || !IsFiniteFloat(radius) || !IsFiniteFloat(cx) || !IsFiniteFloat(cy))
    {
        UpdateCompareFlag(handle, false);
        return;
    }

    float x = cosf(angle) * radius + cx;
    float y = sinf(angle) * radius + cy;

    cleo->GetPointerToScriptVar(handle)->f = x;
    cleo->GetPointerToScriptVar(handle)->f = y;

    UpdateCompareFlag(handle, true);
}

// ORBIT_3D (float)
CLEO_Fn(ORBIT_3D)
{
    int angleMode = cleo->ReadParam(handle)->i;
    float ax = cleo->ReadParam(handle)->f;
    float ay = cleo->ReadParam(handle)->f;
    float radius = cleo->ReadParam(handle)->f;
    float cx = cleo->ReadParam(handle)->f;
    float cy = cleo->ReadParam(handle)->f;
    float cz = cleo->ReadParam(handle)->f;

    if (angleMode == 0) { ax = DegToRadF(ax); ay = DegToRadF(ay); }

    if (!IsFiniteFloat(ax) || !IsFiniteFloat(ay) || !IsFiniteFloat(radius) ||
        !IsFiniteFloat(cx) || !IsFiniteFloat(cy) || !IsFiniteFloat(cz))
    {
        UpdateCompareFlag(handle, false);
        return;
    }

    float sax = sinf(ax);
    float cax = cosf(ax);
    float say = sinf(ay);
    float cay = cosf(ay);

    float x = sax * cay * radius + cx;
    float y = sax * say * radius + cy;
    float z = cax * radius + cz;

    cleo->GetPointerToScriptVar(handle)->f = x;
    cleo->GetPointerToScriptVar(handle)->f = y;
    cleo->GetPointerToScriptVar(handle)->f = z;

    UpdateCompareFlag(handle, true);
}



///////////////////////////////////////////////////
//////////// END OPCODES by MatiDragon ////////////
///////////////////////////////////////////////////

void Init4Opcodes()
{
    SET_TO(ScriptSpace,         cleo->GetMainLibrarySymbol("_ZN11CTheScripts11ScriptSpaceE"));
    SET_TO(UpdateCompareFlag,   cleo->GetMainLibrarySymbol("_ZN14CRunningScript17UpdateCompareFlagEh"));
    SET_TO(pActiveScripts,      cleo->GetMainLibrarySymbol("_ZN11CTheScripts14pActiveScriptsE"));
    SET_TO(pIdleScripts,        cleo->GetMainLibrarySymbol("_ZN11CTheScripts12pIdleScriptsE"));
    SET_TO(ScriptParams,        cleo->GetMainLibrarySymbol("ScriptParams"));
    SET_TO(GetPedFromRef,       cleo->GetMainLibrarySymbol("_ZN6CPools6GetPedEi"));
    SET_TO(GetVehicleFromRef,   cleo->GetMainLibrarySymbol("_ZN6CPools10GetVehicleEi"));
    SET_TO(GetObjectFromRef,    cleo->GetMainLibrarySymbol("_ZN6CPools9GetObjectEi"));
    SET_TO(GetPedRef,           cleo->GetMainLibrarySymbol("_ZN6CPools9GetPedRefEP4CPed"));
    SET_TO(GetVehicleRef,       cleo->GetMainLibrarySymbol("_ZN6CPools13GetVehicleRefEP8CVehicle"));
    SET_TO(GetObjectRef,        cleo->GetMainLibrarySymbol("_ZN6CPools12GetObjectRefEP7CObject"));
    SET_TO(AddBigMessage,       cleo->GetMainLibrarySymbol("_ZN9CMessages13AddBigMessageEPtjt"));
    SET_TO(CLEO_STD_PutStrToAlloced, nCLEOAddr + 0x8F08 + 0x1);
    SET_TO(CLEO_STD_AddToGxtStorage, nCLEOAddr + 0x96CC + 0x1);
    SET_TO(CLEO_STD_DeallocStorage,  nCLEOAddr + 0x5F34 + 0x1);
    SET_TO(TheText,             cleo->GetMainLibrarySymbol("TheText"));
    SET_TO(TextGet,             cleo->GetMainLibrarySymbol("_ZN5CText3GetEPKc"));
    SET_TO(m_CheatString,       cleo->GetMainLibrarySymbol("_ZN6CCheat13m_CheatStringE"));
    SET_TO(keys,                cleo->GetMainLibrarySymbol("keys"));
    SET_TO(ms_modelInfoPtrs,    *(uintptr_t*)(nGameAddr + (*nGameIdent == GTASA ? 0x6796D4 : 0x394D94)));
    if(*nGameIdent == GTASA)
    {
        SET_TO(gMobileMenu,                     cleo->GetMainLibrarySymbol("gMobileMenu"));
        SET_TO(ms_RadarTrace,                   *(uintptr_t*)(nGameAddr + 0x6773CC));
        SET_TO(FindGroundZForCoord,             cleo->GetMainLibrarySymbol("_ZN6CWorld19FindGroundZForCoordEff"));
        SET_TO(LocalVariablesForCurrentMission, cleo->GetMainLibrarySymbol("_ZN11CTheScripts31LocalVariablesForCurrentMissionE"));
        SET_TO(CollectParameters_SA,            cleo->GetMainLibrarySymbol("_ZN14CRunningScript17CollectParametersEs"));
        SET_TO(StoreParameters_SA,              cleo->GetMainLibrarySymbol("_ZN14CRunningScript15StoreParametersEs"));
        SET_TO(SetHelpMessage_SA,               cleo->GetMainLibrarySymbol("_ZN4CHud14SetHelpMessageEPKcPtbbbj"));
        SET_TO(FindPlayerPed,                   cleo->GetMainLibrarySymbol("_Z13FindPlayerPedi"));
        SET_TO(pedPool,                         cleo->GetMainLibrarySymbol("_ZN6CPools11ms_pPedPoolE"));
        SET_TO(vehiclePool,                     cleo->GetMainLibrarySymbol("_ZN6CPools15ms_pVehiclePoolE"));
        SET_TO(objectPool,                      cleo->GetMainLibrarySymbol("_ZN6CPools14ms_pObjectPoolE"));
        SET_TO(AddMessage_SA,                   cleo->GetMainLibrarySymbol("_ZN9CMessages10AddMessageEPKcPtjtb"));
        SET_TO(AddMessageJumpQ_SA,              cleo->GetMainLibrarySymbol("_ZN9CMessages15AddMessageJumpQEPKcPtjtb"));
        SET_TO(SpawnCar,                        cleo->GetMainLibrarySymbol("_ZN6CCheat12VehicleCheatEi"));
        SET_TO(IsHIDPressed,                    cleo->GetMainLibrarySymbol("_ZN4CHID9IsPressedE10HIDMappingPf"));
        SET_TO(ClearAllCrosshairs,              cleo->GetMainLibrarySymbol("_ZN14CWeaponEffects18ClearAllCrosshairsEv"));
        SET_TO(SetWeaponLockOnTarget,           cleo->GetMainLibrarySymbol("_ZN4CPed21SetWeaponLockOnTargetEP7CEntity"));

        //SET_TO(TouchInterfaceWidgets,     cleo->GetMainLibrarySymbol("_ZN15CTouchInterface10m_pWidgetsE"));
        //SET_TO(TouchInterfaceTouchDown,   cleo->GetMainLibrarySymbol("_ZN15CTouchInterface12m_bTouchDownE"));
        //SET_TO(TouchInterfaceCachedPos, cleo->GetMainLibrarySymbol("_ZN15CTouchInterface14m_vecCachedPosE"));
    }
    else if(*nGameIdent == GTAVC)
    {
        SET_TO(CollectParameters_VC,            cleo->GetMainLibrarySymbol("_ZN14CRunningScript17CollectParametersEPjs"));
        SET_TO(StoreParameters_VC,              cleo->GetMainLibrarySymbol("_ZN14CRunningScript15StoreParametersEPjs"));
        SET_TO(SetHelpMessage_VC,               cleo->GetMainLibrarySymbol("_ZN4CHud14SetHelpMessageEPtbbb"));
        SET_TO(AddMessage_VC,                   cleo->GetMainLibrarySymbol("_ZN9CMessages10AddMessageEPtjt"));
        SET_TO(AddMessageJumpQ_VC,              cleo->GetMainLibrarySymbol("_ZN9CMessages15AddMessageJumpQEPtjt"));
        SET_TO(SpawnCar,                        cleo->GetMainLibrarySymbol("_Z12VehicleCheati"));
        SET_TO(curCheatCar_VC,                  cleo->GetMainLibrarySymbol("curCheatCar"));
    }
    
    //CLEO_RegisterOpcode(0x0A8C, WRITE_MEMORY); // WIDGET opcode on Mobile (thanks WarDrum, lol)
    //CLEO_RegisterOpcode(0x0A8D, READ_MEMORY); // WIDGET opcode on Mobile (thanks WarDrum, lol)
    CLEO_RegisterOpcode(0x0A8E, INT_ADD); // 0A8E=3,%3d% = %1d% + %2d% ; int
    CLEO_RegisterOpcode(0x0A8F, INT_SUB); // 0A8F=3,%3d% = %1d% - %2d% ; int
    CLEO_RegisterOpcode(0x0A90, INT_MUL); // 0A90=3,%3d% = %1d% * %2d% ; int
    CLEO_RegisterOpcode(0x0A91, INT_DIV); // 0A91=3,%3d% = %1d% / %2d% ; int
    //CLEO_RegisterOpcode(0x0A92, STREAM_CUSTOM_SCRIPT); // 0A92=-1,create_custom_thread %1d%
    CLEO_RegisterOpcode(0x0A93, TERMINATE_THIS_CUSTOM_SCRIPT); // 0A93=0,terminate_this_custom_script
    //CLEO_RegisterOpcode(0x0A94, LOAD_AND_LAUNCH_CUSTOM_MISSION); // 0A94=-1,create_custom_mission %1d%
    CLEO_RegisterOpcode(0x0A95, SAVE_THIS_CUSTOM_SCRIPT); // 0A95=0,enable_thread_saving
    CLEO_RegisterOpcode(0x0A96, GET_PED_POINTER); // 0A96=2,%2d% = actor %1d% struct
    CLEO_RegisterOpcode(0x0A97, GET_VEHICLE_POINTER); // 0A97=2,%2d% = car %1d% struct
    CLEO_RegisterOpcode(0x0A98, GET_OBJECT_POINTER); // 0A98=2,%2d% = object %1d% struct
    CLEO_RegisterOpcode(0x0A99, SET_CURRENT_DIRECTORY); // 0A99=1,set_current_directory %1b:userdir/rootdir%
    CLEO_RegisterOpcode(0x0A9A, OPEN_FILE); // 0A9A=3,%3d% = openfile %1d% mode %2d% // IF and SET
    CLEO_RegisterOpcode(0x0A9B, CLOSE_FILE); // 0A9B=1,closefile %1d%
    CLEO_RegisterOpcode(0x0A9C, GET_FILE_SIZE); // 0A9C=2,%2d% = file %1d% size
    CLEO_RegisterOpcode(0x0A9D, READ_FROM_FILE); // 0A9D=3,readfile %1d% size %2d% to %3d%
    CLEO_RegisterOpcode(0x0A9E, WRITE_TO_FILE); // 0A9E=3,writefile %1d% size %2d% from %3d%
    CLEO_RegisterOpcode(0x0A9F, GET_THIS_SCRIPT_STRUCT); // 0A9F=1,%1d% = current_thread_pointer
    CLEO_RegisterOpcode(0x0AA0, GOSUB_IF_FALSE); // 0AA0=1,gosub_if_false %1p%
    CLEO_RegisterOpcode(0x0AA1, RETURN_IF_FALSE); // 0AA1=0,return_if_false
    CLEO_RegisterOpcode(0x0AA2, LOAD_DYNAMIC_LIBRARY); // 0AA2=2,%2h% = load_library %1d% // IF and SET
    CLEO_RegisterOpcode(0x0AA3, FREE_DYNAMIC_LIBRARY); // 0AA3=1,free_library %1h%
    CLEO_RegisterOpcode(0x0AA4, GET_DYNAMIC_LIBRARY_PROCEDURE); // 0AA4=3,%3d% = get_proc_address %1d% library %2d% // IF and SET
    // 0AA5 - 0AA8 - Call funcs (we dont support such things, we have a different opcode on Android)
    //CLEO_RegisterOpcode(0x0AA5, CALL_FUNCTION); // 
    //CLEO_RegisterOpcode(0x0AA6, CALL_METHOD); // 
    //CLEO_RegisterOpcode(0x0AA7, CALL_FUNCTION_RETURN); // 
    //CLEO_RegisterOpcode(0x0AA8, CALL_METHOD_RETURN); // 
    CLEO_RegisterOpcode(0x0AA9, IS_GAME_VERSION_ORIGINAL); // 0AA9=0,is_game_version_original // always false, use 0DD6 (GET_GAME_VERSION) for Android
    CLEO_RegisterOpcode(0x0AAA, GET_SCRIPT_STRUCT_NAMED); // 0AAA=2,%2d% = thread %1d% pointer  // IF and SET
    CLEO_RegisterOpcode(0x0AAB, DOES_FILE_EXIST); // 0AAB=1,file_exists %1d%
    // 0AAC - 0AAF - AudioStreams
    CLEO_RegisterOpcode(0x0AB0, IS_KEY_PRESSED); // 0AB0=1,key_pressed %1d%
    CLEO_RegisterOpcode(0x0AB1, CLEO_CALL); // 0AB1=-1,call_scm_func %1p%
    CLEO_RegisterOpcode(0x0AB2, CLEO_RETURN); // 0AB2=-1,ret
    // Those are 0DDC and 0DDD on Mobile:
    CLEO_RegisterOpcode(0x0AB3, SET_CLEO_SHARED_VAR); // 0AB3=2,var %1d% = %2d%
    CLEO_RegisterOpcode(0x0AB4, GET_CLEO_SHARED_VAR); // 0AB4=2,%2d% = var %1d%

    if(*nGameIdent == GTASA)
    {
        CLEO_RegisterOpcode(0x0AB5, STORE_CLOSEST_ENTITIES); // 0AB5=3,store_actor %1d% closest_vehicle_to %2d% closest_ped_to %3d%
        CLEO_RegisterOpcode(0x0AB6, GET_TARGET_BLIP_COORDS); // 0AB6=3,store_target_marker_coords_to %1d% %2d% %3d% // IF and SET
    }
    CLEO_RegisterOpcode(0x0AB7, GET_CAR_NUMBER_OF_GEARS); // 0AB7=2,get_vehicle %1d% number_of_gears_to %2d%
    CLEO_RegisterOpcode(0x0AB8, GET_CAR_CURRENT_GEAR); // 0AB8=2,get_vehicle %1d% current_gear_to %2d%
    // 0AB9, 0ABB-0ABC - AudioStreams
    CLEO_RegisterOpcode(0x0ABA, TERMINATE_ALL_CUSTOM_SCRIPTS_WITH_THIS_NAME); // 0ABA=1,terminate_all_custom_scripts_with_this_name %1d%
    CLEO_RegisterOpcode(0x0ABD, IS_CAR_SIREN_ON); // 0ABD=1,vehicle %1d% siren_on
    CLEO_RegisterOpcode(0x0ABE, IS_CAR_ENGINE_ON); // 0ABE=1,vehicle %1d% engine_on
    CLEO_RegisterOpcode(0x0ABF, CLEO_SET_CAR_ENGINE_ON); // 0ABF=2,set_vehicle %1d% engine_state_to %2d%

    // 0AC0 - 0AC5 - AudioStreams
    CLEO_RegisterOpcode(0x0AC6, GET_LABEL_POINTER); // 0AC6=2,get_label_pointer %1d% store_to %2d%
    CLEO_RegisterOpcode(0x0AC7, GET_VAR_POINTER); // 0AC7=2,%2d% = var %1d% offset
    CLEO_RegisterOpcode(0x0AC8, ALLOCATE_MEMORY); // 0AC8=2,%2d% = allocate_memory_size %1d%
    CLEO_RegisterOpcode(0x0AC9, FREE_MEMORY); // 0AC9=1,free_allocated_memory %1d%
    CLEO_RegisterOpcode(0x0ACA, PRINT_HELP_STRING); // 0ACA=1,show_text_box %1d%
    CLEO_RegisterOpcode(0x0ACB, PRINT_BIG_STRING); // 0ACB=3,show_styled_text %1d% time %2d% style %3d%
    CLEO_RegisterOpcode(0x0ACC, PRINT_STRING); // 0ACC=2,show_text_lowpriority %1d% time %2d%
    CLEO_RegisterOpcode(0x0ACD, PRINT_STRING_NOW); // 0ACD=2,show_text_highpriority %1d% time %2d%
    CLEO_RegisterOpcode(0x0ACE, PRINT_HELP_FORMATTED); // 0ACE=-1,show_formatted_text_box %1d%
    CLEO_RegisterOpcode(0x0ACF, PRINT_BIG_FORMATTED); // 0ACF=-1,show_formatted_styled_text %1d% time %2d% style %3d%
    CLEO_RegisterOpcode(0x0AD0, PRINT_FORMATTED); // 0AD0=-1,show_formatted_text_lowpriority %1d% time %2d%
    CLEO_RegisterOpcode(0x0AD1, PRINT_FORMATTED_NOW); // 0AD1=-1,show_formatted_text_highpriority %1d% time %2d%
    CLEO_RegisterOpcode(0x0AD2, GET_CHAR_PLAYER_IS_TARGETING); // 0AD2=2,%2d% = player %1d% targeted_actor //IF and SET
    CLEO_RegisterOpcode(0x0AD3, STRING_FORMAT); // 0AD3=-1,string %1d% format %2d% ...
    CLEO_RegisterOpcode(0x0AD4, SCAN_STRING); // 0AD4=-1,%3d% = scan_string %1d% format %2d%  //IF and SET
    CLEO_RegisterOpcode(0x0AD5, FILE_SEEK); // 0AD5=3,file %1d% seek %2d% from_origin %3d% //IF and SET
    CLEO_RegisterOpcode(0x0AD6, IS_END_OF_FILE_REACHED); // 0AD6=1,end_of_file %1d% reached
    CLEO_RegisterOpcode(0x0AD7, READ_STRING_FROM_FILE); // 0AD7=3,read_string_from_file %1d% to %2d% size %3d% //IF and SET
    CLEO_RegisterOpcode(0x0AD8, WRITE_STRING_TO_FILE); // 0AD8=2,write_string_to_file %1d% from %2d% //IF and SET
    CLEO_RegisterOpcode(0x0AD9, WRITE_FORMATTED_STRING_TO_FILE); // 0AD9=-1,write_formated_text %2d% to_file %1d%
    CLEO_RegisterOpcode(0x0ADA, SCAN_FILE); // 0ADA=-1,%3d% = scan_file %1d% format %2d% //IF and SET
    CLEO_RegisterOpcode(0x0ADB, GET_NAME_OF_VEHICLE_MODEL); // 0ADB=2,%2d% = car_model %1o% name
    CLEO_RegisterOpcode(0x0ADC, TEST_CHEAT); // 0ADC=1,test_cheat %1d%
    CLEO_RegisterOpcode(0x0ADD, SPAWN_VEHICLE_BY_CHEATING); // 0ADD=1,spawn_car_with_model %1o% at_player_location //IF and SET // custom if-set condition
    CLEO_RegisterOpcode(0x0ADE, GET_TEXT_LABEL_STRING); // 0ADE=2,%2d% = text_by_GXT_entry %1d%
    CLEO_RegisterOpcode(0x0ADF, ADD_TEXT_LABEL); // 0ADF=2,add_dynamic_GXT_entry %1d% text %2d%
    CLEO_RegisterOpcode(0x0AE0, REMOVE_TEXT_LABEL); // 0AE0=1,remove_dynamic_GXT_entry %1d%

    if(*nGameIdent == GTASA)
    {
        CLEO_RegisterOpcode(0x0AE1, GET_RANDOM_CHAR_IN_SPHERE_NO_SAVE_RECURSIVE); // 0AE1=7,%7d% = find_actor_near_point %1d% %2d% %3d% in_radius %4d% find_next %5h% pass_deads %6h% //IF and SET
        CLEO_RegisterOpcode(0x0AE2, GET_RANDOM_CAR_IN_SPHERE_NO_SAVE_RECURSIVE); // 0AE2=7,%7d% = find_vehicle_near_point %1d% %2d% %3d% in_radius %4d% find_next %5h% pass_wrecked %6h% //IF and SET
        CLEO_RegisterOpcode(0x0AE3, GET_RANDOM_OBJECT_IN_SPHERE_NO_SAVE_RECURSIVE); // 0AE3=6,%6d% = find_object_near_point %1d% %2d% %3d% in_radius %4d% find_next %5h% //IF and SET
    }

    CLEO_RegisterOpcode(0x0AE4, DOES_DIRECTORY_EXIST); // 0AE4=1,directory_exist %1d%
    CLEO_RegisterOpcode(0x0AE5, CREATE_DIRECTORY); // 0AE5=1,create_directory %1d% //IF and SET
    CLEO_RegisterOpcode(0x0AE6, FIND_FIRST_FILE); // 0AE6=3,%2d% = find_first_file %1d% get_filename_to %3d% //IF and SET
    CLEO_RegisterOpcode(0x0AE7, FIND_NEXT_FILE); // 0AE7=2,%2d% = find_next_file %1d% //IF and SET
    CLEO_RegisterOpcode(0x0AE8, FIND_CLOSE); // 0AE8=1,find_close %1d%
    CLEO_RegisterOpcode(0x0AE9, POP_FLOAT); // 0AE9=1,pop_float store_to %1d% // popfloat? we have a different FPU logic
    CLEO_RegisterOpcode(0x0AEA, GET_PED_REF); // 0AEA=2,%2d% = actor_struct %1d% handle
    CLEO_RegisterOpcode(0x0AEB, GET_VEHICLE_REF); // 0AEB=2,%2d% = car_struct %1d% handle
    CLEO_RegisterOpcode(0x0AEC, GET_OBJECT_REF); // 0AEC=2,%2d% = object_struct %1d% handle
    CLEO_RegisterOpcode(0x0AED, STRING_FLOAT_FORMAT); // 0AED=3,%3d% = float %1d% to_string_format %2d%
    CLEO_RegisterOpcode(0x0AEE, POW); // 0AEE=3,%3d% = %1d% exp %2d% //all floats
    CLEO_RegisterOpcode(0x0AEF, LOG); // 0AEF=3,%3d% = log %1d% base %2d% //all floats

    // MatiDragon opcodes
    //CLEO_RegisterOpcode(0x7000, SET_WIDGET_TRANSFORM); // 7000=5,set_widget_transform %1d% coords %2d% %3d% scales %4d% %5d%
    //CLEO_RegisterOpcode(0x7001, IS_TOUCH_PRESSED); // 7001=1,is_touch_pressed store_to %1d%
    //CLEO_RegisterOpcode(0x7002, GET_TOUCH_XY); // 7002=2,get_touch_xy %1d% %2d%
    CLEO_RegisterOpcode(0x7003, CREATE_FILE_OR_DIRECTORY); // 7003=1,create_file_or_directory %1d%
    CLEO_RegisterOpcode(0x7004, NORMALIZE_ANGLE_DEGREES); // 7004=2,%2d% = normalize_angle_degrees %1d%
    CLEO_RegisterOpcode(0x7005, NORMALIZE_ANGLE_RADIANS); // 7005=2,%2d% = normalize_angle_radians %1d%
    CLEO_RegisterOpcode(0x7006, TOGGLE_BOOLEAN_VAR); // 7006=2,%2d% = !%1d% ; boolean
    CLEO_RegisterOpcode(0x7007, FLOAT_DIV); // 7007=3,%3d% = %1d% / %2d% ; float
    CLEO_RegisterOpcode(0x7008, FLOAT_MUL); // 7008=3,%3d% = %1d% * %2d% ; float
    CLEO_RegisterOpcode(0x7009, FLOAT_SUM); // 7009=3,%3d% = %1d% + %2d% ; float
    CLEO_RegisterOpcode(0x700A, FLOAT_SUB); // 700A=3,%3d% = %1d% - %2d% ; float
    CLEO_RegisterOpcode(0x700B, SPLIT_FLOAT_TO_SIGNED_PARTS); // 700B=4,%3d% %4d% = split_float_to_signed_parts %1d% decimals %2d%
    CLEO_RegisterOpcode(0x700C, FILE_RENAME); // 700C=2,file_rename %1d% to %2d%
    CLEO_RegisterOpcode(0x700D, CONV_RGB_TO_HSV_INT); // 700D=8,%5d% %6d% %7d% %8d% = CONV_RGB_TO_HSV_INT %1d% %2d% %3d% %4d%
    CLEO_RegisterOpcode(0x700E, CONV_HSV_TO_RGB_INT); // 700E=8,%5d% %6d% %7d% %8d% = CONV_HSV_TO_RGB_INT %1d% %2d% %3d% %4d%
    CLEO_RegisterOpcode(0x700F, CONV_RGB_TO_HSL_INT); // 700F=8,%5d% %6d% %7d% %8d% = CONV_RGB_TO_HSL_INT %1d% %2d% %3d% %4d%
    CLEO_RegisterOpcode(0x7010, CONV_HSL_TO_RGB_INT); // 7010=8,%5d% %6d% %7d% %8d% = CONV_HSL_TO_RGB_INT %1d% %2d% %3d% %4d%
    CLEO_RegisterOpcode(0x7011, RGB_LERP_INT); // 7011=13,%10d% %11d% %12d% %13d% = RGB_LERP_INT %1d% %2d% %3d% %4d% and %5d% %6d% %7d% %8d% percent %9d%
    CLEO_RegisterOpcode(0x7012, HSV_LERP_INT); // 7012=13,%10d% %11d% %12d% %13d% = HSV_LERP_INT %1d% %2d% %3d% %4d% and %5d% %6d% %7d% %8d% percent %9d%
    CLEO_RegisterOpcode(0x7013, HSL_LERP_INT); // 7013=13,%10d% %11d% %12d% %13d% = HSL_LERP_INT %1d% %2d% %3d% %4d% and %5d% %6d% %7d% %8d% percent %9d%
    CLEO_RegisterOpcode(0x7014, BLEND_RGBA_INT); // 7014=9,%5d% %6d% %7d% %8d% = BLEND_RGBA_INT %1d% %2d% %3d% %4d% and %5d% %6d% %7d% %8d% mode %9d%
    CLEO_RegisterOpcode(0x7015, INT_RULE_OF_THREE); // 7015=4,%4d% = %1d% * %2d% / %3d% ; int
    CLEO_RegisterOpcode(0x7016, FLOAT_RULE_OF_THREE); // 7016=4,%4d% = %1d% * %2d% / %3d% ; float
    CLEO_RegisterOpcode(0x7017, ORBIT_2D); // 7017=7,ORBIT_2D %6d% %7d% = angleMode %1d% angle %2d% radius %3d% cx %4d% cy %5d%
    CLEO_RegisterOpcode(0x7018, ORBIT_3D); // 7018=10,ORBIT_3D %8d% %9d% %10d% = angleMode %1d% ax %2d% ay %3d% radius %4d% cx %5d% cy %6d% cz %7d%
}

ScmFunction* ScmFunction::Store[store_size] = { NULL };
