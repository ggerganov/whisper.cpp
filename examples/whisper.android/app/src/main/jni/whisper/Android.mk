LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)
LOCAL_MODULE    := libwhisper
include $(LOCAL_PATH)/Whisper.mk
include $(BUILD_SHARED_LIBRARY)

ifeq ($(TARGET_ARCH_ABI),armeabi-v7a)
	include $(CLEAR_VARS)
	LOCAL_MODULE    := libwhisper_vfpv4
	include $(LOCAL_PATH)/Whisper.mk
	# Allow building NEON FMA code.
	# https://android.googlesource.com/platform/ndk/+/master/sources/android/cpufeatures/cpu-features.h
	LOCAL_CFLAGS += -mfpu=neon-vfpv4
	include $(BUILD_SHARED_LIBRARY)
endif