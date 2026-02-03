#include <mod/amlmod.h>
#include <mod/logger.h>
#include <cleohelpers.h>
#include <cleo4scmfunc.h>
#include <sys/stat.h>
#include <sys/system_properties.h>

struct CLEOLocalVarSave
{
    int value;
    char strvalue[MAX_STR_LEN];
};
CLEOLocalVarSave localVarsSave[40];

struct GameFingerPoint
{
    int x, y, state, clickIndex;
    float clickTime[2];
    int updCount;
};
GameFingerPoint *Points;
int (*OS_PointerGetNumber)();
int (*OS_ScreenGetWidth)();
int (*OS_ScreenGetHeight)();
void (*CorrectAspect)(float*,float*,float*,float*);
double *base_time, *last_current_time;

extern char g_szSavesPath[256];

extern GTASprite2D *ScriptSprites, *ScriptSpritesOrg;
extern void (*SetSprite2dTexture)(GTASprite2D&, const char*);
extern int (*GetVehicleFromRef)(int);

CLEO_Fn(GET_LABEL_ADDR)
{
    uint32_t* pLabelAddr = &cleo->GetPointerToScriptVar(handle)->u;
    int labelOffset = cleo->ReadParam(handle)->i;
    *pLabelAddr = GetLabelAddr(handle, labelOffset);
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
    else SkipUnusedParameters(handle);
}
CLEO_Fn(CLEO_RETURN_IF_TRUE)
{
    if(!GetCond(handle)) CLEO_RETURN(handle, ip, opcode, name);
    else SkipUnusedParameters(handle);
}
CLEO_Fn(SAVE_LOCAL_VARS)
{
    char savename[32], savepath[256];
    int maxParams = ValueForSA(40, 16);
    CLEO_ReadStringEx(handle, savename, sizeof(savename));
    snprintf(savepath, sizeof(savepath), "%s/%s.lvar", g_szSavesPath, savename);
    mkdir(g_szSavesPath, 0777);
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
    snprintf(savepath, sizeof(savepath), "%s/%s.lvar", g_szSavesPath, savename);
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
    snprintf(savepath, sizeof(savepath), "%s/%s.lvar", g_szSavesPath, savename);
    UpdateCompareFlag(handle, remove(savepath) == 0);
}
CLEO_Fn(SAVE_VARS)
{
    static int* varsPointers[MAX_SCRIPT_VARS_TO_SAVE];
    char savename[32], savepath[256];
    CLEO_ReadStringEx(handle, savename, sizeof(savename));
    snprintf(savepath, sizeof(savepath), "%s/%s.var", g_szSavesPath, savename);
    mkdir(g_szSavesPath, 0777);

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
    snprintf(savepath, sizeof(savepath), "%s/%s.var", g_szSavesPath, savename);
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
    snprintf(savepath, sizeof(savepath), "%s/%s.var", g_szSavesPath, savename);
    UpdateCompareFlag(handle, remove(savepath) == 0);
}

