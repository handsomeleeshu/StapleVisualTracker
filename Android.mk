LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := Staple
LOCAL_SRC_FILES := $(LOCAL_PATH)/staple.c
LOCAL_SRC_FILES += $(LOCAL_PATH)/stp_colour_map.c
LOCAL_SRC_FILES += $(LOCAL_PATH)/stp_feature_map.c
LOCAL_SRC_FILES += $(LOCAL_PATH)/stp_fft.c
LOCAL_SRC_FILES += $(LOCAL_PATH)/stp_fhog.c
LOCAL_SRC_FILES += $(LOCAL_PATH)/stp_hist.c
LOCAL_SRC_FILES += $(LOCAL_PATH)/stp_image.c
LOCAL_SRC_FILES += $(LOCAL_PATH)/stp_scaler.c
LOCAL_SRC_FILES += $(LOCAL_PATH)/stp_table.c
LOCAL_SRC_FILES += $(LOCAL_PATH)/stp_tracker.c
LOCAL_C_INCLUDES += $(LOCAL_PATH)/

ifeq ($(TARGET_ARCH_ABI), armeabi-v7a)
LOCAL_CFLAGS += -include $(LOCAL_PATH)/arm/armv7a/staple_neon_opt.h
LOCAL_SRC_FILES += $(LOCAL_PATH)/arm/armv7a/staple_neon_opt.c
LOCAL_CFLAGS += -fopenmp -ftree-vectorize -mfloat-abi=softfp -mfpu=neon
LOCAL_LDFLAGS += -fopenmp
endif

ifeq ($(TARGET_ARCH_ABI), arm64-v8a)
LOCAL_CFLAGS += -include $(LOCAL_PATH)/arm/armv8a/staple_neon_opt.h
LOCAL_SRC_FILES += $(LOCAL_PATH)/arm/armv8a/staple_neon_opt.c
LOCAL_CFLAGS += -fopenmp -ftree-vectorize
LOCAL_LDFLAGS += -fopenmp
endif


LOCAL_SHARED_LIBRARIES := Staple
include $(BUILD_SHARED_LIBRARY)

