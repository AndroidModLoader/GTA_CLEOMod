#include <mod/amlmod.h>
#include <mod/logger.h>

// CLEO
#include "cleo.h"
extern cleo_ifs_t* cleo;
#define CLEO_RegisterOpcode(x, h) cleo->RegisterOpcode(x, h); cleo->RegisterOpcodeFunction(#h, h)
#define CLEO_Fn(h) void h (void *handle, uint32_t *ip, uint16_t opcode, const char *name)
int ValueForGame(int for3, int forvc, int forsa, int forlcs, int forvcs = 0);
uint8_t* ScriptSpace;

int (*GetPedFromRef)(int);
CLEO_Fn(GET_PED_POINTER)
{
    int ref = cleo->ReadParam(handle)->i;
    cleo->GetPointerToScriptVar(handle)->i = GetPedFromRef(ref);
}

int (*GetVehicleFromRef)(int);
CLEO_Fn(GET_VEHICLE_POINTER)
{
    int ref = cleo->ReadParam(handle)->i;
    cleo->GetPointerToScriptVar(handle)->i = GetVehicleFromRef(ref);
}

int (*GetObjectFromRef)(int);
CLEO_Fn(GET_OBJECT_POINTER)
{
    int ref = cleo->ReadParam(handle)->i;
    cleo->GetPointerToScriptVar(handle)->i = GetObjectFromRef(ref);
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
        uint8_t** stack = *(uint8_t***)((uintptr_t)handle + ValueForGame(20, 20, 24, 28, 20));
        uint8_t*& bytePtr = *(uint8_t**)((uintptr_t)handle + ValueForGame(16, 16, 20, 24, 16));
        uint16_t& stackDepth = *(uint16_t*)((uintptr_t)handle + ValueForGame(44, 44, 56, 92, 516));

        stack[stackDepth++] = bytePtr;
        int baseOffset = ValueForGame(0, 0, 16, 20, 0);
        if(!baseOffset)
        {
            bytePtr = (uint8_t*)((offset < 0) ? (ValueForGame(0x20000, 0x370E8, 0, 0) - offset) : offset);
        }
        else
        {
            uint8_t* basePtr = *(uint8_t**)((uintptr_t)handle + baseOffset);
            bytePtr = (uint8_t*)((offset < 0) ? (basePtr - offset) : (ScriptSpace + offset));
        }
    }
}

void Init4Opcodes()
{
    SET_TO(ScriptSpace, cleo->GetMainLibrarySymbol("_ZN11CTheScripts11ScriptSpaceE"));

    SET_TO(GetPedFromRef, cleo->GetMainLibrarySymbol("_ZN6CPools6GetPedEi"));
    CLEO_RegisterOpcode(0x0A96, GET_PED_POINTER); // 0A96=2,%2d% = actor %1d% struct

    SET_TO(GetVehicleFromRef, cleo->GetMainLibrarySymbol("_ZN6CPools10GetVehicleEi"));
    CLEO_RegisterOpcode(0x0A97, GET_VEHICLE_POINTER); // 0A97=2,%2d% = car %1d% struct

    SET_TO(GetObjectFromRef, cleo->GetMainLibrarySymbol("_ZN6CPools9GetObjectEi"));
    CLEO_RegisterOpcode(0x0A98, GET_OBJECT_POINTER); // 0A98=2,%2d% = object %1d% struct

    CLEO_RegisterOpcode(0x0A9F, GET_THIS_SCRIPT_STRUCT); // 0A9F=1,%1d% = current_thread_pointer

    CLEO_RegisterOpcode(0x0AA0, GOSUB_IF_FALSE); // 0AA0=1,gosub_if_false %1p%
}