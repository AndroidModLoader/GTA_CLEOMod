#include <stdint.h>

struct cleo_addon_ifs_t
{
    uint32_t    (*GetInterfaceVersion)(); // why not

                // Interface ver 1
    char*       (*ReadString)(void* handle, char* buf, size_t size);
    void        (*WriteString)(void* handle, const char* buf);
    uint32_t    (*GetStringMaxSize)(void* handle);
    char*       (*GetStringPointer)(void* handle);
    int         (*FormatString)(void* handle, char *str, size_t len, const char *format);
    void        (*AsciiToGXTChar)(const char* src, uint16_t* dst);
    const char* (*GXTCharToAscii)(const uint16_t* src, uint8_t start);
    int         (*ValueForGame)(int for3, int forvc, int forsa, int forlcs, int forvcs);
    void        (*ThreadJump)(void* handle, int offset);
    void        (*SkipUnusedParameters)(void *handle); // different from SkipParamListEnd
    uint8_t*    (*GetScriptPC)(void *handle);
    void        (*PushStack)(void *handle);
    void        (*PopStack)(void *handle);
    bool&       (*GetCond)(void *handle);
    bool&       (*GetNotFlag)(void *handle);
    uint16_t&   (*GetLogicalOp)(void *handle);
    void        (*Interrupt)(void *handle);
    void        (*Skip1Byte)(void* handle);
    void        (*Skip2Bytes)(void* handle);
    void        (*Skip4Bytes)(void* handle);
    void        (*SkipBytes)(void* handle, uint32_t bytes);
    uint8_t     (*Read1Byte)(void* handle);
    uint16_t    (*Read2Bytes)(void* handle);
    uint32_t    (*Read4Bytes)(void* handle);
    uint8_t     (*Read1Byte_NoSkip)(void* handle);
    uint16_t    (*Read2Bytes_NoSkip)(void* handle);
    uint32_t    (*Read4Bytes_NoSkip)(void* handle);
};