inline bool strcmp_partial(const char* source, const char* with)
{
    int len = strlen(with);
    int maxlen = strlen(source) - len;

    for(int i = 0; i < maxlen; ++i)
    {
        if(!strncmp(&source[i], with, len)) return true;
    }
    return false;
}
inline bool strcasecmp_partial(const char* source, const char* with)
{
    int len = strlen(with);
    int maxlen = strlen(source) - len;

    for(int i = 0; i < maxlen; ++i)
    {
        if(!strncasecmp(&source[i], with, len)) return true;
    }
    return false;
}
CLEO_Fn(FIND_CUSTOM_SCRIPT_WITH_NAME)
{
    char scrname[MAX_STR_LEN];
    void** scriptRet = (void**)&cleo->GetPointerToScriptVar(handle)->u;
    CLEO_ReadStringEx(handle, scrname, sizeof(scrname));
    bool caseSensitive = cleo->ReadParam(handle)->i;
    bool partial = cleo->ReadParam(handle)->i;
    bool checkFilename = cleo->ReadParam(handle)->i;

    *scriptRet = NULL;
    int size = GetScriptsStorageSize();
    for(int i = 0; i < size; ++i)
    {
        int storageItem = *(int*)(*pScriptsStorage + i * 4);
        if(storageItem && *(void**)(storageItem + 28))
        {
            const char* scrOrgName = GetScriptName(*(void**)(storageItem + 28));
            if(checkFilename) scrOrgName = *(const char**)(storageItem + 20);

            if(caseSensitive)
            {
                if(partial)
                {
                    if(strcasecmp_partial(scrOrgName, scrname))
                    {
                        *scriptRet = *(void**)(storageItem + 28);
                        break;
                    }
                }
                else
                {
                    if(!strcasecmp(scrOrgName, scrname))
                    {
                        *scriptRet = *(void**)(storageItem + 28);
                        break;
                    }
                }
            }
            else
            {
                if(partial)
                {
                    if(strcmp_partial(scrOrgName, scrname))
                    {
                        *scriptRet = *(void**)(storageItem + 28);
                        break;
                    }
                }
                else
                {
                    if(!strcmp(scrOrgName, scrname))
                    {
                        *scriptRet = *(void**)(storageItem + 28);
                        break;
                    }
                }
            }
        }
    }
    UpdateCompareFlag(handle, *scriptRet != NULL);
}
CLEO_Fn(SET_COMPARE_FLAG)
{
    UpdateCompareFlag(handle, (cleo->ReadParam(handle)->i != 0) );
}
static char m_szDeviceLanguageCode[16] { 0 };
static char m_szDeviceCountryCode[16] { 0 };
static bool m_bAlreadyDidReadProps = false;
inline void InitLanguageProps()
{
    if(!m_bAlreadyDidReadProps)
    {
        strcpy(m_szDeviceLanguageCode, "en");
        strcpy(m_szDeviceCountryCode, "US");

        char localeProp[PROP_VALUE_MAX];
        int len = __system_property_get("persist.sys.locale", localeProp);
        const char* pLocale = ( (len > 0) ? &localeProp[0] : getenv("ANDROID_LOCALE") );

        if(pLocale)
        {
            len = strlen(pLocale);
            for(int i = len-1; i >= 0; --i)
            {
                if(pLocale[i] == '-')
                {
                    strncpy(m_szDeviceLanguageCode, pLocale, ( (i > sizeof(m_szDeviceLanguageCode)-1) ? sizeof(m_szDeviceLanguageCode)-1 : i ));
                    m_szDeviceLanguageCode[sizeof(m_szDeviceLanguageCode)-1] = 0;
                    strncpy(m_szDeviceCountryCode, &pLocale[i+1], sizeof(m_szDeviceCountryCode)-1);
                    m_szDeviceCountryCode[sizeof(m_szDeviceCountryCode)-1] = 0;
                    break;
                }
            }
        }
        m_bAlreadyDidReadProps = true;
    }
}
CLEO_Fn(GET_LANGUAGE_CODE)
{
    InitLanguageProps();
    CLEO_WriteStringEx(handle, m_szDeviceLanguageCode);
}
CLEO_Fn(GET_COUNTRY_CODE)
{
    InitLanguageProps();
    CLEO_WriteStringEx(handle, m_szDeviceCountryCode);
}
CLEO_Fn(ATOF)
{
    char buf[MAX_STR_LEN];
    CLEO_ReadStringEx(handle, buf, sizeof(buf));
    cleo->GetPointerToScriptVar(handle)->f = atof(buf);
}
CLEO_Fn(ATOI)
{
    char buf[MAX_STR_LEN];
    CLEO_ReadStringEx(handle, buf, sizeof(buf));
    cleo->GetPointerToScriptVar(handle)->i = atoi(buf);
}
CLEO_Fn(GET_SCREEN_HEIGHT)
{
    cleo->GetPointerToScriptVar(handle)->i = OS_ScreenGetWidth();
    cleo->GetPointerToScriptVar(handle)->i = OS_ScreenGetHeight();
}
CLEO_Fn(IS_ANY_FINGER_ONSCREEN)
{
    int size = OS_PointerGetNumber();
    for(int i = 0; i < size; ++i)
    {
        if(Points[i].state == 2)
        {
            return UpdateCompareFlag(handle, true);
        }
    }
    UpdateCompareFlag(handle, false);
}
CLEO_Fn(IS_FINGER_IN_AREA)
{
    float tX = cleo->ReadParam(handle)->f;
    float tY = cleo->ReadParam(handle)->f;
    float tR = cleo->ReadParam(handle)->f;
    tR *= tR; // radius SQR

    int size = OS_PointerGetNumber();
    float xMult = 100.0f / (float)OS_ScreenGetWidth();
    float yMult = 100.0f / (float)OS_ScreenGetHeight();
    for(int i = 0; i < size; ++i)
    {
        if(Points[i].state == 2)
        {
            float x = xMult * Points[i].x - tX;
            float y = yMult * Points[i].y - tY;
            if(x*x + y*y < tR)
            {
                return UpdateCompareFlag(handle, true);
            }
        }
    }
    UpdateCompareFlag(handle, false);
}
CLEO_Fn(IS_FINGER_IN_AREA_TIMED)
{
    float tX = cleo->ReadParam(handle)->f;
    float tY = cleo->ReadParam(handle)->f;
    float tR = cleo->ReadParam(handle)->f;
    double time = (double)cleo->ReadParam(handle)->i / 1000.0;
    tR *= tR; // radius SQR

    int size = OS_PointerGetNumber();
    float xMult = 100.0f / (float)OS_ScreenGetWidth();
    float yMult = 100.0f / (float)OS_ScreenGetHeight();
    for(int i = 0; i < size; ++i)
    {
        if(Points[i].state == 2)
        {
            float x = xMult * Points[i].x - tX;
            float y = yMult * Points[i].y - tY;
            if(x*x + y*y < tR)
            {
                int clcIdx = (Points[i].clickIndex == 0);
                if((Points[i].clickTime[clcIdx] + time + *base_time) < *last_current_time)
                {
                    return UpdateCompareFlag(handle, true);
                }
            }
        }
    }
    UpdateCompareFlag(handle, false);
}
CLEO_Fn(TOUCH_XY_TO_PERCENTAGE)
{
    float x = cleo->ReadParam(handle)->f;
    float y = cleo->ReadParam(handle)->f;
    cleo->GetPointerToScriptVar(handle)->f = 100.0f * x / (float)OS_ScreenGetWidth();
    cleo->GetPointerToScriptVar(handle)->f = 100.0f * y / (float)OS_ScreenGetHeight();
}
CLEO_Fn(SPRITE_XY_TO_PERCENTAGE)
{
    float trashVar;
    float x = cleo->ReadParam(handle)->f;
    float y = cleo->ReadParam(handle)->f;
    float sx = OS_ScreenGetWidth(), sy = OS_ScreenGetHeight();

    CorrectAspect(&x, &y, &trashVar, &trashVar);
    cleo->GetPointerToScriptVar(handle)->f = 100.0f * x / sx;
    cleo->GetPointerToScriptVar(handle)->f = 100.0f * y / sy;
}
CLEO_Fn(GET_MAX_POINTS_NUM)
{
    cleo->GetPointerToScriptVar(handle)->i = OS_PointerGetNumber();
}
CLEO_Fn(GET_POINT_XY)
{
    int num = cleo->ReadParam(handle)->i;
    if(num < 0 || num >= OS_PointerGetNumber() || Points[num].state != 2)
    {
        cleo->GetPointerToScriptVar(handle)->f = 0.0f;
        cleo->GetPointerToScriptVar(handle)->f = 0.0f;
    }
    else
    {
        cleo->GetPointerToScriptVar(handle)->f = Points[num].x;
        cleo->GetPointerToScriptVar(handle)->f = Points[num].y;
    }
}
CLEO_Fn(IS_FINGER_NUM_IN_AREA)
{
    int i = cleo->ReadParam(handle)->i;
    float tX = cleo->ReadParam(handle)->f;
    float tY = cleo->ReadParam(handle)->f;
    float tR = cleo->ReadParam(handle)->f;
    tR *= tR; // radius SQR

    int size = OS_PointerGetNumber();
    if(i >= 0 && i < OS_PointerGetNumber())
    {
        if(Points[i].state == 2)
        {
            float xMult = 100.0f / (float)OS_ScreenGetWidth();
            float yMult = 100.0f / (float)OS_ScreenGetHeight();
            float x = xMult * Points[i].x - tX;
            float y = yMult * Points[i].y - tY;
            if(x*x + y*y < tR)
            {
                return UpdateCompareFlag(handle, true);
            }
        }
    }
    UpdateCompareFlag(handle, false);
}
CLEO_Fn(IS_FINGER_NUM_IN_AREA_TIMED)
{
    int i = cleo->ReadParam(handle)->i;
    float tX = cleo->ReadParam(handle)->f;
    float tY = cleo->ReadParam(handle)->f;
    float tR = cleo->ReadParam(handle)->f;
    double time = (double)cleo->ReadParam(handle)->i / 1000.0;
    tR *= tR; // radius SQR

    if(i >= 0 && i < OS_PointerGetNumber())
    {
        if(Points[i].state == 2)
        {
            float xMult = 100.0f / (float)OS_ScreenGetWidth();
            float yMult = 100.0f / (float)OS_ScreenGetHeight();
            float x = xMult * Points[i].x - tX;
            float y = yMult * Points[i].y - tY;
            if(x*x + y*y < tR)
            {
                int clcIdx = (Points[i].clickIndex == 0);
                if((Points[i].clickTime[clcIdx] + time + *base_time) < *last_current_time)
                {
                    return UpdateCompareFlag(handle, true);
                }
            }
        }
    }
    UpdateCompareFlag(handle, false);
}
CLEO_Fn(HAS_VEHICLE_RADIO)
{
    int vehiclePtr = GetVehicleFromRef(cleo->ReadParam(handle)->i);
    if(vehiclePtr)
    {
        if(*nGameIdent == GTASA)
        {
            return UpdateCompareFlag(handle, *(char*)(vehiclePtr + 0x1D7) == 0 );
        }
        else if(*nGameIdent == GTAVC)
        {
            return UpdateCompareFlag(handle, *(uint8_t*)(vehiclePtr + 0x240) < 10 );
        }
    }
    UpdateCompareFlag(handle, false);
}
CLEO_Fn(HAS_VEHICLE_STRUCT_RADIO)
{
    int vehiclePtr = cleo->ReadParam(handle)->i;
    if(vehiclePtr)
    {
        if(*nGameIdent == GTASA)
        {
            return UpdateCompareFlag(handle, *(char*)(vehiclePtr + 0x1D7) == 0 );
        }
        else if(*nGameIdent == GTAVC)
        {
            return UpdateCompareFlag(handle, *(uint8_t*)(vehiclePtr + 0x240) < 10 );
        }
    }
    UpdateCompareFlag(handle, false);
}
CLEO_Fn(GET_RAM_MEGABYTES)
{
    static int totalMem = 0;
    if(totalMem)
    {
        cleo->GetPointerToScriptVar(handle)->i = totalMem;
        return;
    }

    FILE* f = fopen("/proc/meminfo", "r");
    if(f)
    {
        char line[64];
        while(fgets(line, sizeof(line), f))
        {
            if(!strncmp(line, "MemTotal:", 9))
            {
                totalMem = (int)(atol(line + 9) / 1024L);
                break;
            }
        }
        fclose(f);
    }
    cleo->GetPointerToScriptVar(handle)->i = totalMem;
}
CLEO_Fn(GET_FREE_RAM_MEGABYTES)
{
    /*static*/ int totalMem = 0;
    if(totalMem)
    {
        cleo->GetPointerToScriptVar(handle)->i = totalMem;
        return;
    }

    FILE* f = fopen("/proc/meminfo", "r");
    if(f)
    {
        char line[64];
        while(fgets(line, sizeof(line), f))
        {
            if(!strncmp(line, "MemAvailable:", 13))
            {
                totalMem = (int)(atol(line + 13) / 1024L);
                break;
            }
        }
        fclose(f);
    }
    cleo->GetPointerToScriptVar(handle)->i = totalMem;
}
CLEO_Fn(IMPORT_SCM_ADDR)
{
    uintptr_t ret = 0;
    char strImportName[256];
    CLEO_ReadStringEx(handle, strImportName, sizeof(strImportName));

    auto it = g_listExports.find(strImportName);
    if(it != g_listExports.end()) ret = it->second;

    cleo->GetPointerToScriptVar(handle)->i = ret;
    UpdateCompareFlag(handle, ret != 0);
}
CLEO_Fn(EXPORT_SCM_LABEL)
{
    int labelOffset = cleo->ReadParam(handle)->i;
    uint32_t label = GetLabelAddr(handle, labelOffset);

    char strExportName[256];
    CLEO_ReadStringEx(handle, strExportName, sizeof(strExportName));

    g_listExports.insert(std::pair<std::string, uintptr_t>(strExportName, label));
}
CLEO_Fn(EXPORT_SCM_VALUE)
{
    uintptr_t value = (uintptr_t)cleo->ReadParam(handle)->i;

    char strExportName[256];
    CLEO_ReadStringEx(handle, strExportName, sizeof(strExportName));

    g_listExports.insert(std::pair<std::string, uintptr_t>(strExportName, value));
}
CLEO_Fn(EXPORT_SCM_VAR)
{
    uintptr_t variableOffset = (uintptr_t)cleo->GetPointerToScriptVar(handle);

    char strExportName[256];
    CLEO_ReadStringEx(handle, strExportName, sizeof(strExportName));

    g_listExports.insert(std::pair<std::string, uintptr_t>(strExportName, variableOffset));
}

