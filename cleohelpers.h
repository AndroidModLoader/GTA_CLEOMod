#define MAX_STR_LEN 0xFF
#define MAX_SCRIPT_VARS_TO_SAVE 32
#define CHEAT_STRING_SIZE 30

#include <stdint.h>
#include "cleo.h"
extern eGameIdent* nGameIdent;
extern cleo_ifs_t* cleo;
extern uint8_t* ScriptSpace;

// https://github.com/gta-reversed/gta-reversed-modern/blob/1b37b015fbda7957ebbc36dbe8d5e4a90ebb6891/source/game_sa/Scripts/RunningScript.h#L17
constexpr auto SHORT_STRING_SIZE = 8;
constexpr auto LONG_STRING_SIZE = 16;
enum eScriptParameterType : int8_t
{
    SCRIPT_PARAM_END_OF_ARGUMENTS, //< Special type used for vararg stuff

    SCRIPT_PARAM_STATIC_INT_32BITS,
    SCRIPT_PARAM_GLOBAL_NUMBER_VARIABLE, //< Global int32 variable
    SCRIPT_PARAM_LOCAL_NUMBER_VARIABLE, //< Local int32 variable
    SCRIPT_PARAM_STATIC_INT_8BITS,
    SCRIPT_PARAM_STATIC_INT_16BITS,
    SCRIPT_PARAM_STATIC_FLOAT,

    // Types below are only available in GTA SA

    // Number arrays
    SCRIPT_PARAM_GLOBAL_NUMBER_ARRAY, //< Global array of numbers (always int32)
    SCRIPT_PARAM_LOCAL_NUMBER_ARRAY, //< Local array of numbers (always int32)

    SCRIPT_PARAM_STATIC_SHORT_STRING, //< Static 8 byte string

    SCRIPT_PARAM_GLOBAL_SHORT_STRING_VARIABLE, //< Local 8 byte string
    SCRIPT_PARAM_LOCAL_SHORT_STRING_VARIABLE, //< Local 8 byte string

    SCRIPT_PARAM_GLOBAL_SHORT_STRING_ARRAY, //< Global 8 byte string array
    SCRIPT_PARAM_LOCAL_SHORT_STRING_ARRAY,  //< Local 8 byte string array

    SCRIPT_PARAM_STATIC_PASCAL_STRING, //< Pascal string is a sequence of characters with optional size specification. (So says Google)
    SCRIPT_PARAM_STATIC_LONG_STRING,    //< 16 byte string

    SCRIPT_PARAM_GLOBAL_LONG_STRING_VARIABLE, //< Global 16 byte string
    SCRIPT_PARAM_LOCAL_LONG_STRING_VARIABLE, //< Local 16 byte string

    SCRIPT_PARAM_GLOBAL_LONG_STRING_ARRAY, //< Global array of 16 byte strings
    SCRIPT_PARAM_LOCAL_LONG_STRING_ARRAY, //< Local array of 16 byte strings
};

#include <set>
extern std::set<void*> gAllocationsMap;
extern std::set<FILE*> gFilesMap;

// Game Structs
struct GTAVector3D
{
    float x, y, z;
    float SqrMagnitude() { return x*x + y*y + z*z; }
    inline GTAVector3D operator-(const GTAVector3D& v) { return { x - v.x, y - v.y, z - v.z }; }
};
struct GTAMatrix
{
    GTAVector3D  right;
    unsigned int flags;
    GTAVector3D  up;
    unsigned int pad1;
    GTAVector3D  at;
    unsigned int pad2;
    GTAVector3D  pos;
    unsigned int pad3;

