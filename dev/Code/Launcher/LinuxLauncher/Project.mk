#############################################################################
## Crytek Source File
## Copyright (C) 2006, Crytek Studios
##
## Creator: Sascha Demetrio
## Date: Jul 31, 2006
## Description: GNU-make based build system
#############################################################################

PROJECT_TYPE := program

PROJECT_CPPFLAGS_COMMON := \
	-I$(CODE_ROOT)/CryEngine/CryCommon \
	-I$(CODE_ROOT)/CryEngine/CrySystem \
	-I$(CODE_ROOT)/CryEngine/CryAction

# The list of modules (shared objects) that should be linked to the final
# executable.
PROJECT_LINKMODULES := \
	Cry3DEngine \
	CryAction \
	CryAISystem \
	CryAnimation \
	CryEntitySystem \
	CryFont \
	CryInput \
	CryMovie \
	CryNetwork \
	CryPhysics \
	CryScriptSystem \
	CrySoundSystem \
	CrySystem \
	GameDll
ifeq ($(OPTION_DEDICATED),1)
 PROJECT_LINKMODULES += XRenderNULL
endif

PROJECT_SOURCES_CPP := StdAfx.cpp Main.cpp Assert.cpp
PROJECT_SOURCES_H := StdAfx.h assert.h
PROJECT_SOURCE_PCH := StdAfx.h

PROJECT_LDFLAGS :=
PROJECT_LDLIBS :=

ifneq ($(OPTION_DEDICATED),1)
 PROJECT_LDLIBS += -lSDL
endif

# vim:ts=8:sw=8

