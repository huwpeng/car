ifneq ($(strip $(SOURCE_DIR)),)
	export VPATH:=$(foreach dir,$(strip $(SOURCE_DIR)),$(strip $(TOP_DIR))/$(strip $(dir)):)
endif

CROSS=$(strip $(CROSS_COMPILER_CMD_PATH))/$(strip $(CROSS_COMPILER_PREFIX))
CC=$(strip $(CROSS))gcc
CXX=$(strip $(CROSS))g++
AR=$(strip $(CROSS))ar
AS=$(strip $(CROSS))as
LD=$(strip $(CROSS))ld
NM=$(strip $(CROSS))nm
STRIP=$(strip $(CROSS))strip -s
CPP=$(strip $(CC)) -E
SED?=sed
RM=rm -f -r
CP=cp -f -r
TAR=tar -z
MV=mv -f
MKDIR=mkdir -p
MK_ROMFS=$(strip $(HISDK_TOOLS_PATH))genromfs
MK_CRAMFS=$(strip $(HISDK_TOOLS_PATH))mkfs.cramfs
MK_JFFS2=$(strip $(HISDK_TOOLS_PATH))mkfs.jffs2
MK_YAFFS2=$(strip $(HISDK_TOOLS_PATH))mkfs.yaffs2
MK_IMAGE=$(strip $(HISDK_TOOLS_PATH))mkimage

LDFLAG_PREFIX:=-Wl,
ASFLAG_PREFIX:=-Wa,
CPPFLAG_PREFIX:=-Wp,

ifeq ($(strip $(APP_DEBUG)),yes)
	CFLAGS += -g -Wall -O0
else
ifeq ($(strip $(APP_OPTIMIZE)),no)
	CFLAGS += -W -O0
else
	CFLAGS += -W -O
endif
endif

ifeq ($(strip $(APP_SHARED_LIB)),yes)
	CFLAGS += -fPIC -shared
endif

ifeq ($(strip $(USE_DLL)),yes)
	CFLAGS += -rdynamic
endif

ifneq ($(strip $(USER_MACRO)),)
	CPPFLAGS += $(patsubst %,-D% ,$(strip $(USER_MACRO)))
	ASFLAGS += $(patsubst %,-D% ,$(strip $(USER_MACRO)))
endif

ifneq ($(strip $(INCLUDE_DIR)),)
	CPPFLAGS += $(patsubst %,-I % ,$(strip $(INCLUDE_DIR)))
	ASFLAGS += $(patsubst %,-I % ,$(strip $(INCLUDE_DIR)))
endif

ifeq ($(strip $(LIB_STATIC)),yes)
	LDFLAGS += $(strip $(LDFLAG_PREFIX))-Bstatic
else
	LDFLAGS += $(strip $(LDFLAG_PREFIX))-Bdynamic
endif

ifneq ($(strip $(LIB_DIR)),)
	LDFLAGS += $(patsubst %,-L % ,$(strip $(LIB_DIR)))
endif
ifneq ($(strip $(LIB)),)
	LDLIBS += $(patsubst %,-l % ,$(strip $(LIB)))
endif


ifeq ($(strip $(TARGET_ENDIAN)),big-endian)
	LDFLAGS += $(strip $(LDFLAG_PREFIX))-EB
	ASFLAGS += $(strip $(ASFLAG_PREFIX))-EB
endif
ifeq ($(strip $(TARGET_ENDIAN)),little-endian)
	LDFLAGS += $(strip $(LDFLAG_PREFIX))-EL
	ASFLAGS += $(strip $(ASFLAG_PREFIX))-EL
endif


ifneq ($(filter-out %.c %.C %.cpp %.CPP %.cc %.CC %.s %.S,$(strip $(SOURCE))),)
	$(error 存在本项目不允许的非法源文件!)
endif


OBJ:=$(patsubst %.S,%.o,\
$(patsubst %.s,%.o,\
$(patsubst %.cc,%.o,\
$(patsubst %.cpp,%.o,\
$(patsubst %.C,%.o,\
$(patsubst %.c,%.o\
,$(strip $(SOURCE))))))))

DEPEND:=$(patsubst %.o,%.d,$(strip $(OBJ)))
-include $(strip $(DEPEND))


CMD_COMPILE_C 		=	$(strip $(CC)) $(strip $(CFLAGS)) $(strip $(CPPFLAGS)) -c -o $@ $<
CMD_COMPILE_CXX		=	$(strip $(CXX)) $(strip $(CFLAGS)) $(strip $(CPPFLAGS)) $(strip $(CXXFLAGS)) -c -o $@ $<
CMD_COMPILE_ASM		=	$(strip $(CC)) $(strip $(ASFLAGS)) -o $@ $<	
CMD_LINK_OBJ			=	$(strip $(CC)) $(strip $(CFLAGS)) $(strip $(LDFLAGS))
CMD_MAKE_DEPEND		=	@set -e; $(strip $(RM)) $@; \
				$(strip $(CC)) -M $(CPPFLAGS) $< > $@.$$$$; \
				$(strip $(SED)) 's,\($*\)\.o[ :]*,\1.o $@ : ,g' < $@.$$$$ > $@; \
				$(strip $(RM)) $@.$$$$


%.o : %.c
	$(strip $(CMD_COMPILE_C))

%.o : %.C
	$(strip $(CMD_COMPILE_C))
	
%.o : %.cpp
	$(strip $(CMD_COMPILE_CXX))

%.o : %.cc
	$(strip $(CMD_COMPILE_CXX))

%.o : %.s
	$(strip $(CMD_COMPILE_ASM))
	
%.o : %.S
	$(strip $(CMD_COMPILE_ASM))

%.d : %.c
	$(strip $(CMD_MAKE_DEPEND))
	
%.d : %.C
	$(strip $(CMD_MAKE_DEPEND))

%.d : %.cpp
	$(strip $(CMD_MAKE_DEPEND))

%.d : %.cc
	$(strip $(CMD_MAKE_DEPEND))

%.d : %.s
	$(strip $(CMD_MAKE_DEPEND))

%.d : %.S
	$(strip $(CMD_MAKE_DEPEND))
	
	
	
	