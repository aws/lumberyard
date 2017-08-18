# Crytek Source File.
# Copyright (C), Crytek Studios, 2001-2006.
# -------------------------------------------------------------------------
# Description:
# Makefile for the Linux launcher project.
# -------------------------------------------------------------------------
# History:
# - Jul 13, 2006:	Created by Sascha Demetrio

DEFAULT_TARGET ?= alldebug

INCLUDE_FLAGS = \
  -I$(PROJECT_HOME)/Code/CryEngine/CryCommon \
  -I$(PROJECT_HOME)/Code/CryEngine/CryAction \
  -I$(PROJECT_HOME)/Code/CryEngine/CrySystem
LIBS = 
LINK = shared
DEP_OBJS = \
	$(PROJECT_HOME)/BinLinux/game \
	$(PROJECT_HOME)/BinLinux/cryaction \
	$(PROJECT_HOME)/BinLinux/crysystem \
	$(PROJECT_HOME)/BinLinux/crynetwork \
	$(PROJECT_HOME)/BinLinux/cry3dengine \
	$(PROJECT_HOME)/BinLinux/cryaisystem \
	$(PROJECT_HOME)/BinLinux/cryanimation \
	$(PROJECT_HOME)/BinLinux/cryentitysystem \
	$(PROJECT_HOME)/BinLinux/cryfont \
	$(PROJECT_HOME)/BinLinux/cryinput \
	$(PROJECT_HOME)/BinLinux/crymovie \
	$(PROJECT_HOME)/BinLinux/cryphysics \
	$(PROJECT_HOME)/BinLinux/cryscriptsystem \
	$(PROJECT_HOME)/BinLinux/crysoundsystem

PROJECTS = \
	$(PROJECT_HOME)/Code/CryEngine/Cry3DEngine \
	$(PROJECT_HOME)/Code/CryEngine/CryAnimation \
	$(PROJECT_HOME)/Code/CryEngine/CryFont \
	$(PROJECT_HOME)/Code/CryEngine/CryNetwork \
	$(PROJECT_HOME)/Code/CryEngine/CrySoundSystem \
	$(PROJECT_HOME)/Code/CryEngine/CryAction \
	$(PROJECT_HOME)/Code/CryEngine/CryInput \
	$(PROJECT_HOME)/Code/CryEngine/CryPhysics \
	$(PROJECT_HOME)/Code/CryEngine/CrySystem \
	$(PROJECT_HOME)/Code/CryEngine/CryAISystem \
	$(PROJECT_HOME)/Code/CryEngine/CryEntitySystem \
	$(PROJECT_HOME)/Code/CryEngine/CryMovie \
	$(PROJECT_HOME)/Code/CryEngine/CryScriptSystem \
	$(PROJECT_HOME)/Code/Game02/GameDll

ifneq "$(BUILD_PROJECT)" ""
# Build/rebuild of a specific project requested.
ifeq "$(BUILD_PROJECT)" "GameDll"
PROJECTS := $(PROJECT_HOME)/Code/Game02/GameDll
else
PROJECTS := $(PROJECT_HOME)/Code/CryEngine/$(BUILD_PROJECT)
endif
endif

ifeq "$(DEDICATED_SERVER)" "1"
DEP_OBJS += $(PROJECT_HOME)/BinLinux/xrendernull
PROJECTS += $(PROJECT_HOME)/Code/CryEngine/RenderDll/XRenderNULL
else
#DEP_OBJS += \
#	$(PROJECT_HOME)/BinLinux/xrenderd3d9 \
#	$(PROJECT_HOME)/BinLinux/xrendernull
PROJECTS += \
	$(PROJECT_HOME)/Code/CryEngine/RenderDll/XRenderD3D9 \
	$(PROJECT_HOME)/Code/CryEngine/RenderDll/XRenderNULL
endif

EXTRA_DEBUG_DEPS =

LIBS_NO_SUFFIX = -lm

VPATH_ADD = ../../CryEngine/CryCommon

FILES_ADD = Assert.cpp

# Additional dependencies
DebugLinux/CryCommon.o: $(PROJECT_HOME)/Code/CryEngine/CryCommon/WinBase.cpp
DebugLinux/CryCommon.o: $(PROJECT_HOME)/Code/CryEngine/CryCommon/TypeInfo.cpp
DebugLinux/CryCommon.o: $(PROJECT_HOME)/Code/CryEngine/CryCommon/AutoTypeInfo.cpp
DebugLinux/CryCommon.o: $(PROJECT_HOME)/Code/CryEngine/CryCommon/MTPseudoRandom.cpp
DebugLinux/CryCommon.o: $(PROJECT_HOME)/Code/CryEngine/CryCommon/CryStructPack.cpp
ifeq "$(TMP_CRYMP)" "1"
DebugLinux/CryCommon.o: $(PROJECT_HOME)/Code/CryEngine/CryCommon/CryMPJobManager.cpp
endif
ProfileLinux/CryCommon.o: $(PROJECT_HOME)/Code/CryEngine/CryCommon/WinBase.cpp
ProfileLinux/CryCommon.o: $(PROJECT_HOME)/Code/CryEngine/CryCommon/TypeInfo.cpp
ProfileLinux/CryCommon.o: $(PROJECT_HOME)/Code/CryEngine/CryCommon/AutoTypeInfo.cpp
ProfileLinux/CryCommon.o: $(PROJECT_HOME)/Code/CryEngine/CryCommon/MTPseudoRandom.cpp
ProfileLinux/CryCommon.o: $(PROJECT_HOME)/Code/CryEngine/CryCommon/CryStructPack.cpp
ifeq "$(TMP_CRYMP)" "1"
ProfileLinux/CryCommon.o: $(PROJECT_HOME)/Code/CryEngine/CryCommon/CryMPJobManager.cpp
endif

OUTPUT = linuxlauncher
PROJECT = LinuxLauncher
OUTPUT_TYPE = program

CXXFLAGS_OPT_LEVEL = -O2

all: $(DEFAULT_TARGET)

.PHONY: alldebug
alldebug: _build_libs_debug

.PHONY: allrelease
allrelease: _build_libs_release

clean: cleandebug cleanrelease
	rm -f $(addsuffix .o, $(DEP_OBJS))
	rm -f $(addsuffix _debug.o, $(DEP_OBJS))
	rm -f $(RELEASE_OUTPUT)
	rm -f $(DEBUG_OUTPUT)
	@for P in $(PROJECTS); do \
		$(MAKE) -C "$$P" cleandebug cleanrelease; \
	done
	@(find '$(PROJECT_HOME)' -name '*.gch'; echo none.gch) |xargs rm -vf

depend: _depend
	@for P in $(PROJECTS); do \
		$(MAKE) -C "$$P" _depend; \
	done

# This is a hack to work around the code lock.
# Make the launcher compile for both MAIN and CryMP_WIP.
ifeq "$(CRYMP_WIP)" "1"
CFLAGS_ADD += -DCRYMP_WIP
endif

include ../../CryEngine/CryCommon/Makefile.include

.PHONY: _build_libs_debug
_build_libs_debug:
	@for P in $(PROJECTS); do \
		$(MAKE) -C "$$P" alldebug || exit $?; \
	done

.PHONY: _build_libs_release
_build_libs_release:
	@for P in $(PROJECTS); do \
		$(MAKE) -C "$$P" allrelease || exit $?; \
	done