// Default scripting funcs

CLEO_Fn(DRAW_SPRITE)
{
    if(IsScriptCustom(handle))
    {
        int textureId = cleo->ReadParam(handle)->i;
        float pX = cleo->ReadParam(handle)->f;
        float pY = cleo->ReadParam(handle)->f;
        float sX = 0.5f * cleo->ReadParam(handle)->f;
        float sY = 0.5f * cleo->ReadParam(handle)->f;
        int r = cleo->ReadParam(handle)->i;
        int g = cleo->ReadParam(handle)->i;
        int b = cleo->ReadParam(handle)->i;
        int a = cleo->ReadParam(handle)->i;

        CorrectAspect(&pX, &pY, &sX, &sY);
        CustomScriptRect& rt = GetAddonInfo(handle).scriptRects[ ( GetAddonInfo(handle).scriptRectsThisFrame )++ ];

        rt.type = 4;
        rt.spriteIndex = textureId;
        rt.rotation = 0.0f;
        rt.color = GTARGBA(r, g, b, a).intColor;

        rt.rectMin.x = pX - sX;
        rt.rectMin.y = pY - sY;
        rt.rectMax.x = pX + sX;
        rt.rectMax.y = pY + sY;
    }
    else
    {
        CallDefaultOpcode(handle, opcode);
    }
}
CLEO_Fn(DRAW_RECT)
{
    if(IsScriptCustom(handle))
    {
        float pX = cleo->ReadParam(handle)->f;
        float pY = cleo->ReadParam(handle)->f;
        float sX = 0.5f * cleo->ReadParam(handle)->f;
        float sY = 0.5f * cleo->ReadParam(handle)->f;
        int r = cleo->ReadParam(handle)->i;
        int g = cleo->ReadParam(handle)->i;
        int b = cleo->ReadParam(handle)->i;
        int a = cleo->ReadParam(handle)->i;

        CorrectAspect(&pX, &pY, &sX, &sY);
        CustomScriptRect& rt = GetAddonInfo(handle).scriptRects[ ( GetAddonInfo(handle).scriptRectsThisFrame )++ ];

        rt.type = 4;
        rt.spriteIndex = 0;
        rt.rotation = 0.0f;
        rt.color = GTARGBA(r, g, b, a).intColor;

        rt.rectMin.x = pX - sX;
        rt.rectMin.y = pY - sY;
        rt.rectMax.x = pX + sX;
        rt.rectMax.y = pY + sY;
    }
    else
    {
        CallDefaultOpcode(handle, opcode);
    }
}
extern int (*FindTxdSlot)(const char*);
extern void (*PushCurrentTxd)();
extern void (*SetCurrentTxd)(int, const char*);
extern void (*PopCurrentTxd)();
CLEO_Fn(LOAD_SPRITE)
{
    char str[MAX_STR_LEN], strLower[MAX_STR_LEN];
    int id = cleo->ReadParam(handle)->i - 1;
    CLEO_ReadStringEx(handle, str, sizeof(str));

    int len = strlen(str);
    for(int i = 0; i < len; ++i)
    {
        strLower[i] = std::tolower(str[i]);
    }

    int slot = FindTxdSlot("script");
    PushCurrentTxd();
    SetCurrentTxd(slot, NULL);
    if(GetAddonInfo(handle).isCustom)
    {
        void* bak = ScriptSprites[id].texture;
        ScriptSprites[id].texture = NULL;
        
        SetSprite2dTexture(ScriptSprites[id], strLower);
        if(!ScriptSprites[id].texture)
        {
            SetSprite2dTexture(ScriptSprites[id], str);
        }
        SetCLEOSpriteTexture(handle, id + 1, ScriptSprites[id].texture);

        ScriptSprites[id].texture = bak;
    }
    else
    {
        SetSprite2dTexture(ScriptSprites[id], strLower);
        if(!ScriptSprites[id].texture)
        {
            SetSprite2dTexture(ScriptSprites[id], str);
        }
    }
    PopCurrentTxd();
}

