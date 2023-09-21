    enum eLogicalOperation : uint16_t
    {
        NONE = 0, // just replace

        AND_2 = 1, // AND operation on results of next two conditional opcodes
        AND_3,
        AND_4,
        AND_5,
        AND_6,
        AND_7,
        AND_END,

        OR_2 = 21, // OR operation on results of next two conditional opcodes
        OR_3,
        OR_4,
        OR_5,
        OR_6,
        OR_7,
        OR_END,
    };

    enum eDataType
    {
        DT_END,
        DT_DWORD,
        DT_VAR,
        DT_LVAR,
        DT_BYTE,
        DT_WORD,
        DT_FLOAT,
        DT_VAR_ARRAY,
        DT_LVAR_ARRAY,
        DT_TEXTLABEL,
        DT_VAR_TEXTLABEL,
        DT_LVAR_TEXTLABEL,
        DT_VAR_TEXTLABEL_ARRAY,
        DT_LVAR_TEXTLABEL_ARRAY,
        DT_VARLEN_STRING,
        DT_STRING,
        DT_VAR_STRING,
        DT_LVAR_STRING,
        DT_VAR_STRING_ARRAY,
        DT_LVAR_STRING_ARRAY
    };

    struct ScmFunction
    {
        unsigned short prevScmFunctionId, thisScmFunctionId;
        uint8_t *retnAddress;
        uint8_t *savedStack[8]; // gosub stack
        uint16_t savedSP;
        eLogicalOperation savedLogicalOp;
        int savedTls[40];
        int* savedRets[40]; // EXPERIMENTAL
        std::list<std::string> stringParams; // texts with this scope lifetime
        bool savedCondResult;
        bool savedNotFlag;
        static const size_t store_size = 0x400;
        static ScmFunction *Store[store_size];
        static size_t allocationPlace; // contains an index of last allocated object

        void *operator new(size_t size)
        {
            size_t start_search = allocationPlace;
            while (Store[allocationPlace]) // find first unused position in store
            {
                if (++allocationPlace >= store_size) allocationPlace = 0; // end of store reached
                if (allocationPlace == start_search) return NULL;         // the store is filled up
            }
            ScmFunction *obj = (ScmFunction*)(::operator new(size));
            Store[allocationPlace] = obj;
            return obj;
        }

        void operator delete(void *mem)
        {
            Store[((ScmFunction*)mem)->thisScmFunctionId] = NULL;
            ::operator delete(mem);
        }

        ScmFunction(void *thread) : prevScmFunctionId(GetScmFunc(thread))
        {
            // create snapshot of current scope
            memcpy(savedStack, GetStack(thread), 4 * ValueForSA(8, 6));
            savedSP = GetStackDepth(thread);

            // create snapshot of parent scope's local variables
            void* scope = (void*)((uintptr_t)thread + ValueForGame(48, 48, 60, 96, 520));
            if(*nGameIdent == GTASA && IsMissionScript(thread)) scope = (void*)LocalVariablesForCurrentMission;
            memcpy(savedTls, scope, 4 * ValueForSA(40, 16));

            savedCondResult = GetCond(thread);
            savedLogicalOp = (eLogicalOperation)GetLogicalOp(thread);
            savedNotFlag = GetNotFlag(thread);
            memset(savedRets, 0, sizeof(savedRets)); // EXPERIMENTAL

            // init new scope
            memset(GetStack(thread), 0, 4 * ValueForSA(8, 6));
            GetStackDepth(thread) = 0;
            GetCond(thread) = false;
            GetLogicalOp(thread) = eLogicalOperation::NONE;
            GetNotFlag(thread) = false;

            SetScmFunc(thread, thisScmFunctionId = allocationPlace);
        }

        void Return(void *thread)
        {
            // restore parent scope's gosub call stack
            memcpy(GetStack(thread), savedStack, 4 * ValueForSA(8, 6));
            GetStackDepth(thread) = savedSP;
            
            // restore parent scope's local variables
            void* scope = (void*)((uintptr_t)thread + ValueForGame(48, 48, 60, 96, 520));
            if(*nGameIdent == GTASA && IsMissionScript(thread)) scope = (void*)LocalVariablesForCurrentMission;
            memcpy(scope, savedTls, 4 * ValueForSA(40, 16));

            // process conditional result of just ended function in parent scope
            bool condResult = GetCond(thread);
            if (savedNotFlag) condResult = !condResult;

            if (savedLogicalOp >= eLogicalOperation::AND_2 && savedLogicalOp < eLogicalOperation::AND_END)
            {
                GetCond(thread) = savedCondResult && condResult;
                GetLogicalOp(thread) = (uint16_t)savedLogicalOp - 1;
            }
            else if(savedLogicalOp >= eLogicalOperation::OR_2 && savedLogicalOp < eLogicalOperation::OR_END)
            {
                GetCond(thread) = savedCondResult || condResult;
                GetLogicalOp(thread) = (uint16_t)savedLogicalOp - 1;
            }
            else // eLogicalOperation::NONE
            {
                GetCond(thread) = condResult;
                GetLogicalOp(thread) = savedLogicalOp;
            }

            GetPC(thread) = retnAddress;
            SetScmFunc(thread, prevScmFunctionId);
        }
    };

    size_t ScmFunction::allocationPlace = 0;
    ScmFunction* ScmFunction::Store[store_size] = { NULL };