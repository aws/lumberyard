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
#include "CryLegacy_precompiled.h"
#include "UiFlow.h"
#include <LyShine/Bus/UiMaskBus.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
// Get/Set flow nodes
////////////////////////////////////////////////////////////////////////////////////////////////////

UI_FLOW_NODE__GET_AND_SET_BOOL(Mask, IsMaskingEnabled,
    "Get whether masking is enabled",
    "Set whether masking is enabled",
    "IsMaskingEnabled", "Whether masking is enabled");

UI_FLOW_NODE__GET_AND_SET_BOOL(Mask, DrawBehind,
    "Get whether mask is drawn behind the child elements",
    "Set whether mask is drawn behind the child elements",
    "DrawBehind", "Whether mask is drawn behind the child elements");

UI_FLOW_NODE__GET_AND_SET_BOOL(Mask, DrawInFront,
    "Get whether mask is drawn in front of child elements",
    "Set whether mask is drawn in front of child elements",
    "DrawInFront", "Whether mask is drawn in front of child elements");

UI_FLOW_NODE__GET_AND_SET_BOOL(Mask, UseAlphaTest,
    "Get whether to use the alpha channel in the mask visual's texture to define the mask",
    "Set whether to use the alpha channel in the mask visual's texture to define the mask",
    "UseAlphaTest", "Whether to use the alpha channel in the mask visual's texture to define the mask");