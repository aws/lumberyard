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

#include <IAudioConnection.h>
#include <IAudioSystemControl.h>

namespace AudioControls
{
    enum EWwiseControlTypes
    {
        eWCT_INVALID = 0,
        eWCT_WWISE_EVENT = BIT(0),
        eWCT_WWISE_RTPC = BIT(1),
        eWCT_WWISE_SWITCH = BIT(2),
        eWCT_WWISE_AUX_BUS = BIT(3),
        eWCT_WWISE_SOUND_BANK = BIT(4),
        eWCT_WWISE_GAME_STATE = BIT(5),
        eWCT_WWISE_SWITCH_GROUP = BIT(6),
        eWCT_WWISE_GAME_STATE_GROUP = BIT(7),
    };

    //-------------------------------------------------------------------------------------------//
    class IAudioSystemControl_wwise
        : public IAudioSystemControl
    {
    public:
        IAudioSystemControl_wwise() {}
        IAudioSystemControl_wwise(const string& name, CID id, TImplControlType type);
        ~IAudioSystemControl_wwise() override {}
    };
} // namespace AudioControls
