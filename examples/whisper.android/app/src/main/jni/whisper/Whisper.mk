WHISPER_LIB_DIR := $(LOCAL_PATH)/../../../../../../../
LOCAL_LDLIBS    := -landroid -llog

# Make the final output library smaller by only keeping the symbols referenced from the app.
ifneq ($(APP_OPTIM),debug)
    LOCAL_CFLAGS += -O3
    LOCAL_CFLAGS += -fvisibility=hidden -fvisibility-inlines-hidden
    LOCAL_CFLAGS += -ffunction-sections -fdata-sections
    LOCAL_LDFLAGS += -Wl,--gc-sections
    LOCAL_LDFLAGS += -Wl,--exclude-libs,ALL
    LOCAL_LDFLAGS += -flto
endif

LOCAL_CFLAGS    += -DSTDC_HEADERS -std=c11 -I $(WHISPER_LIB_DIR)
LOCAL_CPPFLAGS  += -std=c++11
LOCAL_SRC_FILES := $(WHISPER_LIB_DIR)/ggml.c \
                   $(WHISPER_LIB_DIR)/whisper.cpp \
                   $(LOCAL_PATH)/jni.c