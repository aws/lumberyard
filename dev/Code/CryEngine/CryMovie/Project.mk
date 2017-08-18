#############################################################################
## Crytek Source File
## Copyright (C) 2006, Crytek Studios
##
## Creator: Sascha Demetrio
## Date: Jul 31, 2006
## Description: GNU-make based build system
#############################################################################

PROJECT_TYPE := module
PROJECT_VCPROJ := CryMovie.vcproj

-include $(PROJECT_CODE)/Project_override.mk

PROJECT_CPPFLAGS_COMMON += \
	-I$(CODE_ROOT)/CryEngine/CryCommon \
	-I$(CODE_ROOT)/SDKs/boost

ifeq ($(MKOPTION_UNITYBUILD),1)
PROJECT_SOURCES_CPP_REMOVE := StdAfx.cpp\
	AnimCameraNode.cpp \
	AnimNode.cpp \
	AnimSequence.cpp \
	AnimSplineTrack.cpp \
	AnimTrack.cpp \
	BoolTrack.cpp \
	CaptureTrack.cpp \
	CVarNode.cpp \
	CharacterTrack.cpp \
	ConsoleTrack.cpp \
	CryMovie.cpp \
	EntityNode.cpp \
	EventNode.cpp \
	EventTrack.cpp \
	ExprTrack.cpp \
	FaceSeqTrack.cpp \
	GotoTrack.cpp \
	LookAtTrack.cpp \
	MaterialNode.cpp \
	Movie.cpp \
	MusicTrack.cpp \
	SceneNode.cpp \
	ScriptVarNode.cpp \
	SelectTrack.cpp \
	SequenceIt.cpp \
	SequenceTrack.cpp \
	SoundTrack.cpp \
	TrackEventTrack.cpp \
	CompoundSplineTrack.cpp \
	LayerNode.cpp \
	CommentTrack.cpp \
	AnimPostFXNode.cpp \
	ScreenFaderTrack.cpp \
	AnimSequenceGroup.cpp \
	CommentNode.cpp \
	AnimScreenFaderNode.cpp \
	AnimLightNode.cpp
	
PROJECT_SOURCES_CPP_ADD += CryMovie_uber.cpp

endif

# vim:ts=8:sw=8
