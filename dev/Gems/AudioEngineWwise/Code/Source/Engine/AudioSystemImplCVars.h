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
// Original file Copyright Crytek GMBH or its affiliates, used under license.

#pragma once

namespace Audio
{
    ///////////////////////////////////////////////////////////////////////////////////////////////////
    class CAudioWwiseImplCVars
    {
    public:
        CAudioWwiseImplCVars();
        ~CAudioWwiseImplCVars();

        CAudioWwiseImplCVars(const CAudioWwiseImplCVars&)= delete;              // Copy protection
        CAudioWwiseImplCVars& operator=(const CAudioWwiseImplCVars&) = delete; // Copy protection

        void RegisterVariables();
        void UnregisterVariables();

        int m_nPrimaryMemoryPoolSize;
        int m_nSecondaryMemoryPoolSize;
        int m_nPrepareEventMemoryPoolSize;
        int m_nStreamManagerMemoryPoolSize;
        int m_nStreamDeviceMemoryPoolSize;
        int m_nSoundEngineDefaultMemoryPoolSize;
        int m_nCommandQueueMemoryPoolSize;
        int m_nLowerEngineDefaultPoolSize;

#if defined(INCLUDE_WWISE_IMPL_PRODUCTION_CODE)
        int m_nEnableCommSystem;
        int m_nEnableOutputCapture;
        int m_nMonitorMemoryPoolSize;
        int m_nMonitorQueueMemoryPoolSize;
#endif // INCLUDE_WWISE_IMPL_PRODUCTION_CODE
    };

} // namespace Audio
