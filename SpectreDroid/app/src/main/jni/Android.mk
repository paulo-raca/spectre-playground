LOCAL_PATH := $(call my-dir)

define walk
  $(wildcard $(1)) $(foreach e, $(wildcard $(1)/*), $(call walk, $(e)))
endef

ALL_FILES := $(call walk, $(LOCAL_PATH))
ALL_C_FILES := $(filter %.c, $(ALL_FILES))
ALL_CPP_FILES := $(filter %.cpp, $(ALL_FILES))

# shared ipercron
include $(CLEAR_VARS)
LOCAL_MODULE := spectre

LOCAL_LDLIBS := -pthread
LOCAL_LDLIBS += -landroid
LOCAL_LDLIBS += -llog

LOCAL_C_INCLUDES += $(LOCAL_PATH)/libflush

LOCAL_CFLAGS += -g
LOCAL_CFLAGS += -O0
LOCAL_CFLAGS += -fexceptions
LOCAL_CFLAGS += -fpermissive
LOCAL_CFLAGS += -DANDROID_NDK
LOCAL_CFLAGS += -DTIME_SOURCE=TIME_SOURCE_MONOTONIC_CLOCK
LOCAL_CFLAGS += -DDEVICE_CONFIGURATION=strategies/default.h


ifeq ($(TARGET_ARCH_ABI),armeabi-v7a)
LOCAL_CFLAGS += -DUSE_EVICTION=1
endif

LOCAL_SRC_FILES := $(ALL_C_FILES:$(LOCAL_PATH)/%=%)
LOCAL_SRC_FILES += $(ALL_CPP_FILES:$(LOCAL_PATH)/%=%)

include $(BUILD_SHARED_LIBRARY)
