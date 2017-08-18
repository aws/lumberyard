LOCAL_PATH := $(call my-dir)

################################
#
# SDL Extension shared library  
#
################################

include $(CLEAR_VARS)

LOCAL_MODULE     := SDL2Ext
LOCAL_SRC_FILES  := SDLExt_android.c
LOCAL_C_INCLUDES := $(LOCAL_PATH)/../../../../SDKs/SDL2/include/android-armeabi-v7a $(LOCAL_PATH)/../../../../SDKs/SDL2/src/src
LOCAL_SHARED_LIBRARIES += SDL2
LOCAL_LDLIBS := -landroid -llog

include $(BUILD_SHARED_LIBRARY)

$(call import-module,SDL2/src)
