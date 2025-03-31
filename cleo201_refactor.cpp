#include <mod/amlmod.h>
#include <mod/logger.h>
#include <cleohelpers.h>
#include <cleo4scmfunc.h>

struct CLEOLocalVarSave
{
    int value;
    char strvalue[MAX_STR_LEN];
};
CLEOLocalVarSave localVarsSave[40];

extern uintptr_t nCLEOAddr;
extern int lastStorageItem;

CLEO_Fn(GET_LABEL_ADDR)
{
    uint32_t* pLabelAddr = &cleo->GetPointerToScriptVar(handle)->u;
    int labelOffset = cleo->ReadParam(handle)->i;

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
            *pLabelAddr = (uint32_t)((labelOffset < 0) ? (ValueForGame(0x20000, 0x370E8, 0, 0) - labelOffset) : labelOffset);
        }
    }
}
CLEO_Fn(GET_FUNC_ADDR_BY_NAME)
{
    char funcName[MAX_STR_LEN];
    uint32_t* pFuncAddr = &cleo->GetPointerToScriptVar(handle)->u;
    CLEO_ReadStringEx(handle, funcName, sizeof(funcName));
    *pFuncAddr = (uint32_t)cleo->GetMainLibrarySymbol(funcName);
}

CLEO_Fn(CLEO_RETURN);
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

CLEO_Fn(FIND_CUSTOM_SCRIPT_WITH_NAME)
{
    char name[MAX_STR_LEN];
    void** scriptRet = (void**)&cleo->GetPointerToScriptVar(handle)->u;
    CLEO_ReadStringEx(handle, name, sizeof(name));
    bool caseSensitive = cleo->ReadParam(handle)->i;
    bool partial = cleo->ReadParam(handle)->i;
    bool checkFilename = cleo->ReadParam(handle)->i;

    *scriptRet = NULL;
    
}

void Init201Opcodes()
{
    // Disable switch-case labels for default opcodes
    aml->Write16(nCLEOAddr + 0x75CC + 4 * 0x00, 0x0466); // 0DD0
    aml->Write16(nCLEOAddr + 0x75CC + 4 * 0x01, 0x0466); // 0DD1

    // Reimplement opcodes
    CLEO_RegisterOpcode(0x0DD0, GET_LABEL_ADDR); // 0DD0=2,%1d% = get_label_addr %2p% ; android
    CLEO_RegisterOpcode(0x0DD1, GET_FUNC_ADDR_BY_NAME); // 0DD1=2,%1d% = get_func_addr_by_cstr_name %2d% ; android

    // Fully custom opcodes for Android
    CLEO_RegisterOpcode(0x0AF6, CLEO_RETURN_IF_FALSE); // 0AF6=-1,ret_if_false
    CLEO_RegisterOpcode(0x0AF7, CLEO_RETURN_IF_TRUE); // 0AF7=-1,ret_if_true
    CLEO_RegisterOpcode(0x0AF8, SAVE_LOCAL_VARS); // 0AF8=1,save_local_vars_named %1d% //IF and SET
    CLEO_RegisterOpcode(0x0AF9, LOAD_LOCAL_VARS); // 0AF9=1,load_local_vars_named %1d% //IF and SET
    CLEO_RegisterOpcode(0x0AFA, DELETE_LOCAL_VARS_SAVE); // 0AFA=1,delete_local_vars_save %1d% //IF and SET
    CLEO_RegisterOpcode(0x0AFB, SAVE_VARS); // 0AFB=-1,save_script_vars_named %1d% //IF and SET
    CLEO_RegisterOpcode(0x0AFC, LOAD_VARS); // 0AFC=-1,load_script_vars_named %1d% //IF and SET
    CLEO_RegisterOpcode(0x0AFD, DELETE_VARS_SAVE); // 0AFD=1,delete_script_vars_save %1d% //IF and SET

    CLEO_RegisterOpcode(0x0AFE, FIND_CUSTOM_SCRIPT_WITH_NAME); // 0AFE=4,%1d% = find_custom_script_named %2d% case %3d% partial %4d% check_filename %5d% //IF and SET
}