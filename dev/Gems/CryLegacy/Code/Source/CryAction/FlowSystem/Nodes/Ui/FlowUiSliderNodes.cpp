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
#include <LyShine/Bus/UiSliderBus.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
// Get/Set flow nodes
////////////////////////////////////////////////////////////////////////////////////////////////////

UI_FLOW_NODE__GET_AND_SET_FLOAT(Slider, Value,
    "Get the value of the slider",
    "Set the value of the slider",
    "Value", "The slider value of element [ElementID]");

UI_FLOW_NODE__GET_AND_SET_FLOAT(Slider, MinValue,
    "Get the minimum value of the slider",
    "Set the minimum value of the slider",
    "MinValue", "The slider minimum value of element [ElementID]");

UI_FLOW_NODE__GET_AND_SET_FLOAT(Slider, MaxValue,
    "Get the maximum value of the slider",
    "Set the maximum value of the slider",
    "MaxValue", "The slider maximum value of element [ElementID]");

UI_FLOW_NODE__GET_AND_SET_FLOAT(Slider, StepValue,
    "Get the smallest increment allowed between values. Zero means no restriction.",
    "Set the smallest increment allowed between values. Use zero for no restriction.",
    "StepValue", "The smallest increment allowed between values of element [ElementID]. Zero means no restriction.");

UI_FLOW_NODE__GET_AND_SET_AZ_STRING(Slider, ValueChangingActionName,
    "Get the action triggered while the value is changing",
    "Set the action triggered while the value is changing",
    "ValueChangingAction", "The action name");

UI_FLOW_NODE__GET_AND_SET_AZ_STRING(Slider, ValueChangedActionName,
    "Get the action triggered when the value is done changing",
    "Set the action triggered when the value is done changing",
    "ValueChangedAction", "The action name");

UI_FLOW_NODE__GET_AND_SET_ENTITY_ID_AS_ELEMENT_ID(Slider, TrackEntity,
    "Get the track element",
    "Set the track element",
    "Track", "The track element");

UI_FLOW_NODE__GET_AND_SET_ENTITY_ID_AS_ELEMENT_ID(Slider, FillEntity,
    "Get the fill element",
    "Set the fill element",
    "FillElement", "The fill element");

UI_FLOW_NODE__GET_AND_SET_ENTITY_ID_AS_ELEMENT_ID(Slider, ManipulatorEntity,
    "Get the manipulator element",
    "Set the manipulator element",
    "ManipulatorElement", "The manipulator element");