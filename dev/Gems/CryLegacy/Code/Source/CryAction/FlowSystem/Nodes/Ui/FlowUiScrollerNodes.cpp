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
#include <LyShine/Bus/UiScrollerBus.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
// Get/Set flow nodes
////////////////////////////////////////////////////////////////////////////////////////////////////

UI_FLOW_NODE__GET_AND_SET_FLOAT(Scroller, Value,
    "Get the value of the Scroller (0 - 1)",
    "Set the value of the Scroller (0 - 1)",
    "Value", "The value (0 - 1) of element [ElementID]");

UI_FLOW_NODE__GET_AND_SET_AZ_STRING(Scroller, ValueChangingActionName,
    "Get the action triggered while the value of the Scroller is changing",
    "Set the action triggered while the value of the Scroller is changing",
    "ChangingAction", "The action name");

UI_FLOW_NODE__GET_AND_SET_AZ_STRING(Scroller, ValueChangedActionName,
    "Get the action triggered when the value of the Scroller is done changing",
    "Set the action triggered when the value of the Scroller is done changing",
    "ChangedAction", "The action name");