CLEO_Fn(SET_SPRITES_DRAW_BEFORE_FADE)
{
    if(GetAddonInfo(handle).isCustom)
    {
        CustomScriptRect& rt = GetAddonInfo(handle).scriptRects[GetAddonInfo(handle).scriptRectsThisFrame];
        rt.beforeFade = cleo->ReadParam(handle)->i != 0;
    }
    else
    {
        CallDefaultOpcode(handle, opcode);
    }
}

CLEO_Fn(DRAW_SPRITE_WITH_ROTATION)
{
    if(GetAddonInfo(handle).isCustom)
    {
        int textureId = cleo->ReadParam(handle)->i;
        float pX = cleo->ReadParam(handle)->f;
        float pY = cleo->ReadParam(handle)->f;
        float sX = 0.5f * cleo->ReadParam(handle)->f;
        float sY = 0.5f * cleo->ReadParam(handle)->f;
        float angle = (M_PI * cleo->ReadParam(handle)->f) / 180.0f;
        int r = cleo->ReadParam(handle)->i;
        int g = cleo->ReadParam(handle)->i;
        int b = cleo->ReadParam(handle)->i;
        int a = cleo->ReadParam(handle)->i;

        CorrectAspect(&pX, &pY, &sX, &sY);
        CustomScriptRect& rt = GetAddonInfo(handle).scriptRects[ ( GetAddonInfo(handle).scriptRectsThisFrame )++ ];

        rt.type = 5;
        rt.spriteIndex = textureId;
        rt.rotation = angle;
        rt.color = GTARGBA(r, g, b, a).intColor;

        rt.rectMin.x = pX - sX;
        rt.rectMin.y = pY - sY;
        rt.rectMax.x = pX + sX;
        rt.rectMax.y = pY + sY;
    }
    else
    {
        CallDefaultOpcode(handle, opcode);
    }
}

