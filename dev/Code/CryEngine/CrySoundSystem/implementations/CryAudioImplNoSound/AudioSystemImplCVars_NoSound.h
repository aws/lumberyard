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

namespace Audio
{
    ///////////////////////////////////////////////////////////////////////////////////////////////////
    class CAudioSystemImplCVars_nosound
    {
    public:
        CAudioSystemImplCVars_nosound();
        ~CAudioSystemImplCVars_nosound();

        CAudioSystemImplCVars_nosound(const CAudioSystemImplCVars_nosound&) = delete;           // Copy protection
        CAudioSystemImplCVars_nosound& operator=(const CAudioSystemImplCVars_nosound&) = delete; // Copy protection

        void RegisterVariables();
        void UnregisterVariables();

        int m_nPrimaryPoolSize;
    };

    extern CAudioSystemImplCVars_nosound g_audioImplCVars_nosound;
} // namespace Audio
