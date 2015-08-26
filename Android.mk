LOCAL_PATH := $(call my-dir)

# JNI Wrapper
include $(CLEAR_VARS)

LOCAL_CFLAGS := -g
ifeq ($(TARGET_ARCH_ABI), armeabi-v7a)
    LOCAL_CFLAGS += -mfloat-abi=softfp -mfpu=neon
endif
LOCAL_MODULE := libarutils_android
LOCAL_SRC_FILES := JNI/c/ARUTILS_JNI_RFCommFtp.c JNI/c/ARUTILS_JNI_FtpConnection.c JNI/c/ARUTILS_JNI_HttpConnection.c JNI/c/ARUTILS_JNI_FileSystem.c JNI/c/ARUTILS_JNI_BLEFtp.c JNI/c/ARUTILS_JNI_Manager.c
LOCAL_LDLIBS := -llog -lz
LOCAL_SHARED_LIBRARIES := libARUtils-prebuilt libARSAL-prebuilt
include $(BUILD_SHARED_LIBRARY)