extern void (*DrawSprite2d)(GTASprite2D&, float*, uint32_t*);
extern void (*DrawRectSprite2d)(GTASprite2D&, float*, uint32_t*);
extern void (*DrawRotatedSprite2d)(GTASprite2D&,float,float,float,float,float,float,float,float,uint32_t*);
void DrawSingleRect(void* handle, CustomScriptRect& rt)
{
    float fakeRect[4];
    GTASprite2D tmpSprite;

    switch(rt.type)
    {
        case 1: // WINDOW_HEADER_AND_TEXT
        {

            break;
        }
        
        case 2: // WINDOW_HEADER_NO_TEXT
        {

            break;
        }

        case 3: // WINDOW_SOLID_COLOUR
        {
            tmpSprite.texture = GetCLEOSpriteTexture(handle, rt.spriteIndex);
            fakeRect[0] = rt.rectMin.x;
            fakeRect[1] = rt.rectMax.y;
            fakeRect[2] = rt.rectMax.x;
            fakeRect[3] = rt.rectMin.y;
            DrawRectSprite2d(tmpSprite, fakeRect, &rt.color);
            break;
        }

        case 4: // WINDOW_SPRITE_NO_ROTATION
        {
            tmpSprite.texture = GetCLEOSpriteTexture(handle, rt.spriteIndex);
            fakeRect[0] = rt.rectMin.x;
            fakeRect[1] = rt.rectMax.y;
            fakeRect[2] = rt.rectMax.x;
            fakeRect[3] = rt.rectMin.y;
            DrawSprite2d(tmpSprite, fakeRect, &rt.color);
            break;
        }
        
        case 5: // WINDOW_SPRITE_WITH_ROTATION
        {
            float centerX = 0.5f * (rt.rectMax.x + rt.rectMin.x); float diffX = centerX - rt.rectMin.x;
            float centerY = 0.5f * (rt.rectMax.y + rt.rectMin.y); float diffY = centerY - rt.rectMin.y;
            float csin, ccos; sincosf(rt.rotation, &csin, &ccos);

            tmpSprite.texture = GetCLEOSpriteTexture(handle, rt.spriteIndex);
            // https://github.com/gta-reversed/gta-reversed/blob/1887338b0d4c12facccaf8505b6946a346c965a5/source/game_sa/Scripts/TheScripts.cpp#L1777
            DrawRotatedSprite2d(tmpSprite, 
                -ccos * diffX + csin * diffY + centerX,
                -csin * diffX - ccos * diffY + centerY,
                 csin * diffY + ccos * diffX + centerX,
                 csin * diffX - ccos * diffY + centerY,
                -ccos * diffX - csin * diffY + centerX,
                 ccos * diffY - csin * diffX + centerY,
                 ccos * diffX - csin * diffY + centerX,
                 csin * diffX + ccos * diffY + centerY,
                &rt.color
            );
            break;
        }

        default: return;
    }
}

