LOCAL_PATH := $(call my-dir)

###########################
# Rust compilation outputs
###########################

include $(CLEAR_VARS)
LOCAL_MODULE := shadowmask-rs
LOCAL_EXPORT_C_INCLUDES := src/core/include
LOCAL_LIB = ../out/$(TARGET_ARCH_ABI)/libshadowmask-rs.a
ifneq (,$(wildcard $(LOCAL_PATH)/$(LOCAL_LIB)))
LOCAL_SRC_FILES := $(LOCAL_LIB)
include $(PREBUILT_STATIC_LIBRARY)
else
include $(BUILD_STATIC_LIBRARY)
endif

include $(CLEAR_VARS)
LOCAL_MODULE := boot-rs
LOCAL_LIB = ../out/$(TARGET_ARCH_ABI)/libshadowmaskboot-rs.a
ifneq (,$(wildcard $(LOCAL_PATH)/$(LOCAL_LIB)))
LOCAL_SRC_FILES := $(LOCAL_LIB)
include $(PREBUILT_STATIC_LIBRARY)
else
include $(BUILD_STATIC_LIBRARY)
endif

include $(CLEAR_VARS)
LOCAL_MODULE := init-rs
LOCAL_LIB = ../out/$(TARGET_ARCH_ABI)/libshadowmaskinit-rs.a
ifneq (,$(wildcard $(LOCAL_PATH)/$(LOCAL_LIB)))
LOCAL_SRC_FILES := $(LOCAL_LIB)
include $(PREBUILT_STATIC_LIBRARY)
else
include $(BUILD_STATIC_LIBRARY)
endif

include $(CLEAR_VARS)
LOCAL_MODULE := policy-rs
LOCAL_LIB = ../out/$(TARGET_ARCH_ABI)/libshadowmaskpolicy-rs.a
ifneq (,$(wildcard $(LOCAL_PATH)/$(LOCAL_LIB)))
LOCAL_SRC_FILES := $(LOCAL_LIB)
include $(PREBUILT_STATIC_LIBRARY)
else
include $(BUILD_STATIC_LIBRARY)
endif
