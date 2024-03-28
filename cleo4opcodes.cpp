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
#define CLEO_RegisterOpcode(x, h) cleo->RegisterOpcode(x, h); cleo->RegisterOpcodeFunction(#h, h)
#define CLEO_Fn(h) void h (void *handle, uint32_t *ip, uint16_t opcode, const char *name)

extern uintptr_t nCLEOAddr;

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
GTAScript **pActiveScripts;
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
struct CLEOLocalVarSave
{
    int value;
    char strvalue[MAX_STR_LEN];
};
CLEOLocalVarSave localVarsSave[40];

// ----------------------------------- OPCODES!

CLEO_Fn(INT_ADD)
{
    int a = cleo->ReadParam(handle)->i;
    int b = cleo->ReadParam(handle)->i;
    cleo->GetPointerToScriptVar(handle)->i = a + b;
}

CLEO_Fn(INT_SUB)
{
    int a = cleo->ReadParam(handle)->i;
    int b = cleo->ReadParam(handle)->i;
    cleo->GetPointerToScriptVar(handle)->i = a - b;
}

CLEO_Fn(INT_MUL)
{
    int a = cleo->ReadParam(handle)->i;
    int b = cleo->ReadParam(handle)->i;
    cleo->GetPointerToScriptVar(handle)->i = a * b;
}

