###################### optee-chaincode-proxy ######################
# source: file bases on https://github.com/linaro-swg/optee_examples/tree/master/hello_world

LOCAL_PATH := $(call my-dir)

OPTEE_CLIENT_EXPORT = $(LOCAL_PATH)/../../optee_client/out/export

include $(CLEAR_VARS)
LOCAL_CFLAGS += -DANDROID_BUILD
LOCAL_CFLAGS += -Wall

LOCAL_SRC_FILES += host/main.c

LOCAL_C_INCLUDES := $(OPTEE_CLIENT_EXPORT)/include \

LOCAL_SHARED_LIBRARIES := libteec
LOCAL_MODULE := optee_example_chaincode_proxy_cpp
LOCAL_VENDOR_MODULE := true
LOCAL_MODULE_TAGS := optional
include $(BUILD_EXECUTABLE)
