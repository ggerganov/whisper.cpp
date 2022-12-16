LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)
PROJECT_DIR 	:= $(LOCAL_PATH)/../../../../../../../
WHISPER_LIB_DIR := libwhisper
LOCAL_LDLIBS    := -llog
LOCAL_MODULE    := libwhisper

# Make the final output library smaller by only keeping the symbols referenced from the app.
ifneq ($(APP_OPTIM),debug)
    LOCAL_CFLAGS += -fvisibility=hidden -fvisibility-inlines-hidden
    LOCAL_CFLAGS += -ffunction-sections -fdata-sections
    LOCAL_LDFLAGS += -Wl,--gc-sections
    LOCAL_LDFLAGS += -Wl,--exclude-libs,ALL
    LOCAL_LDFLAGS += -flto
endif

LOCAL_CFLAGS    += -DSTDC_HEADERS -std=c11 -I $(PROJECT_DIR)
LOCAL_CPPFLAGS  += -std=c++11
LOCAL_SRC_FILES := $(PROJECT_DIR)/ggml.c \
                   $(PROJECT_DIR)/whisper.cpp \
                   $(LOCAL_PATH)/jni.c

include $(BUILD_SHARED_LIBRARY)