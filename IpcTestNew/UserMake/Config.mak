TARGET_OS?=linux
TARGET_ARCH?=arm
CROSS_COMPILER_PREFIX:=arm-linux-gnueabihf-
TARGET_ENDIAN=default-endian						#[big-endian,little-endian,default-endian]


CROSS_COMPILER_ROOT:=/opt/gcc-linaro-6.5.0-2018.12-x86_64_arm-linux-gnueabihf
ifneq ($(strip $(CROSS_COMPILER_ROOT)),)
ifeq ($(strip $(CROSS_COMPILER_CMD_PATH)),)
	CROSS_COMPILER_CMD_PATH:=$(strip $(CROSS_COMPILER_ROOT))/bin
endif
endif

HISDK_ROOT:=
HISDK_TOOLS_PATH:=
ifneq ($(strip $(HISDK_ROOT)),)
ifeq ($(strip $(HISDK_TOOLS_PATH)),)
	HISDK_TOOLS_PATH:=$(strip $(HISDK_ROOT))/tools/bin
endif
endif




APP_SHARED_LIB:=no							#[yes,no]
APP_DEBUG:=no 									#[yes,no]
APP_OPTIMIZE:=yes								#[yes,no]
LIB_STATIC:=no									#[yes,no]
STRIP_TARGET:=yes								#[yes,no]
USE_DLL:=no											#[yes,no]

PRIVATE_LIB:=$(strip $(CURDIR))/Lib
PRIVATE_INC:=$(strip $(CURDIR))/include
INCLUDE_DIR:=$(strip $(PRIVATE_INC))
LIB_DIR:=$(strip $(PRIVATE_LIB))


LIB:=pthread m LocalAppPublicLib iksemel IpcApiLib hyperlpr3 MNN stdc++ opencv_core opencv_imgproc opencv_highgui opencv_imgcodecs
USER_MACRO:=
APP_TARGET_ORI:=IpcTest
SOURCE:=$(wildcard ./source/*.c ./source/*.cpp)

TOP_DIR:=
SOURCE_DIR:=source
APP_DIR:=App
OUT_DIR:=Out
OUT_FILES:=

INSTALL_DIR:=
FS_ROOT:=
DEBUG_ROOT:=
DOWNLOAD_DIR:=




ifeq ($(strip $(APP_SHARED_LIB)),yes)
	APP_TARGET:=lib$(strip $(APP_TARGET_ORI)).so
else
	APP_TARGET:=$(strip $(APP_TARGET_ORI))
endif

ifeq ($(strip $(APP_DEBUG)),yes)
	USER_MACRO += DEBUG
endif
ifneq ($(strip $(WEB_MACRO)),)
	USER_MACRO += $(strip $(WEB_MACRO))
endif

ifeq ($(strip $(APP_SHARED_LIB)),yes)
ifeq ($(strip $(INSTALL_DIR)),)
	INSTALL_DIR:=lib
endif
endif



