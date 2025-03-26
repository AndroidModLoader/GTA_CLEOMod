LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_CPP_EXTENSION := .cpp .cc
LOCAL_MODULE    := CLEOMod
LOCAL_SRC_FILES := main.cpp mod/logger.cpp mod/config.cpp libcleo.cpp cleo201_refactor.cpp cleo4opcodes.cpp cleo5opcodes.cpp cleomath.cpp mini-scanf-cleo/c_scan.c
LOCAL_CFLAGS += -O2 -mfloat-abi=softfp -DNDEBUG -std=c11 -DAML_CLEO
LOCAL_CXXFLAGS += -O2 -mfloat-abi=softfp -DNDEBUG -std=c++17
LOCAL_LDLIBS += -llog
include $(BUILD_SHARED_LIBRARY)