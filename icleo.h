#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

enum eGameIdent
{
	GTA3,
	GTAVC,
	GTASA,
	GTALCS,
	GTAVCS
};

enum eGameVerInternal
{
	VER_NONE,
	VER_GTA3_1_4,
	VER_GTAVC_1_03,
	VER_GTASA_1_00,
	VER_GTASA_1_01,
	VER_GTASA_1_02,
	VER_GTASA_1_03,
	VER_GTASA_1_05,
	VER_GTASA_1_06,
	VER_GTASA_1_05_GER,
	VER_GTASA_1_07,
	VER_GTA3_1_6,
	VER_GTAVC_1_06,
	VER_GTASA_1_08,
	VER_GTALCS_2_2,
	VER_GTA3_1_8_OR_HIGHER,
	VER_GTAVC_1_09_OR_HIGHER,
	VER_GTASA_2_00_OR_HIGHER,
	VER_GTALCS_2_4_OR_HIGHER,
	VER_GTALCS_PSP_1_05_OR_HIGHER,
	VER_GTAVCS_PSP_1_02_OR_HIGHER
};

class ICLEO
{
public:
    virtual uint32_t GetInterfaceVersion() = 0;
	virtual eGameIdent GetGameIdentifier() = 0;
	virtual eGameVerInternal GetGameVersionInternal() = 0;
	virtual const char *GetPackageName() = 0;
	virtual const char *GetPackageVersionStr() = 0;
	virtual uint32_t GetPackageVersionCode() = 0;
	virtual const char *GetCleoStorageDir() = 0;			 
	//virtual const char *GetCleoPluginLoadDir() = 0; // Removed ¯\_(ツ)_/¯
	virtual void PrintToCleoLog(const char *str) = 0;
	virtual const char *GetMainLibraryFileName() = 0;	
	virtual void *GetMainLibraryLoadAddress() = 0;
	typedef struct { uint32_t rva, size; } section_t;
	virtual uint32_t GetMainLibraryExecutableSections(section_t *sections, uint32_t size) = 0;
	virtual void *FindExecutablePattern(const char *pattern, uint32_t index) = 0;
	virtual void *GetMainLibrarySymbol(const char *name) = 0;
	virtual void MemWriteArr(void *addr, uint8_t *arr, uint32_t size, bool protect) = 0;
	virtual void ReplaceThumbCall(void *addr, void *func_to) = 0;
	virtual void HookThumbFunc(void *func, uint32_t startSize, void *func_to, void **func_orig) = 0;
	virtual void ReplaceArmCall(void *addr, void *func_to) = 0;
	virtual void HookArmFunc(void *func, uint32_t startSize, void *func_to, void **func_orig) = 0;
	// ip is a pointer inside scriptHandle structure, the structure is different in all games
	typedef void opcode_handler_t(void *scriptHandle, uint32_t *ip, uint16_t opcode, const char *name);
	virtual bool RegisterOpcode(uint16_t opcode, opcode_handler_t handler) = 0;
	virtual bool RegisterOpcodeFunction(const char *name, opcode_handler_t handler) = 0;
	typedef struct { union { int32_t i; uint32_t u; float f; }; } data_t;
	virtual data_t *ReadParam(void *scriptHandle) = 0; // returned pointer has the data until next param read call (points into game's ScriptParams)
	virtual data_t *GetPointerToScriptVar(void *scriptHandle) = 0; // returned pointer is valid all the time (points into game's script handle or into global seg)
	virtual bool ReadString8byte(void *scriptHandle, char *buf, uint32_t size) = 0;
	virtual bool ReadStringLong(void *scriptHandle, char *buf, uint32_t size) = 0; // supported only in III/VC/SA (so far)
	virtual bool IsParamListEnd(uint32_t ip) = 0;
	virtual void SkipParamListEnd(uint32_t *ip) = 0;
	virtual void *GetRealCodePointer(uint32_t ip) = 0;	// script ip to ptr, script ip has to be gained from handle->ip as *ip (handler param)
	virtual uint32_t GetIpUsingRealCodePointer(void *realPtr) = 0;	// ptr to script ip
	virtual void *GetRealLabelPointer(void *scriptHandle, uint32_t offset) = 0;	// offset has to be represented in the raw way i.e. opcode label param encoding
};