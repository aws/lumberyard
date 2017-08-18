#############################################################################
## Crytek Source File
## Copyright (C) 2006, Crytek Studios
##
## Creator: Sascha Demetrio
## Date: Jul 31, 2006
## Description: GNU-make based build system
#############################################################################

PROJECT_TYPE := module
PROJECT_VCPROJ := CryFont.vcproj

-include $(PROJECT_CODE)/Project_override.mk

PROJECT_CPPFLAGS_COMMON += \
	-I$(CODE_ROOT)/CryEngine/CryCommon \
	-I$(PROJECT_CODE)/FreeType2/include \
	-I$(CODE_ROOT)/SDKs/boost

ifeq ($(MKOPTION_UNITYBUILD),1)
PROJECT_SOURCES_CPP_REMOVE := StdAfx.cpp\
	CryFont.cpp \
	FFont.cpp \
	FFontXML.cpp \
	FontRenderer.cpp \
	FontTexture.cpp \
	GlyphBitmap.cpp \
	GlyphCache.cpp \
	ICryFont.cpp \
	NullFont.cpp

PROJECT_SOURCES_CPP_ADD += CryFont_main_uber.cpp

endif

ifeq ($(ARCH),PS3-cell)
  PROJECT_SOURCES_C_REMOVE += FreeType2/%
endif

# vim:ts=8:sw=8

