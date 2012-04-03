LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
    PostProc.cpp \
    PostProcFactory.cpp \
    PostProcController.cpp \
    PostProcNativeWindow.cpp \

LOCAL_C_INCLUDES := \
    $(TOP)/frameworks/av/media/libstagefright/include \
    $(TOP)/frameworks/av/include/media/stagefright \
    $(TOP)/frameworks/av/media/libstagefright/include/postproc \
    $(TOP)/hardware/qcom/display/libgralloc \
    $(TOP)/hardware/qcom/media/libstagefrighthw \
    $(TOP)/frameworks/native/include/ui \
    $(TOP)/frameworks/native/include/media/openmax \
    $(TOP)/frameworks/av/include/utils

ifeq ($(call is-board-platform,msm8960),true)

    ifeq ($(TARGET_USES_POST_PROCESSING),true)
        LOCAL_CFLAGS += -DUSE_ION
        LOCAL_CFLAGS += -DPOSTPROC_SUPPORTED

        LOCAL_SRC_FILES += PostProcC2DColorConversion.cpp
        LOCAL_SRC_FILES += PostProc2Dto3D.cpp

        LOCAL_C_INCLUDES += $(TOP)/hardware/qcom/display/libcopybit
        LOCAL_C_INCLUDES += $(TOP)/hardware/qcom/media/libc2dcolorconvert
        LOCAL_C_INCLUDES += $(TARGET_OUT_INTERMEDIATES)/KERNEL_OBJ/usr/include
        LOCAL_C_INCLUDES += $(TARGET_OUT_HEADERS)/mm-swdec2dto3d
        LOCAL_C_INCLUDES += $(TOP)/hardware/qcom/display/libqdutils

        LOCAL_ADDITIONAL_DEPENDENCIES := $(TARGET_OUT_INTERMEDIATES)/KERNEL_OBJ/usr
    endif

endif

LOCAL_MODULE := libstagefright_postproc

LOCAL_SHARED_LIBRARIES := libui liblog libutils libdl

LOCAL_MODULE_TAGS := optional

LOCAL_PRELINK_MODULE:= false

include $(BUILD_STATIC_LIBRARY)

