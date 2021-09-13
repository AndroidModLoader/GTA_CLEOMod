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

	//typedef struct { uint32_t rva, size; } section_t;

	uint32_t (*GetMainLibraryExecutableSections)(ICLEO::section_t *sections, uint32_t size);
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
	//typedef void (*opcode_handler_t)(void *scriptHandle, uint32_t *ip, uint16_t opcode, const char *name);

	bool (*RegisterOpcode)(uint16_t opcode, ICLEO::opcode_handler_t handler);
	bool (*RegisterOpcodeFunction)(const char *name, ICLEO::opcode_handler_t handler);

	//typedef struct { union { int32_t i; uint32_t u; float f; }; } data_t;

	ICLEO::data_t *(*ReadParam)(void *scriptHandle); // returned pointer has the data until next param read call (points into game's ScriptParams)
	ICLEO::data_t *(*GetPointerToScriptVar)(void *scriptHandle); // returned pointer is valid all the time (points into game's script handle or into global seg)
	bool (*ReadString8byte)(void *scriptHandle, char *buf, uint32_t size);
	bool (*ReadStringLong)(void *scriptHandle, char *buf, uint32_t size);	// supported only in III/VC/SA (so far)

	bool (*IsParamListEnd)(uint32_t ip);
	void (*SkipParamListEnd)(uint32_t *ip);

	void *(*GetRealCodePointer)(uint32_t ip);	// script ip to ptr, script ip has to be gained from handle->ip as *ip (handler param)
	uint32_t (*GetIpUsingRealCodePointer)(void *realPtr);	// ptr to script ip
	void *(*GetRealLabelPointer)(void *scriptHandle, uint32_t offset);	// offset has to be represented in the raw way i.e. opcode label param encoding
};

extern cleo_ifs_t* cleo;

class CLEO : public ICLEO
{
public:
    uint32_t GetInterfaceVersion() { return cleo->GetInterfaceVersion(); };
	eGameIdent GetGameIdentifier() { return cleo->GetGameIdentifier(); };
	eGameVerInternal GetGameVersionInternal() { return cleo->GetGameVersionInternal(); };
	const char *GetPackageName() { return cleo->GetPackageName(); };
	const char *GetPackageVersionStr() { return cleo->GetPackageVersionStr(); };
	uint32_t GetPackageVersionCode() { return cleo->GetPackageVersionCode(); };
	const char *GetCleoStorageDir() { return cleo->GetCleoStorageDir(); };
	//const char *GetCleoPluginLoadDir() { return cleo->GetCleoPluginLoadDir(); }; // Removed ¯\_(ツ)_/¯
	void PrintToCleoLog(const char *str) { return cleo->PrintToCleoLog(str); };
	const char *GetMainLibraryFileName() { return cleo->GetMainLibraryFileName(); };
	void *GetMainLibraryLoadAddress() { return cleo->GetMainLibraryLoadAddress(); };
	uint32_t GetMainLibraryExecutableSections(ICLEO::section_t *sections, uint32_t size) { return cleo->GetMainLibraryExecutableSections(sections, size); };
	void *FindExecutablePattern(const char *pattern, uint32_t index) { return cleo->FindExecutablePattern(pattern, index); };
	void *GetMainLibrarySymbol(const char *name) { return cleo->GetMainLibrarySymbol(name); };
	void MemWriteArr(void *addr, uint8_t *arr, uint32_t size, bool protect) { return cleo->MemWriteArr(addr, arr, size, protect); };
	void ReplaceThumbCall(void *addr, void *func_to) { return cleo->ReplaceThumbCall(addr, func_to); };
	void HookThumbFunc(void *func, uint32_t startSize, void *func_to, void **func_orig) { return cleo->HookThumbFunc(func, startSize, func_to, func_orig); };
	void ReplaceArmCall(void *addr, void *func_to) { return cleo->ReplaceArmCall(addr, func_to); };
	void HookArmFunc(void *func, uint32_t startSize, void *func_to, void **func_orig) { return cleo->HookArmFunc(func, startSize, func_to, func_orig); };
	bool RegisterOpcode(uint16_t opcode, ICLEO::opcode_handler_t handler) { return cleo->RegisterOpcode(opcode, handler); };
	bool RegisterOpcodeFunction(const char *name, ICLEO::opcode_handler_t handler) { return cleo->RegisterOpcodeFunction(name, handler); };
	ICLEO::data_t *ReadParam(void *scriptHandle) { return cleo->ReadParam(scriptHandle); };
	ICLEO::data_t *GetPointerToScriptVar(void *scriptHandle) { return cleo->GetPointerToScriptVar(scriptHandle); };
	bool ReadString8byte(void *scriptHandle, char *buf, uint32_t size) { return cleo->ReadString8byte(scriptHandle, buf, size); };
	bool ReadStringLong(void *scriptHandle, char *buf, uint32_t size) { return cleo->ReadStringLong(scriptHandle, buf, size); };
	bool IsParamListEnd(uint32_t ip) { return cleo->IsParamListEnd(ip); };
	void SkipParamListEnd(uint32_t *ip) { return cleo->SkipParamListEnd(ip); };
	void *GetRealCodePointer(uint32_t ip) { return cleo->GetRealCodePointer(ip); };
	uint32_t GetIpUsingRealCodePointer(void *realPtr) { return cleo->GetIpUsingRealCodePointer(realPtr); };
	void *GetRealLabelPointer(void *scriptHandle, uint32_t offset) { return cleo->GetRealLabelPointer(scriptHandle, offset); };
};