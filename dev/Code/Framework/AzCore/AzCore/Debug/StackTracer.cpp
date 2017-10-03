/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/
#ifndef AZ_UNITY_BUILD

#include <AzCore/Debug/StackTracer.h>

#if defined(AZ_PLATFORM_WINDOWS)
#   include <AzCore/Debug/StackTracerWinCpp.inl>
#elif defined(AZ_PLATFORM_LINUX) || defined(AZ_PLATFORM_APPLE)
#   include <AzCore/Debug/StackTracerLinux.inl>
#else // DEFAULT NOT STACK TRACE SUPPORT
#if defined(AZ_PLATFORM_XBONE) || defined(AZ_PLATFORM_PS4) || defined(AZ_PLATFORM_ANDROID)
/* NOTE: Android has not official support, but there are solutions for different versions like
https://android.googlesource.com/platform/system/core/+/kitkat-dev/libutils/CallStack.cpp for KitKat
Add an implementation when we establish the supported versions!*/
    #pragma message("--- No stack trace support ---")
    #pragma message("--- Implement it ---")
#endif
// By default callstack tracing is not supported.
using namespace AZ;
using namespace AZ::Debug;

unsigned int    StackRecorder::Record(StackFrame*, unsigned int, unsigned int, void*) { return false; }

void            SymbolStorage::LoadModuleData(const void*, unsigned int)       {}
unsigned int    SymbolStorage::GetModuleInfoDataSize()  { return 0; }
void            SymbolStorage::StoreModuleInfoData(void*, unsigned int) {}
unsigned int    SymbolStorage::GetNumLoadedModules() { return 0; }
const           SymbolStorage::ModuleInfo*  SymbolStorage::GetModuleInfo(unsigned int) { return 0; }
void            SymbolStorage::RegisterModuleListeners() {}
void            SymbolStorage::UnregisterModuleListeners() {}
void            SymbolStorage::SetMapFilename(const char*) {}
const char*     SymbolStorage::GetMapFilename() { return 0; }
void            SymbolStorage::DecodeFrames(const StackFrame*, unsigned int, StackLine*) {}
#endif

#endif // #ifndef AZ_UNITY_BUILD

