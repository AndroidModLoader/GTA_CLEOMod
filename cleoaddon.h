#ifndef _CLEO_ADDON_H
#define _CLEO_ADDON_H

#include <stdint.h>
#include <string>
#include <list>
#include <map>

#include "cleo.h"

//#define SCRIPTS_UNIQUE_SPRITE_IDS

#define CLEO_RegisterOpcode(x, h) cleo->RegisterOpcode(x, h); cleo->RegisterOpcodeFunction(#h, h)
#define CLEO_Fn(h) void h (void *handle, uint32_t *ip, uint16_t opcode, const char *name)

struct GTAVector2D
{
    float x, y;
    inline float GetDistance2D(GTAVector2D* a)
    {
        GTAVector2D b = { x - a->x, y - a->y };
        return sqrt(b.x * b.x + b.y * b.y);
    }
};
struct CustomScriptRect
{
    uint8_t type;
    bool beforeFade;
    bool align;
    uint8_t zIndex;
    GTAVector2D rectMin;
    GTAVector2D rectMax;
    float rotation;
    uint32_t color;
    char title[10];
    char msg[10];
    int spriteIndex;
};
struct ScriptAddonInfo
{
    static const int allocSize = 0x400;
    static const int scriptStackSize = 0x200;

    ScriptAddonInfo() { Reset(NULL); }
    inline void Reset(char* stackPtr)
    {
        unused = 0xFFFF;

        workDir.clear();
        childThreads.clear();
        parentThread = NULL;

        scmFuncId = 0;
        isCustom = false;
        debugMode = false;
        enableThreadSaving = false;
        scriptTextures.clear();
        scriptRectsThisFrame = 0;

        scriptVarStackPtr = stackPtr;
    }

    inline void* GetScriptTexture(int id)
    {
        auto it = scriptTextures.find(id);
        if(it != scriptTextures.end())
        {
            return it->second;
        }
        return NULL;
    }

    inline bool HasParent()
    {
        return (parentThread != NULL);
    }

    inline void OnScriptProcess()
    {
        scriptRectsThisFrame = 0;
    }

    inline char* GetVarStack()
    {
        return scriptVarStackPtr;
    }

    template<typename T>
    inline void PushVarToStack(T var)
    {
        PushStack<T>();
        *(T*)scriptVarStackPtr = var;
    }

    template<typename T>
    inline void PopVarFromStack(T& var)
    {
        var = *(T*)scriptVarStackPtr;
        PopStack<T>();
    }

    template<typename T = int>
    inline void PushStack()
    {
        size_t Tsize = sizeof(T);
        if(Tsize & 3) Tsize -= (Tsize & 3 - 4);
        scriptVarStackPtr -= Tsize;
    }

    template<typename T = int>
    inline void PopStack()
    {
        size_t Tsize = sizeof(T);
        if(Tsize & 3) Tsize -= (Tsize & 3 - 4);
        scriptVarStackPtr += Tsize;
    }

    inline char* AllocateFromStack(const int bytes)
    {
        size_t Tsize = bytes;
        if(Tsize & 3) Tsize -= (Tsize & 3 - 4);
        scriptVarStackPtr -= Tsize;
        return scriptVarStackPtr;
    }

    inline void DeallocateFromStack(const int bytes)
    {
        size_t Tsize = bytes;
        if(Tsize & 3) Tsize -= (Tsize & 3 - 4);
        scriptVarStackPtr += Tsize;
    }

    // GetInterfaceVersion() == 1
    std::string workDir;
    std::list<void*> childThreads;
    void* parentThread;
    
    uint16_t unused;

    // GetInterfaceVersion() == 2
    uint16_t scmFuncId;
    bool isCustom;
    bool debugMode;
    bool enableThreadSaving;
    std::map<int, void*> scriptTextures;

    uint8_t scriptRectsThisFrame;
    CustomScriptRect scriptRects[64];

    // GetInterfaceVersion() == 3
    cleo_ifs_t::data_t privateVars[32];
    char* scriptVarStackPtr;

    // GetInterfaceVersion() == 4
    // To Be Added (c)
};


struct cleo_addon_ifs_t
{
    uint32_t        (*GetInterfaceVersion)(); // why not

