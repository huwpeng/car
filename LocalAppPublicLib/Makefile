include $(strip $(CURDIR))/UserMake/Config.mak

ifneq ($(strip $(TARGET_OS)),linux)
	$(error  本项目必须指定运行平台arm-linux)
endif
ifneq ($(strip $(TARGET_ARCH)),arm)
	$(error  本项目必须指定运行平台arm-linux)
endif

ifeq ($(strip $(TOP_DIR)),)
	TOP_DIR=$(strip $(CURDIR))
endif

ifeq ($(strip $(APP_TARGET)),)
	APP_TARGET=VideoServer
endif

ifeq ($(strip $(SOURCE)),)
	SOURCE:=$(wildcard *.c)
endif

ifeq ($(strip $(SOURCE)),)
	TEMPFILES:=*.o
	DENCFILES:=*.d
else
	TEMPFILES:=$(strip ${SOURCE})
	TEMPFILES:=${patsubst %.s,%.o,$(strip $(TEMPFILES))}
	TEMPFILES:=${patsubst %.S,%.o,$(strip $(TEMPFILES))}
	TEMPFILES:=${patsubst %.c,%.o,$(strip $(TEMPFILES))}
	TEMPFILES:=${patsubst %.C,%.o,$(strip $(TEMPFILES))}
	TEMPFILES:=${patsubst %.cpp,%.o,$(strip $(TEMPFILES))}
	TEMPFILES:=${patsubst %.cc,%.o,$(strip $(TEMPFILES))}
	DENCFILES:=${patsubst %.o,%.d,$(strip $(TEMPFILES))}
endif
TEMPFILES += $(strip $(TOP_DIR))/source/Debug $(strip $(TOP_DIR))/source/Release








.PHONY:	all clean cleanall

all:$(strip $(APP_TARGET))

cleanall:clean
	$(strip $(RM)) $(strip $(DENCFILES))
	$(strip $(RM)) $(strip $(APP_TARGET))
	
	
clean:
	$(strip $(RM)) $(strip $(TEMPFILES))
	
	
include	$(strip $(CURDIR))/UserMake/Rules.mak


$(strip $(APP_TARGET)):$(strip $(OBJ))
	$(strip $(CMD_LINK_OBJ)) -o $(strip $(APP_TARGET)) $(strip $(OBJ)) $(strip $(LOADLIBES)) $(strip $(LDLIBS));
ifeq ($(strip $(STRIP_TARGET)),yes)
	$(strip $(STRIP)) $(strip $(APP_TARGET))
endif




ifneq ($(strip $(APP_DIR)),)
	$(strip $(MKDIR)) $(strip $(APP_DIR))
	$(strip $(CP)) $(strip $(TOP_DIR))/$(strip $(APP_TARGET)) $(strip $(TOP_DIR))/$(strip $(APP_DIR))/
endif

ifneq ($(strip $(DOWNLOAD_DIR)),)
	$(strip $(CP)) $(strip $(TOP_DIR))/$(strip $(APP_TARGET)) $(strip $(DOWNLOAD_DIR))/
endif

ifneq ($(strip $(FS_ROOT)),)
ifneq ($(strip $(INSTALL_DIR)),)
	$(strip $(CP)) $(strip $(TOP_DIR))/$(strip $(APP_TARGET)) $(strip $(FS_ROOT))/$(strip $(INSTALL_DIR))/
else
	$(strip $(CP)) $(strip $(TOP_DIR))/$(strip $(APP_TARGET)) $(strip $(FS_ROOT))/usr/bin/
endif
endif

ifneq ($(strip $(DEBUG_ROOT)),)
ifneq ($(strip $(INSTALL_DIR)),)
	$(strip $(CP)) $(strip $(TOP_DIR))/$(strip $(APP_TARGET)) $(strip $(DEBUG_ROOT))/$(strip $(INSTALL_DIR))/
else
	$(strip $(CP)) $(strip $(TOP_DIR))/$(strip $(APP_TARGET)) $(strip $(DEBUG_ROOT))/usr/bin/
endif
endif



ifneq ($(strip $(OUT_DIR)),)
ifneq ($(strip $(OUT_FILES)),)
	$(strip $(CP)) -t $(strip $(TOP_DIR))/$(strip $(OUT_DIR))/ $(strip $(OUT_FILES))
endif
endif




ifneq ($(strip $(APP_DIR)),)
	$(strip $(RM))	$(strip $(APP_TARGET))
endif

