#include <mod/amlmod.h>
#include <mod/logger.h>
#include <cleohelpers.h>
#include <cleo4scmfunc.h>

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

void Init201Opcodes()
{
    // Disable switch-case labels for default opcodes
    aml->Write16(nCLEOAddr + 0x75CC + 4 * 0x00, 0x0466); // 0DD0
    aml->Write16(nCLEOAddr + 0x75CC + 4 * 0x01, 0x0466); // 0DD1

    // Reimplement opcodes
    CLEO_RegisterOpcode(0x0DD0, GET_LABEL_ADDR); // 0DD0=2,%1d% = get_label_addr %2p% ; android
    CLEO_RegisterOpcode(0x0DD1, GET_FUNC_ADDR_BY_NAME); // 0DD1=2,%1d% = get_func_addr_by_cstr_name %2d% ; android
}