                    // Interface ver 1
    char*           (*ReadString)(void* handle, char* buf, size_t size);
    void            (*WriteString)(void* handle, const char* buf);
    uint32_t        (*GetStringMaxSize)(void* handle);
    char*           (*GetStringPointer)(void* handle);
    int             (*FormatString)(void* handle, char *str, size_t len, const char *format);
    void            (*AsciiToGXTChar)(const char* src, uint16_t* dst);
    const char*     (*GXTCharToAscii)(const uint16_t* src, uint8_t start);
    int             (*ValueForGame)(int for3, int forvc, int forsa, int forlcs, int forvcs);
    void            (*ThreadJump)(void* handle, int offset);
    void            (*SkipUnusedParameters)(void *handle); // different from SkipParamListEnd
    uint8_t*        (*GetScriptPC)(void *handle); // this contains a real code IN ALL GAMES
    void            (*PushStack)(void *handle);
    void            (*PopStack)(void *handle);
    bool&           (*GetCond)(void *handle);
    bool&           (*GetNotFlag)(void *handle);
    uint16_t&       (*GetLogicalOp)(void *handle);
    void            (*Interrupt)(void *handle);
    void            (*Skip1Byte)(void* handle);
    void            (*Skip2Bytes)(void* handle);
    void            (*Skip4Bytes)(void* handle);
    void            (*SkipBytes)(void* handle, uint32_t bytes);
    uint8_t         (*Read1Byte)(void* handle);
    uint16_t        (*Read2Bytes)(void* handle);
    uint32_t        (*Read4Bytes)(void* handle);
    uint8_t         (*Read1Byte_NoSkip)(void* handle);
    uint16_t        (*Read2Bytes_NoSkip)(void* handle);
    uint32_t        (*Read4Bytes_NoSkip)(void* handle);
    int*            (*GetLocalVars)(void* handle);
    uint8_t*&       (*GetPC)(void *handle); // this contains a real code ONLY IN GTASA (maybe LCS too), used for thread jumping &etc
    void            (*SkipOpcodeParameters)(void* handle, int count);
    int             (*GetVarArgCount)(void* handle);
    ScriptAddonInfo&(*GetAddonInfo)(void* handle);
    void            (*UpdateCompareFlag)(void* handle, uint8_t flag);
    bool            (*IsOpcodeAlreadyExists)(uint16_t opcode);
    bool            (*IsValidScriptHandle)(void* handle);
    std::string     (*ResolvePath)(void* handle, const char* path, const char* customWorkDir);
    void            (*AddGXTLabel)(const char* gxtLabel, const char* text);

    // Interface ver 2
    bool&           (*GetActiveFlag)(void *handle);
    bool            (*IsInActiveScripts)(void *handle);
    bool            (*IsInPausedScripts)(void *handle);
    bool            (*IsInCLEOScripts)(void *handle);
    bool            (*IsParamString)(void *handle, bool checkIfPointer);
    const char*     (*GetScriptFilename)(void* handle);
    const char*     (*GetVarTypeName)(int varType);
    char*           (*GetStringPtr)(void* handle);
    uint32_t        (*GetStringPtrMaxSize)(void* handle);
    bool&           (*IsMissionScript)(void *handle);
    std::string     (*ReadStdString)(void* handle);
    void*           (*GetLastCustomScriptCreated)();
    uint32_t&       (*GetWakeTime)(void* handle);
    void*           (*GetScriptTextureByID)(void* handle, int id);
    void            (*SetScriptTextureByID)(void* handle, int id, void* texture);
    bool            (*IsScriptCustom)(void* handle);
    int8_t          (*CallDefaultOpcode)(void* handle, uint16_t opcode);

    // Interface ver 3
    void            (*SetPrivateVar)(void* handle, int idx, cleo_ifs_t::data_t value);
    cleo_ifs_t::data_t (*GetPrivateVar)(void* handle, int idx);
    uintptr_t       (*GetLabelAddr)(void* handle, int labelOffset);
    void            (*ExportAddressToSCM)(const char* exportName, void* address);
    void*           (*ImportAddressFromSCM)(const char* exportName);
    int             (*GetCLEOScriptsCount)();
    void*           (*GetCLEOScript)(int num);
    uint16_t        (*GetScriptID)(void* handle);

    // Interface ver 4
    // To Be Added (c)
};

#endif // _CLEO_ADDON_H