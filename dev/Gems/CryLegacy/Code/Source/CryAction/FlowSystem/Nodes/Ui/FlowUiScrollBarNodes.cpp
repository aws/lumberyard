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
#include <LyShine/Bus/UiScrollBarBus.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
// Get/Set flow nodes
////////////////////////////////////////////////////////////////////////////////////////////////////

UI_FLOW_NODE__GET_AND_SET_FLOAT(ScrollBar, HandleSize,
    "Get the size of the handle (0 - 1)",
    "Set the size of the handle (0 - 1)",
    "HandleSize", "The size of the handle (0 - 1) of element [ElementID]");

UI_FLOW_NODE__GET_AND_SET_FLOAT(ScrollBar, MinHandlePixelSize,
    "Get the minimum size of the handle in pixels",
    "Set the minimum size of the handle in pixels",
    "MinHandleSize", "The minimum size in pixels of the handle of element [ElementID]");

UI_FLOW_NODE__GET_AND_SET_ENTITY_ID_AS_ELEMENT_ID(ScrollBar, HandleEntity,
    "Get the handle element for the ScrollBar",
    "Set the handle element for the ScrollBar",
    "Handle", "The handle element of the ScrollBar");