    void* ptr;
    bool bln;
};
struct GTAScript
{
    GTAScript* next;
    GTAScript* prev;
    char name[8];
    // Incomplete so dont try it ;)
};
struct tByteFlag
{
    uint8_t nId : 7;
    bool    bEmpty : 1;
};
struct GTAEntity
{
    inline uintptr_t AsInt() { return (uintptr_t)this; }
    inline int& IntAt(int off) { return *(int*)(AsInt() + off); }
    inline uint32_t& UIntAt(int off) { return *(uint32_t*)(AsInt() + off); }
    inline uint8_t& UInt8At(int off) { return *(uint8_t*)(AsInt() + off); }
    inline GTAVector3D& GetPos()
    {
        GTAMatrix* matrix = *(GTAMatrix**)(AsInt() + 20);
        return matrix ? matrix->pos : *(GTAVector3D*)(AsInt() + 4);
    }
};
struct GTAPedSA : GTAEntity
{
    char structure[1996];
    bool Player() { return UIntAt(1436) < 2; }
};
struct GTAVehicleSA : GTAEntity
{
    char structure[2604];
};
struct GTAObjectSA : GTAEntity
{
    char structure[420];
};
union GXTChar
{
    struct { char s1, s2; };
    uint16_t s;
};

// Custom Funcs
inline void* AllocMem(size_t size)
{
    void* mem = malloc(size);
    if(mem) gAllocationsMap.insert(mem);
    return mem;
}
inline bool IsAlloced(void* mem)
{
    return (gAllocationsMap.find(mem) != gAllocationsMap.end());
}
inline void FreeMem(void* mem)
{
    if (gAllocationsMap.find(mem) != gAllocationsMap.end())
    {
        free(mem);
        gAllocationsMap.erase(mem);
    }
}
inline FILE* DoFile(const char* filename, const char* mode)
{
    char path[256];
    snprintf(path, sizeof(path), "%s/%s", cleo->GetCleoStorageDir(), filename);
    FILE* file = fopen(path, mode);
    if(file) gFilesMap.insert(file);
    return file;
}
inline bool IsFileCreated(FILE* file)
{
    return (file && gFilesMap.find(file) != gFilesMap.end());
}
inline void FreeFile(FILE* file)
{
    if(file)
    {
        fflush(file);
        fclose(file);
        if (gFilesMap.find(file) != gFilesMap.end())
        {
            gFilesMap.erase(file);
        }
    }
}
inline void AsciiToGXTChar(const char* src, GXTChar* dst)
{
    int i = 0;
    while(src[i])
    {
        dst[i].s = src[i];
        ++i;
    }
    dst[i].s = 0;
}
inline void AsciiToGXTChar(const char* src, uint16_t* dst) { AsciiToGXTChar(src, (GXTChar*)dst); }
inline const char* GXTCharToAscii(const GXTChar* src, uint8_t start)
{
    static char buf[256] = { 0 };
    if(!src) return buf;

    const char* str = (char*)src;

    int i = 0;
    char symbol;
    while (i < (sizeof(buf) - 1) && (symbol = str[2 * i]))
    {
        if (symbol >= 0x80 && symbol <= 0x83)
            symbol += 0x40;
        else if (symbol >= 0x84 && symbol <= 0x8D)
            symbol += 0x42;
        else if (symbol >= 0x8E && symbol <= 0x91)
            symbol += 0x44;
        else if (symbol >= 0x92 && symbol <= 0x95)
            symbol += 0x47;
        else if (symbol >= 0x96 && symbol <= 0x9A)
            symbol += 0x49;
        else if (symbol >= 0x9B && symbol <= 0xA4)
            symbol += 0x4B;
        else if (symbol >= 0xA5 && symbol <= 0xA8)
            symbol += 0x4D;
        else if (symbol >= 0xA9 && symbol <= 0xCC)
            symbol += 0x50;
        else if (symbol == 0xCD)
            symbol = 0xD1;
        else if (symbol == 0xCE)
            symbol = 0xF1;
        else if (symbol == 0xCF)
            symbol = 0xBF;
        else if (symbol >= 0xD0)
            symbol = 0x23; // '#'

        buf[i] = symbol;

        ++i; // uint16_t
    }
    buf[i] = 0;

    return buf;
}
inline const char* GXTCharToAscii(const uint16_t* src, uint8_t start) { return GXTCharToAscii((const GXTChar*)src, start); }
inline int ValueForGame(int for3, int forvc, int forsa, int forlcs = 0, int forvcs = 0)
{
    switch(*nGameIdent)
    {
        case GTA3:   return for3;
        case GTAVC:  return forvc;
        case GTASA:  return forsa;
        case GTALCS: return forlcs;
        case GTAVCS: return forvcs;
    }
    return 0;
}
inline int ValueForSA(int forsa, int forothers)
{
    return *nGameIdent == GTASA ? forsa : forothers;
}
inline int GetPCOffset()
{
    switch(*nGameIdent)
    {
        case GTASA: return 20;
        case GTALCS: return 24;

        default: return 16;
    }
}
inline uint8_t*& GetPC(void* handle)
{
    return *(uint8_t**)((uintptr_t)handle + GetPCOffset());
}
inline uint8_t* GetPC_CLEO(void* handle) // weird-ass trash from CLEO for VC *facepalm*
{
    return (uint8_t*)cleo->GetRealCodePointer(*(uint32_t*)((uintptr_t)handle + GetPCOffset()));
}
inline uint8_t*& GetBasePC(void* handle)
{
    return *(uint8_t**)((uintptr_t)handle + GetPCOffset() - 4);
}
inline uint8_t** GetStack(void* handle)
{
    return (uint8_t**)((uintptr_t)handle + ValueForGame(20, 20, 24, 28, 20));
}
inline uint16_t& GetStackDepth(void* handle)
{
    return *(uint16_t*)((uintptr_t)handle + ValueForGame(44, 44, 56, 92, 516));
}
inline bool& GetCond(void* handle)
{
    return *(bool*)((uintptr_t)handle + ValueForGame(121, 121, 229));
}
inline bool& GetNotFlag(void* handle)
{
    return *(bool*)((uintptr_t)handle + ValueForGame(130, 130, 242));
}
inline uint16_t& GetLogicalOp(void* handle)
{
    return *(uint16_t*)((uintptr_t)handle + ValueForGame(128, 128, 240));
}
inline bool& IsMissionScript(void* handle)
{
    return *(bool*)((uintptr_t)handle + ValueForGame(133, 133, 252));
}
inline void PushStack(void* handle)
{
    uint8_t** stack = GetStack(handle);
    uint8_t*& bytePtr = GetPC(handle);
    uint16_t& stackDepth = GetStackDepth(handle);
    stack[stackDepth++] = bytePtr;
}
inline void PopStack(void* handle)
{
    GetPC(handle) = GetStack(handle)[--GetStackDepth(handle)];
}
inline int* GetLocalVars(void* handle)
{
    return (int*)((uintptr_t)handle + ValueForGame(48, 48, 60, 96, 520));
}
inline void ThreadJump(void* handle, int offset)
{
    uint8_t* newPCPointer = (uint8_t*)cleo->GetRealLabelPointer(handle, offset);
    if(newPCPointer) // CLEO script
    {
        GetPC(handle) = newPCPointer;
    }
    else // Not CLEO script (main.scm ???)
    {
        uint8_t*& bytePtr = GetPC(handle);
        int baseOffset = ValueForGame(0, 0, 16, 20, 0);
        if(baseOffset)
        {
            uint8_t* basePtr = GetBasePC(handle);
            bytePtr = (uint8_t*)((offset < 0) ? (basePtr - offset) : (ScriptSpace + offset));
        }
        else
        {
            bytePtr = (uint8_t*)((offset < 0) ? (ValueForGame(0x20000, 0x370E8, 0, 0) - offset) : offset);
        }
    }
}
inline void Skip1Byte(void* handle)
{
    GetPC(handle) += 1;
}
inline void Skip2Bytes(void* handle)
{
    GetPC(handle) += 2;
}
inline void Skip4Bytes(void* handle)
{
    GetPC(handle) += 4;
}
inline void SkipBytes(void* handle, uint32_t bytes)
{
    GetPC(handle) += bytes;
}
inline uint8_t* GetRealPC(void* handle)
{
    return (cleo->GetGameIdentifier() == GTASA ? GetPC(handle) : GetPC_CLEO(handle));
}
inline uint8_t Read1Byte_NoSkip(void* handle)
{
    return *GetRealPC(handle);
}
inline uint16_t Read2Bytes_NoSkip(void* handle)
{
    return *(uint16_t*)GetRealPC(handle);
}
inline uint32_t Read4Bytes_NoSkip(void* handle)
{
    return *(uint32_t*)GetRealPC(handle);
}
inline uint8_t Read1Byte(void* handle)
{
    uint8_t thatOneByte = *GetRealPC(handle);
    Skip1Byte(handle);
    return thatOneByte;
}
inline uint16_t Read2Bytes(void* handle)
{
    uint16_t theseBytes = *(uint16_t*)GetRealPC(handle);
    Skip2Bytes(handle);
    return theseBytes;
}
inline uint32_t Read4Bytes(void* handle)
{
    uint32_t theseBytes = *(uint16_t*)GetRealPC(handle);
    Skip4Bytes(handle);
    return theseBytes;
}
inline char* CLEO_ReadStringEx(void* handle, char* buf, size_t size)
{
    uint8_t type = Read1Byte_NoSkip(handle);

    static char newBuf[MAX_STR_LEN];
    if(!buf || size < 1) buf = (char*)newBuf;

    switch(type)
    {
        default:
        //case SCRIPT_PARAM_STATIC_INT_8BITS:
        //case SCRIPT_PARAM_STATIC_INT_16BITS:
            // They cant hold any info in 'em
            return NULL;

        case SCRIPT_PARAM_STATIC_INT_32BITS:
        case SCRIPT_PARAM_GLOBAL_NUMBER_VARIABLE:
        case SCRIPT_PARAM_LOCAL_NUMBER_VARIABLE:
        case SCRIPT_PARAM_STATIC_FLOAT:
        case SCRIPT_PARAM_GLOBAL_NUMBER_ARRAY:
        case SCRIPT_PARAM_LOCAL_NUMBER_ARRAY:
        {
            char *str = (char*)cleo->ReadParam(handle)->i;
            if(!str) return NULL;
            strncpy(buf, str, size - 1);
            buf[size - 1] = 0;
            return buf;
        }

        case SCRIPT_PARAM_STATIC_SHORT_STRING:
            Skip1Byte(handle); // "type" byte
            if(size < 1) size = SHORT_STRING_SIZE;
            for(int i = 0; i < SHORT_STRING_SIZE; ++i)
            {
                if(i < size) buf[i] = (char)Read1Byte(handle);
                else Skip1Byte(handle);
                buf[size - 1] = 0;
            }
            return buf;
            //return cleo->ReadString8byte(handle, buf, size) ? buf : NULL;

        case SCRIPT_PARAM_GLOBAL_SHORT_STRING_VARIABLE:
        case SCRIPT_PARAM_LOCAL_SHORT_STRING_VARIABLE:
        case SCRIPT_PARAM_GLOBAL_SHORT_STRING_ARRAY:
        case SCRIPT_PARAM_LOCAL_SHORT_STRING_ARRAY:
        {
            size = (size > SHORT_STRING_SIZE) ? SHORT_STRING_SIZE : size;
            strncpy(buf, (char*)cleo->GetPointerToScriptVar(handle), size);
            buf[size-1] = 0;
            return buf;
        }

        case SCRIPT_PARAM_STATIC_PASCAL_STRING: // not even a LONG STRING
            return cleo->ReadStringLong(handle, buf, size) ? buf : NULL;

        case SCRIPT_PARAM_STATIC_LONG_STRING:
            Skip1Byte(handle);
            size = (size > LONG_STRING_SIZE) ? LONG_STRING_SIZE : size;
            strncpy(buf, (char*)GetRealPC(handle), size);
            buf[size-1] = 0;
            SkipBytes(handle, LONG_STRING_SIZE);
            return buf;

        case SCRIPT_PARAM_GLOBAL_LONG_STRING_VARIABLE:
        case SCRIPT_PARAM_LOCAL_LONG_STRING_VARIABLE:
        case SCRIPT_PARAM_GLOBAL_LONG_STRING_ARRAY:
        case SCRIPT_PARAM_LOCAL_LONG_STRING_ARRAY:
        {
            size = (size > LONG_STRING_SIZE) ? LONG_STRING_SIZE : size;
            strncpy(buf, (char*)cleo->GetPointerToScriptVar(handle), size);
            buf[size-1] = 0;
            return buf;
        }
    }
    return buf;
}
inline void CLEO_WriteStringEx(void* handle, const char* buf)
{
    uint8_t byte = Read1Byte_NoSkip(handle);
    char* dst;
    switch(byte)
    {
        default:
            dst = (char*)cleo->ReadParam(handle)->i;
            strcpy(dst, buf);
            break;

        case 0x0A:
        case 0x0B:
        case 0x0C:
        case 0x0D:
        {
            dst = (char*)cleo->GetPointerToScriptVar(handle);
            strncpy(dst, buf, 7); dst[7] = 0;
            break;
        }

        case 0x10:
        case 0x11:
        case 0x12:
        case 0x13:
        {
            dst = (char*)cleo->GetPointerToScriptVar(handle);
            strncpy(dst, buf, 15); dst[15] = 0;
            break;
        }
    }
}
inline char* CLEO_GetStringPtr(void* handle)
{
    uint8_t byte = Read1Byte_NoSkip(handle);
    if(byte > 8)
    {
        return (char*)cleo->GetPointerToScriptVar(handle);
    }
    else
    {
        return (char*)cleo->ReadParam(handle)->i;
    }
}
inline uint32_t CLEO_GetStringPtrMaxSize(void* handle)
{
    uint8_t byte = Read1Byte_NoSkip(handle);
    switch(byte)
    {
        default:
            return MAX_STR_LEN;

        case 0x0A:
        case 0x0B:
        case 0x0C:
        case 0x0D:
            return 8;

        case 0x10:
        case 0x11:
        case 0x12:
        case 0x13:
            return 16;
    }
}
// https://github.com/cleolibrary/CLEO4/blob/efe00ef49945a85012cc2938c27ff82cccea5866/source/CCustomOpcodeSystem.cpp#L462
inline int CLEO_FormatString(void* handle, char *str, size_t len, const char *format)
{
    unsigned int written = 0;
    const char *iter = format;
    char bufa[MAX_STR_LEN], fmtbufa[64], *fmta, readbuf[MAX_STR_LEN];
    while (*iter)
    {
        while (*iter && *iter != '%')
        {
            if (written++ >= len) return -1;
            *str++ = *iter++;
        }
        if (*iter == '%')
        {
            if (iter[1] == '%')
            {
                if (written++ >= len) return -1;
                *str++ = '%'; /* "%%"->'%' */
                iter += 2;
                continue;
            }

            //get flags and width specifier
            fmta = fmtbufa;
            *fmta++ = *iter++;
            while (*iter == '0' ||
                   *iter == '+' ||
                   *iter == '-' ||
                   *iter == ' ' ||
                   *iter == '*' ||
                   *iter == '#')
            {
                if (*iter == '*')
                {
                    char *buffiter = bufa;
                    //get width
                    sprintf(buffiter, "%d", cleo->ReadParam(handle)->i);
                    while (*buffiter) *fmta++ = *buffiter++;
                }
                else
                {
                    *fmta++ = *iter;
                }
                iter++;
            }

            //get immediate width value
            while (isdigit(*iter)) *fmta++ = *iter++;

            //get precision
            if (*iter == '.')
            {
                *fmta++ = *iter++;
                if (*iter == '*')
                {
                    char *buffiter = bufa;
                    sprintf(buffiter, "%d", cleo->ReadParam(handle)->i);
                    while (*buffiter) *fmta++ = *buffiter++;
                }
                else
                {
                    while (isdigit(*iter)) *fmta++ = *iter++;
                }
            }

            //get size
            if (*iter == 'h' || *iter == 'l')
            {
                *fmta++ = *iter++;
            }
            switch (*iter)
            {
                case 's':
                {
                    static const char none[] = "(null)";
                    const char *astr = CLEO_ReadStringEx(handle, readbuf, sizeof(readbuf));
                    const char *striter = astr ? astr : none;
                    *fmta++ = *iter++;
                    *fmta++ = 0;
                    snprintf(bufa, sizeof(bufa), fmtbufa, striter); striter = bufa;
                    while (*striter)
                    {
                        if (written++ >= len) return -1;
                        *str++ = *striter++;
                    }
                    break;
                }
                case 'c':
                {
                    if (written++ >= len) return -1;
                    *fmta++ = *iter++;
                    *fmta++ = 0;
                    snprintf(bufa, sizeof(bufa), fmtbufa, (char)cleo->ReadParam(handle)->i);
                    const char *striter = bufa;
                    while (*striter)
                    {
                        if (written++ >= len) return -1;
                        *str++ = *striter++;
                    }
                    break;
                }
                default:
                {
                    /* For non wc types, use system sprintf and append to wide char output */
                    /* FIXME: for unrecognised types, should ignore % when printing */
                    char *bufaiter = bufa;
                    if (*iter == 'p' || *iter == 'P')
                    {
                        sprintf(bufaiter, "%08X", cleo->ReadParam(handle)->i);
                    }
                    else
                    {
                        *fmta++ = *iter;
                        *fmta = '\0';
                        if (*iter == 'a' || *iter == 'A' ||
                            *iter == 'e' || *iter == 'E' ||
                            *iter == 'f' || *iter == 'F' ||
                            *iter == 'g' || *iter == 'G')
                        {
                            sprintf(bufaiter, fmtbufa, cleo->ReadParam(handle)->f);
                        }
                        else
                        {
                            sprintf(bufaiter, fmtbufa, (void*)(cleo->ReadParam(handle)->i));
                        }
                    }
                    while (*bufaiter)
                    {
                        if (written++ >= len) return -1;
                        *str++ = *bufaiter++;
                    }
                    iter++;
                    break;
                }
            }
        }
    }
    if (written >= len) return -1;
    *str++ = 0;
    
    return (int)written;
}
inline bool IsCLEORelatedGXTKey(char* gxtLabel)
{
    if(gxtLabel[0] == 'C' && gxtLabel[1] == 'L') // Most likely a CLEO
    {
        if((gxtLabel[2] == 'M' && gxtLabel[3] == 'N' && gxtLabel[4] == 'U') ||
           (gxtLabel[2] == 'D' && gxtLabel[3] == 'S' && gxtLabel[4] == 'C'))
            return true; // nuh-uh
    }
    else if(gxtLabel[0] == 'C' && gxtLabel[1] == 'S' && gxtLabel[2] == 'I' && gxtLabel[3] == '_') return true; // nuh-uh
    else if(gxtLabel[0] == 'S' && gxtLabel[1] == 'P' && gxtLabel[2] == 'L' &&
            gxtLabel[3] == 'A' && gxtLabel[4] == 'S' && gxtLabel[5] == 'H') return true; // nuh-uh

    return false; // uh-nuh
}
inline uint16_t GetScmFunc(void* handle)
{
    return *(uint16_t*)((uintptr_t)handle + ValueForGame(0, 0x2E, 0x3A, 0, 0));
}
inline void SetScmFunc(void* handle, uint16_t idx)
{
    *(uint16_t*)((uintptr_t)handle + ValueForGame(0, 0x2E, 0x3A, 0, 0)) = idx;
}
inline void SkipUnusedParameters(void *thread)
{
    while(*(cleo->GetGameIdentifier() == GTASA ? GetPC(thread) : GetPC_CLEO(thread)))
        cleo->ReadParam(thread);
    GetPC(thread) += 1;
}