void Init201Opcodes()
{
    SET_TO(Points, *(void**)(nGameAddr + ValueForGame(0, 0x394C78, 0x679E94)));
    SET_TO(OS_PointerGetNumber, cleo->GetMainLibrarySymbol("_Z19OS_PointerGetNumberv"));
    SET_TO(OS_ScreenGetWidth, cleo->GetMainLibrarySymbol("_Z17OS_ScreenGetWidthv"));
    SET_TO(OS_ScreenGetHeight, cleo->GetMainLibrarySymbol("_Z18OS_ScreenGetHeightv"));
    SET_TO(CorrectAspect, cleo->GetMainLibrarySymbol("_Z13CorrectAspectRfS_S_S_"));
    SET_TO(base_time, cleo->GetMainLibrarySymbol("base_time"));
    SET_TO(last_current_time, nGameAddr + ValueForGame(0, 0x74BD68, 0x6D70D8));

    // Disable switch-case labels for default opcodes
    aml->Write16(nCLEOAddr + 0x75CC + 4 * 0x00, 0x0466); // 0DD0
    //aml->Write16(nCLEOAddr + 0x75CC + 4 * 0x01, 0x0466); // 0DD1

    // Reimplement opcodes
    CLEO_RegisterOpcode(0x0DD0, GET_LABEL_ADDR); // 0DD0=2,%1d% = get_label_addr %2p% ; android
    //CLEO_RegisterOpcode(0x0DD1, GET_FUNC_ADDR_BY_NAME); // 0DD1=2,%1d% = get_func_addr_by_cstr_name %2d% ; android

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
    CLEO_RegisterOpcode(0x0AFF, SET_COMPARE_FLAG); // 0AFF=1,set_compare_flag %1d%
    CLEO_RegisterOpcode(0x0CB0, GET_LANGUAGE_CODE); // 0CB0=1,%1d% = get_language_code
    CLEO_RegisterOpcode(0x0CB1, GET_COUNTRY_CODE); // 0CB1=1,%1d% = get_country_code
    CLEO_RegisterOpcode(0x0CB2, ATOF); // 0CB2=2,%2d% = atof %1d%
    CLEO_RegisterOpcode(0x0CB3, ATOI); // 0CB3=2,%2d% = atoi %1d%
    CLEO_RegisterOpcode(0x0CB4, GET_SCREEN_HEIGHT); // 0CB4=2,get_screen_height x %1d% y %2d%
    CLEO_RegisterOpcode(0x0CB5, IS_ANY_FINGER_ONSCREEN); // 0CB5=0,is_any_finger_onscreen // IF and SET
    CLEO_RegisterOpcode(0x0CB6, IS_FINGER_IN_AREA); // 0CB6=3,is_finger_in_area %1d% %2d% radius %3d% // IF and SET
    CLEO_RegisterOpcode(0x0CB7, IS_FINGER_IN_AREA_TIMED); // 0CB7=4,is_finger_in_area_timed %1d% %2d% radius %3d% time_ms %4d% // IF and SET
    CLEO_RegisterOpcode(0x0CB8, TOUCH_XY_TO_PERCENTAGE); // 0CB8=4,%3d% %4d% = touchxy_to_perc %1d% %2d%
    CLEO_RegisterOpcode(0x0CB9, SPRITE_XY_TO_PERCENTAGE); // 0CB9=4,%3d% %4d% = spritexy_to_perc %1d% %2d%
    CLEO_RegisterOpcode(0x0CBA, GET_MAX_POINTS_NUM); // 0CBA=1,%1d% = get_max_points_num
    CLEO_RegisterOpcode(0x0CBB, GET_POINT_XY); // 0CBB=3,%2d% %3d% = get_pointer_xy %1d%
    CLEO_RegisterOpcode(0x0CBC, IS_FINGER_NUM_IN_AREA); // 0CBC=4,is_finger %1d% in_area %2d% %3d% radius %4d% // IF and SET
    CLEO_RegisterOpcode(0x0CBD, IS_FINGER_NUM_IN_AREA_TIMED); // 0CBD=5,is_finger %1d% in_area_timed %2d% %3d% radius %4d% time_ms %5d% // IF and SET
    CLEO_RegisterOpcode(0x0CD0, HAS_VEHICLE_RADIO); // 0CD0=1,has_vehicle_radio %1d% // IF and SET
    CLEO_RegisterOpcode(0x0CD1, HAS_VEHICLE_STRUCT_RADIO); // 0CD1=1,has_vehicle_struct_radio %1d% // IF and SET
    CLEO_RegisterOpcode(0x0CD2, GET_RAM_MEGABYTES); // 0CD2=1,%1d% = get_ram_megabytes
    CLEO_RegisterOpcode(0x0CD3, GET_FREE_RAM_MEGABYTES); // 0CD3=1,%1d% = get_free_ram_megabytes
    CLEO_RegisterOpcode(0x0CD4, IMPORT_SCM_ADDR); // 0CD4=2,%2d% = import_scm_addr %1d% // IF and SET
    CLEO_RegisterOpcode(0x0CD5, EXPORT_SCM_LABEL); // 0CD5=2,export_scm_label %1d% as %2d%
    CLEO_RegisterOpcode(0x0CD6, EXPORT_SCM_VALUE); // 0CD6=2,export_scm_value %1d% as %2d%
    CLEO_RegisterOpcode(0x0CD7, EXPORT_SCM_VAR); // 0CD7=2,export_scm_var %1d% as %2d%

    // Regular opcodes rewriting (for GTA:SA only)
#ifdef SCRIPTS_UNIQUE_SPRITE_IDS
    if(*nGameIdent == GTASA)
    {
        CLEO_RegisterOpcode(0x038D, DRAW_SPRITE); // 038D=9,draw_texture %1h% position %2d% %3d% size %4d% %5d% RGBA %6d% %7d% %8d% %9d%
        CLEO_RegisterOpcode(0x038E, DRAW_RECT); // 038E=8,draw_box_position %1d% %2d% size %3d% %4d% RGBA %5h% %6h% %7h% %8d%
        CLEO_RegisterOpcode(0x038F, LOAD_SPRITE); // 038F=2,load_texture %2h% as %1d%
        CLEO_RegisterOpcode(0x03E3, SET_SPRITES_DRAW_BEFORE_FADE); // 03E3=1,set_texture_to_be_drawn_antialiased %1h%
        CLEO_RegisterOpcode(0x074B, DRAW_SPRITE_WITH_ROTATION); // 074B=10,draw_texture %1h% position %2d% %3d% scale %4d% %5d% angle %6d% color_RGBA %7d% %8d% %9d% %10d%
    }
#endif
}