CLEO_Fn(INT_DIV)
{
    int a = cleo->ReadParam(handle)->i;
    int b = cleo->ReadParam(handle)->i;
    cleo->GetPointerToScriptVar(handle)->i = a / b;
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

CLEO_Fn(OPEN_FILE)
{
    char filename[MAX_STR_LEN], mode[10];
    CLEO_ReadStringEx(handle, filename, sizeof(filename));
    CLEO_ReadStringEx(handle, mode, sizeof(mode));

    FILE* file = DoFile(filename, mode);
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
    if(file)
    {
        char *str = new char[size];
        fread(str, size, 1, file);
        CLEO_WriteStringEx(handle, str);
        free(str);
    }
    else CLEO_WriteStringEx(handle, "");
}

CLEO_Fn(WRITE_TO_FILE)
{
    FILE* file = (FILE*)cleo->ReadParam(handle)->i;
    int size = cleo->ReadParam(handle)->i;
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
    bool condition = *(bool*)((uintptr_t)handle + ValueForGame(120, 121, 229, 525, 521));
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
    char buf[64]; CLEO_ReadStringEx(handle, buf, sizeof(buf));
    void* libHandle = dlopen(buf, RTLD_NOW);

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
CLEO_Fn(CLEO_CALL)
{
    int label = cleo->ReadParam(handle)->i;
    int nParams = (*(cleo->GetGameIdentifier() == GTASA ? GetPC(handle) : GetPC_CLEO(handle)) != 0) ? cleo->ReadParam(handle)->i : 0;
    ScmFunction* scmFunc = new ScmFunction(handle);

    char buf[MAX_STR_LEN];
    static int arguments[40];
    int maxParams = ValueForSA(40, 16);
    int* scope = (int*)((uintptr_t)handle + ValueForGame(48, 48, 60, 96, 520));
    if(*nGameIdent == GTASA && IsMissionScript(handle)) scope = (int*)LocalVariablesForCurrentMission;
    int* scopeEnd = scope + maxParams;
    int* storedLocals = scmFunc->savedTls;

    // collect arguments
    uint8_t max_i = nParams < maxParams ? nParams : maxParams;
    for (uint8_t i = 0; i < max_i; ++i)
    {
        int* val = &arguments[i];
        switch(*(cleo->GetGameIdentifier() == GTASA ? GetPC(handle) : GetPC_CLEO(handle)))
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
    while(*(cleo->GetGameIdentifier() == GTASA ? GetPC(handle) : GetPC_CLEO(handle)) != 0)
    {
        scmFunc->savedRets[++i] = &cleo->GetPointerToScriptVar(handle)->i;
    }

    // EXPERIMENTAL - COMMENTED
    if (nParams > maxParams)
    {
        (*nGameIdent == GTASA) ? CollectParameters_SA(handle, nParams - maxParams) : CollectParameters_VC(handle, &GetPC(handle), nParams - maxParams);
    }
    SkipUnusedParameters(handle);
    scmFunc->retnAddress = GetPC(handle);
    memcpy(scope, arguments, 4 * nParams);
    
    int* localVars = GetLocalVars(handle);
    for(int i = nParams; i < maxParams; ++i) localVars[i] = 0;

    ThreadJump(handle, label);
}

CLEO_Fn(CLEO_RETURN)
{
    ScmFunction *scmFunc = ScmFunction::Store[GetScmFunc(handle)];
    int nRetParams = 0;
    if(*(cleo->GetGameIdentifier() == GTASA ? GetPC(handle) : GetPC_CLEO(handle)))
    {
        nRetParams = cleo->ReadParam(handle)->i;
    }

    if(*nGameIdent == GTASA)
    {
        if(nRetParams) CollectParameters_SA(handle, nRetParams);
        scmFunc->Return(handle);
        //if(nRetParams) StoreParameters_SA(handle, nRetParams);
        // EXPERIMENTAL
        for(uint8_t i = 0; i < nRetParams; ++i)
        {
            *(scmFunc->savedRets[i]) = ScriptParams[i];
        }
    }
    else
    {
        if(nRetParams) CollectParameters_VC(handle, &GetPC(handle), nRetParams);
        scmFunc->Return(handle);
        if(nRetParams) StoreParameters_VC(handle, &GetPC(handle), nRetParams);
    }
    //SkipUnusedParameters(handle);
    delete scmFunc;
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
    cleo->GetPointerToScriptVar(handle)->i = *(uint8_t*)(*(int*)(vehiclePtr + 904) + 118);
}

CLEO_Fn(GET_CAR_CURRENT_GEAR)
{
    int ref = cleo->ReadParam(handle)->i;
    int vehiclePtr = GetVehicleFromRef(ref);
    cleo->GetPointerToScriptVar(handle)->i = *(uint8_t*)(vehiclePtr + 1216);
}

CLEO_Fn(IS_CAR_SIREN_ON)
{
    int ref = cleo->ReadParam(handle)->i;
    int vehiclePtr = GetVehicleFromRef(ref);
    UpdateCompareFlag(handle, *(uint8_t*)(vehiclePtr + 1073) >> 7);
}

CLEO_Fn(IS_CAR_ENGINE_ON)
{
    int ref = cleo->ReadParam(handle)->i;
    int vehiclePtr = GetVehicleFromRef(ref);
    UpdateCompareFlag(handle, (*(uint8_t *)(vehiclePtr + 1068) >> 4) & 1);
}

CLEO_Fn(CLEO_SET_CAR_ENGINE_ON)
{
    int vehiclePtr = GetVehicleFromRef(cleo->ReadParam(handle)->i);
    bool state = cleo->ReadParam(handle)->i != 0;
    *(int*)(vehiclePtr + 1068) = *(int*)(vehiclePtr + 1068) & 0xFFFFFFEF | (16 * (state & 1));
}

CLEO_Fn(PUSH_STRING_TO_VAR)
{
    char buf[128];
    CLEO_ReadStringEx(handle, buf, sizeof(buf));
    CLEO_WriteStringEx(handle, buf);
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

CLEO_Fn(ADD_TEXT_LABEL)
{
    char gxtLabel[8], text[MAX_STR_LEN];
    CLEO_ReadStringEx(handle, gxtLabel, sizeof(gxtLabel));
    CLEO_ReadStringEx(handle, text, sizeof(text));

    if(IsCLEORelatedGXTKey(gxtLabel)) return; // NUH-UH

    CLEO_STD_String key, keytext;
    CLEO_STD_PutStrToAlloced(&key, gxtLabel);
    CLEO_STD_PutStrToAlloced(&keytext, text);

    CLEO_STD_AddToGxtStorage(&key, &keytext);

    CLEO_STD_DeallocStorage(&key);
    CLEO_STD_DeallocStorage(&keytext);
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
    
    char path[256];
    snprintf(path, sizeof(path), "%s/%s", aml->GetAndroidDataPath(), filepath);

    DIR* dir = opendir(path);
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
    
    char path[256];
    snprintf(path, sizeof(path), "%s/%s", aml->GetAndroidDataPath(), filepath);

    int result = mkdir(path, 0777);
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
    
    char path[256];
    snprintf(path, sizeof(path), "%s/%s", aml->GetAndroidDataPath(), filepath);

    DIR* dir = opendir(path);
    if(dir)
    {
        struct dirent *entry;
        struct stat buf;
        char filecheckpath[256];
        while((entry = readdir(dir)) != NULL)
        {
            snprintf(filecheckpath, sizeof(filecheckpath), "%s/%s", path, entry->d_name);
            lstat(filecheckpath, &buf);

            if(!S_ISDIR(buf.st_mode))
            {
                CLEO_DirScan *scan = new CLEO_DirScan;
                strncpy(scan->path, path, sizeof(scan->path));
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

CLEO_Fn(CLEO_RETURN_IF_FALSE)
{
    if(!GetCond(handle)) CLEO_RETURN(handle, ip, opcode, name);
}

CLEO_Fn(CLEO_RETURN_IF_TRUE)
{
    if(!GetCond(handle)) CLEO_RETURN(handle, ip, opcode, name);
}

CLEO_Fn(SAVE_LOCAL_VARS)
{
    char savename[32], savepath[256];
    int maxParams = ValueForSA(40, 16);
    CLEO_ReadStringEx(handle, savename, sizeof(savename));
    snprintf(savepath, sizeof(savepath), "%s/sav/%s.lvar", cleo->GetCleoStorageDir(), savename);
    //logger->Info("SAVE_LOCAL_VARS: %s", savepath);

    FILE* savefile = fopen(savepath, "w+b");
    if(!savefile)
    {
        UpdateCompareFlag(handle, false);
        //logger->Error("SAVE_LOCAL_VARS");
        return;
    }

    int* scriptVars = GetLocalVars(handle);
    for(int i = 0; i < maxParams; ++i)
    {
        if(IsAlloced((void*)scriptVars[i]))
        {
            localVarsSave[i].value = 0;
            strcpy(localVarsSave[i].strvalue, (char*)scriptVars[i]);
        }
        else
        {
            localVarsSave[i].value = scriptVars[i];
            localVarsSave[i].strvalue[0] = 0;
        }
    }
    fwrite(localVarsSave, 1, sizeof(CLEOLocalVarSave) * maxParams, savefile);
    fclose(savefile);
    UpdateCompareFlag(handle, true);
}

CLEO_Fn(LOAD_LOCAL_VARS)
{
    char savename[32], savepath[256];
    int maxParams = ValueForSA(40, 16);
    CLEO_ReadStringEx(handle, savename, sizeof(savename));
    snprintf(savepath, sizeof(savepath), "%s/sav/%s.lvar", cleo->GetCleoStorageDir(), savename);
    FILE* savefile = fopen(savepath, "r+b");
    if(!savefile)
    {
        UpdateCompareFlag(handle, false);
        return;
    }

    int readBytes = sizeof(CLEOLocalVarSave) * maxParams;
    if(fread(localVarsSave, 1, readBytes, savefile) != readBytes)
    {
        fclose(savefile);
        UpdateCompareFlag(handle, false);
        return;
    }
    
    int* scriptVars = GetLocalVars(handle);
    for(int i = 0; i < maxParams; ++i)
    {
        if(IsAlloced((void*)scriptVars[i]))
        {
            FreeMem((void*)scriptVars[i]);
        }
        if(localVarsSave[i].strvalue[0])
        {
            int flag = localVarsSave[i].value;
            if(flag == 0)
            {
                int len = strlen(localVarsSave[i].strvalue) + 1;
                scriptVars[i] = (int)AllocMem(len);
                memcpy((void*)(scriptVars[i]), localVarsSave[i].strvalue, len);
            }
        }
        else
        {
            scriptVars[i] = localVarsSave[i].value;
        }
    }
    fclose(savefile);
    UpdateCompareFlag(handle, true);
}

CLEO_Fn(DELETE_LOCAL_VARS_SAVE)
{
    char savename[32], savepath[256];
    CLEO_ReadStringEx(handle, savename, sizeof(savename));
    snprintf(savepath, sizeof(savepath), "%s/sav/%s.lvar", cleo->GetCleoStorageDir(), savename);
    UpdateCompareFlag(handle, remove(savepath) == 0);
}

CLEO_Fn(SAVE_VARS)
{
    static int* varsPointers[MAX_SCRIPT_VARS_TO_SAVE];
    char savename[32], savepath[256];
    CLEO_ReadStringEx(handle, savename, sizeof(savename));
    snprintf(savepath, sizeof(savepath), "%s/sav/%s.var", cleo->GetCleoStorageDir(), savename);

    FILE* savefile = fopen(savepath, "w+b");
    if(!savefile)
    {
        SkipUnusedParameters(handle);
        UpdateCompareFlag(handle, false);
        return;
    }
    
    int variables = MAX_SCRIPT_VARS_TO_SAVE;
    for(int i = 0; i < MAX_SCRIPT_VARS_TO_SAVE; ++i)
    {
        if(*GetPC(handle))
        {
            int* scriptVar = (int*)cleo->GetPointerToScriptVar(handle);
            if(IsAlloced((void*)*scriptVar))
            {
                localVarsSave[i].value = 0;
                strcpy(localVarsSave[i].strvalue, (char*)*scriptVar);
            }
            else
            {
                localVarsSave[i].value = *scriptVar;
                localVarsSave[i].strvalue[0] = 0;
            }
        }
        else
        {
            variables = i;
            break;
        }
    }
    SkipUnusedParameters(handle);
    fwrite(localVarsSave, 1, sizeof(CLEOLocalVarSave) * variables, savefile);
    fclose(savefile);
    UpdateCompareFlag(handle, true);
}

CLEO_Fn(LOAD_VARS)
{
    static int* varsPointers[MAX_SCRIPT_VARS_TO_SAVE];
    char savename[32], savepath[256];
    CLEO_ReadStringEx(handle, savename, sizeof(savename));
    snprintf(savepath, sizeof(savepath), "%s/sav/%s.var", cleo->GetCleoStorageDir(), savename);
    FILE* savefile = fopen(savepath, "r+b");
    if(!savefile)
    {
        SkipUnusedParameters(handle);
        UpdateCompareFlag(handle, false);
        return;
    }
    
    int variables = MAX_SCRIPT_VARS_TO_SAVE;
    for(int i = 0; i < MAX_SCRIPT_VARS_TO_SAVE; ++i)
    {
        if(*GetPC(handle))
        {
            varsPointers[i] = (int*)cleo->GetPointerToScriptVar(handle);
        }
        else
        {
            variables = i;
            break;
        }
    }
    SkipUnusedParameters(handle);

    int readBytes = sizeof(CLEOLocalVarSave) * variables;
    if(fread(localVarsSave, 1, readBytes, savefile) != readBytes)
    {
        fclose(savefile);
        UpdateCompareFlag(handle, false);
        return;
    }
    
    for(int i = 0; i < variables; ++i)
    {
        if(IsAlloced((void*)*varsPointers[i]))
        {
            FreeMem((void*)*varsPointers[i]);
        }
        if(localVarsSave[i].strvalue[0])
        {
            int flag = localVarsSave[i].value;
            if(flag == 0)
            {
                int len = strlen(localVarsSave[i].strvalue) + 1;
                *varsPointers[i] = (int)AllocMem(len);
                memcpy((void*)(*varsPointers[i]), localVarsSave[i].strvalue, len);
            }
        }
        else
        {
            *varsPointers[i] = localVarsSave[i].value;
        }
    }

    fclose(savefile);
    UpdateCompareFlag(handle, true);
}

CLEO_Fn(DELETE_VARS_SAVE)
{
    char savename[32], savepath[256];
    CLEO_ReadStringEx(handle, savename, sizeof(savename));
    snprintf(savepath, sizeof(savepath), "%s/sav/%s.var", cleo->GetCleoStorageDir(), savename);
    UpdateCompareFlag(handle, remove(savepath) == 0);
}

void Init4Opcodes()
{
    SET_TO(ScriptSpace,         cleo->GetMainLibrarySymbol("_ZN11CTheScripts11ScriptSpaceE"));
    SET_TO(UpdateCompareFlag,   cleo->GetMainLibrarySymbol("_ZN14CRunningScript17UpdateCompareFlagEh"));
    SET_TO(pActiveScripts,      cleo->GetMainLibrarySymbol("_ZN11CTheScripts14pActiveScriptsE"));
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
    SET_TO(CLEO_STD_DeallocStorage, nCLEOAddr + 0x5F34 + 0x1);
    SET_TO(TheText,             cleo->GetMainLibrarySymbol("TheText"));
    SET_TO(TextGet,             cleo->GetMainLibrarySymbol("_ZN5CText3GetEPKc"));
    SET_TO(m_CheatString,       cleo->GetMainLibrarySymbol("_ZN6CCheat13m_CheatStringE"));
    SET_TO(keys,                cleo->GetMainLibrarySymbol("keys"));
    SET_TO(ms_modelInfoPtrs, *(uintptr_t*)((uintptr_t)cleo->GetMainLibraryLoadAddress() + (*nGameIdent == GTASA ? 0x6796D4 : 0x394D94)));
    if(*nGameIdent == GTASA)
    {
        SET_TO(gMobileMenu, cleo->GetMainLibrarySymbol("gMobileMenu"));
        SET_TO(ms_RadarTrace, *(uintptr_t*)((uintptr_t)cleo->GetMainLibraryLoadAddress() + 0x6773CC));
        SET_TO(FindGroundZForCoord, cleo->GetMainLibrarySymbol("_ZN6CWorld19FindGroundZForCoordEff"));
        SET_TO(LocalVariablesForCurrentMission, cleo->GetMainLibrarySymbol("_ZN11CTheScripts31LocalVariablesForCurrentMissionE"));
        SET_TO(CollectParameters_SA, cleo->GetMainLibrarySymbol("_ZN14CRunningScript17CollectParametersEs"));
        SET_TO(StoreParameters_SA, cleo->GetMainLibrarySymbol("_ZN14CRunningScript15StoreParametersEs"));
        SET_TO(SetHelpMessage_SA, cleo->GetMainLibrarySymbol("_ZN4CHud14SetHelpMessageEPKcPtbbbj"));
        SET_TO(FindPlayerPed, cleo->GetMainLibrarySymbol("_Z13FindPlayerPedi"));
        SET_TO(pedPool, cleo->GetMainLibrarySymbol("_ZN6CPools11ms_pPedPoolE"));
        SET_TO(vehiclePool, cleo->GetMainLibrarySymbol("_ZN6CPools15ms_pVehiclePoolE"));
        SET_TO(objectPool, cleo->GetMainLibrarySymbol("_ZN6CPools14ms_pObjectPoolE"));
        SET_TO(AddMessage_SA, cleo->GetMainLibrarySymbol("_ZN9CMessages10AddMessageEPKcPtjtb"));
        SET_TO(AddMessageJumpQ_SA, cleo->GetMainLibrarySymbol("_ZN9CMessages15AddMessageJumpQEPKcPtjtb"));
        SET_TO(SpawnCar, cleo->GetMainLibrarySymbol("_ZN6CCheat12VehicleCheatEi"));
    }
    else
    {
        SET_TO(CollectParameters_VC, cleo->GetMainLibrarySymbol("_ZN14CRunningScript17CollectParametersEPjs"));
        SET_TO(StoreParameters_VC, cleo->GetMainLibrarySymbol("_ZN14CRunningScript15StoreParametersEPjs"));
        SET_TO(SetHelpMessage_VC, cleo->GetMainLibrarySymbol("_ZN4CHud14SetHelpMessageEPtbbb"));
        SET_TO(AddMessage_VC, cleo->GetMainLibrarySymbol("_ZN9CMessages10AddMessageEPtjt"));
        SET_TO(AddMessageJumpQ_VC, cleo->GetMainLibrarySymbol("_ZN9CMessages15AddMessageJumpQEPtjt"));
        SET_TO(SpawnCar, cleo->GetMainLibrarySymbol("_Z12VehicleCheati"));
        SET_TO(curCheatCar_VC, cleo->GetMainLibrarySymbol("curCheatCar"));
    }

    CLEO_RegisterOpcode(0x0A8E, INT_ADD); // 0A8E=3,%3d% = %1d% + %2d% ; int
    CLEO_RegisterOpcode(0x0A8F, INT_SUB); // 0A8F=3,%3d% = %1d% - %2d% ; int
    CLEO_RegisterOpcode(0x0A90, INT_MUL); // 0A90=3,%3d% = %1d% * %2d% ; int
    CLEO_RegisterOpcode(0x0A91, INT_DIV); // 0A91=3,%3d% = %1d% / %2d% ; int
    CLEO_RegisterOpcode(0x0A96, GET_PED_POINTER); // 0A96=2,%2d% = actor %1d% struct
    CLEO_RegisterOpcode(0x0A97, GET_VEHICLE_POINTER); // 0A97=2,%2d% = car %1d% struct
    CLEO_RegisterOpcode(0x0A98, GET_OBJECT_POINTER); // 0A98=2,%2d% = object %1d% struct
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
        CLEO_RegisterOpcode(0x0AB7, GET_CAR_NUMBER_OF_GEARS); // 0AB7=2,get_vehicle %1d% number_of_gears_to %2d%
        CLEO_RegisterOpcode(0x0AB8, GET_CAR_CURRENT_GEAR); // 0AB8=2,get_vehicle %1d% current_gear_to %2d%
        // 0AB9, 0ABB-0ABC - AudioStreams
        // 0ABA - Threads (we dont have them!)
        CLEO_RegisterOpcode(0x0ABD, IS_CAR_SIREN_ON); // 0ABD=1,vehicle %1d% siren_on
        CLEO_RegisterOpcode(0x0ABE, IS_CAR_ENGINE_ON); // 0ABE=1,vehicle %1d% engine_on
        CLEO_RegisterOpcode(0x0ABF, CLEO_SET_CAR_ENGINE_ON); // 0ABF=2,set_vehicle %1d% engine_state_to %2d%
    }

    // 0AC0 - 0AC5 - AudioStreams
    CLEO_RegisterOpcode(0x0AC6, PUSH_STRING_TO_VAR); // 0DD0, so this one is CUSTOM: 0AC6=2,push_string %1d% var %2d%
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
    // popfloat? we have a different logic of FPU
    CLEO_RegisterOpcode(0x0AEA, GET_PED_REF); // 0AEA=2,%2d% = actor_struct %1d% handle
    CLEO_RegisterOpcode(0x0AEB, GET_VEHICLE_REF); // 0AEB=2,%2d% = car_struct %1d% handle
    CLEO_RegisterOpcode(0x0AEC, GET_OBJECT_REF); // 0AEC=2,%2d% = object_struct %1d% handle
    CLEO_RegisterOpcode(0x0AED, STRING_FLOAT_FORMAT); // 0AED=3,%3d% = float %1d% to_string_format %2d%
    CLEO_RegisterOpcode(0x0AEE, POW); // 0AEE=3,%3d% = %1d% exp %2d% //all floats
    CLEO_RegisterOpcode(0x0AEF, LOG); // 0AEF=3,%3d% = log %1d% base %2d% //all floats

    // Fully custom opcodes for Android
    CLEO_RegisterOpcode(0x0AF6, CLEO_RETURN_IF_FALSE); // 0AF6=-1,ret_if_false
    CLEO_RegisterOpcode(0x0AF7, CLEO_RETURN_IF_TRUE); // 0AF7=-1,ret_if_true
    CLEO_RegisterOpcode(0x0AF8, SAVE_LOCAL_VARS); // 0AF8=1,save_local_vars_named %1d% //IF and SET
    CLEO_RegisterOpcode(0x0AF9, LOAD_LOCAL_VARS); // 0AF9=1,load_local_vars_named %1d% //IF and SET
    CLEO_RegisterOpcode(0x0AFA, DELETE_LOCAL_VARS_SAVE); // 0AFA=1,delete_local_vars_save %1d% //IF and SET
    CLEO_RegisterOpcode(0x0AFB, SAVE_VARS); // 0AFB=-1,save_script_vars_named %1d% //IF and SET
    CLEO_RegisterOpcode(0x0AFC, LOAD_VARS); // 0AFC=-1,load_script_vars_named %1d% //IF and SET
    CLEO_RegisterOpcode(0x0AFD, DELETE_VARS_SAVE); // 0AFD=1,delete_script_vars_save %1d% //IF and SET
}
