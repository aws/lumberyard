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

#pragma once

#include <IAudioSystem.h>

#if defined(AZ_PLATFORM_WINDOWS) || defined(AZ_PLATFORM_APPLE_OSX) || defined(SUPPORT_WINDOWS_DLL_DECLSPEC)
    #ifdef CRYSOUND_EXPORTS
        #define CRYSOUND_API AZ_DLL_EXPORT
    #else
        #define CRYSOUND_API AZ_DLL_IMPORT
    #endif
#else
    #define CRYSOUND_API
#endif

#ifdef AZ_PLATFORM_PROVO
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdllexport-hidden-visibility"
#endif

namespace Audio
{
    // Definitions can be found in ATLEntities.cpp

    ///////////////////////////////////////////////////////////////////////////////////////////////
    struct CRYSOUND_API SATLXmlTags
    {
        static const char* const sPlatform;

        static const char* const sRootNodeTag;
        static const char* const sTriggersNodeTag;
        static const char* const sRtpcsNodeTag;
        static const char* const sSwitchesNodeTag;
        static const char* const sPreloadsNodeTag;
        static const char* const sEnvironmentsNodeTag;

        static const char* const sATLTriggerTag;
        static const char* const sATLSwitchTag;
        static const char* const sATLRtpcTag;
        static const char* const sATLSwitchStateTag;
        static const char* const sATLEnvironmentTag;
        static const char* const sATLPlatformsTag;
        static const char* const sATLConfigGroupTag;

        static const char* const sATLTriggerRequestTag;
        static const char* const sATLSwitchRequestTag;
        static const char* const sATLRtpcRequestTag;
        static const char* const sATLPreloadRequestTag;
        static const char* const sATLEnvironmentRequestTag;
        static const char* const sATLValueTag;

        static const char* const sATLNameAttribute;
        static const char* const sATLInternalNameAttribute;
        static const char* const sATLTypeAttribute;
        static const char* const sATLConfigGroupAttribute;

        static const char* const sATLDataLoadType;
    };

    ///////////////////////////////////////////////////////////////////////////////////////////////
    struct CRYSOUND_API SATLInternalControlNames
    {
        static const char* const sLoseFocusName;
        static const char* const sGetFocusName;
        static const char* const sMuteAllName;
        static const char* const sUnmuteAllName;
        static const char* const sDoNothingName;
        static const char* const sObjectSpeedName;
        static const char* const sObstructionOcclusionCalcName;
        static const char* const sOOCIgnoreStateName;
        static const char* const sOOCSingleRayStateName;
        static const char* const sOOCMultiRayStateName;
        static const char* const sObjectVelocityTrackingName;
        static const char* const sOVTOnStateName;
        static const char* const sOVTOffStateName;
        static const char* const sGlobalPreloadRequestName;
    };

    ///////////////////////////////////////////////////////////////////////////////////////////////
    struct CRYSOUND_API SATLInternalControlIDs
    {
        static TAudioControlID nLoseFocusTriggerID;
        static TAudioControlID nGetFocusTriggerID;
        static TAudioControlID nMuteAllTriggerID;
        static TAudioControlID nUnmuteAllTriggerID;
        static TAudioControlID nDoNothingTriggerID;
        static TAudioControlID nObjectSpeedRtpcID;

        static TAudioControlID nObstructionOcclusionCalcSwitchID;
        static TAudioSwitchStateID nOOCStateIDs[eAOOCT_COUNT];

        static TAudioControlID nObjectVelocityTrackingSwitchID;
        static TAudioSwitchStateID nOVTOnStateID;
        static TAudioSwitchStateID nOVTOffStateID;

        static TAudioPreloadRequestID nGlobalPreloadRequestID;
    };

} // namespace Audio

#ifdef AZ_PLATFORM_PROVO
#pragma clang diagnostic pop
#endif
