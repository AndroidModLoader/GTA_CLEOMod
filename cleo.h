#pragma once

#ifndef ANDROID
	#define ANDROID
#endif // ANDROID

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

struct cleo_ifs_t
{
	// interface v1

	uint32_t (*GetInterfaceVersion)();	// currently = 1

	eGameIdent (*GetGameIdentifier)();
	eGameVerInternal (*GetGameVersionInternal)();

	const char *(*GetPackageName)();			// PSP: DiscId
	const char *(*GetPackageVersionStr)();		// PSP: DiscVersion
	uint32_t (*GetPackageVersionCode)();		// PSP: DiscVersionCode

	const char *(*GetCleoStorageDir)();			 
	const char *(*GetCleoPluginLoadDir)();		// PSP: same as StorageDir

	void (*PrintToCleoLog)(const char *str);

	const char *(*GetMainLibraryFileName)();	
	void *(*GetMainLibraryLoadAddress)();

	typedef struct { uint32_t rva, size; } section_t;

	uint32_t (*GetMainLibraryExecutableSections)(section_t *sections, uint32_t size);
	void *(*FindExecutablePattern)(const char *pattern, uint32_t index);

#ifdef ANDROID
	void *(*GetMainLibrarySymbol)(const char *name);
#endif

	void (*MemWriteArr)(void *addr, uint8_t *arr, uint32_t size, bool protect);

#ifdef ANDROID
	void (*ReplaceThumbCall)(void *addr, void *func_to);
	void (*HookThumbFunc)(void *func, uint32_t startSize, void *func_to, void **func_orig);
	void (*ReplaceArmCall)(void *addr, void *func_to);
	void (*HookArmFunc)(void *func, uint32_t startSize, void *func_to, void **func_orig);
#else
	void (*ReplaceMipsCall)(void *addr, void *func_to);
	void (*HookMipsFunc)(void *func, uint32_t startSize, void *func_to, void **func_orig);
#endif

	// ip is a pointer inside scriptHandle structure, the structure is different in all games
	typedef void (*opcode_handler_t)(void *scriptHandle, uint32_t *ip, uint16_t opcode, const char *name);

	bool (*RegisterOpcode)(uint16_t opcode, opcode_handler_t handler);
	bool (*RegisterOpcodeFunction)(const char *name, opcode_handler_t handler);

	typedef struct { union { int32_t i; uint32_t u; float f; }; } data_t;

	data_t *(*ReadParam)(void *scriptHandle); // returned pointer has the data until next param read call (points into game's ScriptParams)
	data_t *(*GetPointerToScriptVar)(void *scriptHandle); // returned pointer is valid all the time (points into game's script handle or into global seg)
	bool (*ReadString8byte)(void *scriptHandle, char *buf, uint32_t size);
	bool (*ReadStringLong)(void *scriptHandle, char *buf, uint32_t size);	// supported only in III/VC/SA (so far)

	bool (*IsParamListEnd)(uint32_t ip);
	void (*SkipParamListEnd)(uint32_t *ip);

	void *(*GetRealCodePointer)(uint32_t ip);	// script ip to ptr, script ip has to be gained from handle->ip as *ip (handler param)
	uint32_t (*GetIpUsingRealCodePointer)(void *realPtr);	// ptr to script ip
	void *(*GetRealLabelPointer)(void *scriptHandle, uint32_t offset);	// offset has to be represented in the raw way i.e. opcode label param encoding
};