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
#include <LyShine/Bus/UiCheckboxBus.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
// Get/Set flow nodes
////////////////////////////////////////////////////////////////////////////////////////////////////

UI_FLOW_NODE__GET_AND_SET_BOOL(Checkbox, State,
    "Get the state of the checkbox",
    "Set the state of the checkbox",
    "State", "The checkbox state of element [ElementID]");

UI_FLOW_NODE__GET_AND_SET_AZ_STRING(Checkbox, TurnOnActionName,
    "Get the action triggered when the checkbox is turned on",
    "Set the action triggered when the checkbox is turned on",
    "TurnOnAction", "The action name");

UI_FLOW_NODE__GET_AND_SET_AZ_STRING(Checkbox, TurnOffActionName,
    "Get the action triggered when the checkbox is turned off",
    "Set the action triggered when the checkbox is turned off",
    "TurnOffAction", "The action name");

UI_FLOW_NODE__GET_AND_SET_AZ_STRING(Checkbox, ChangedActionName,
    "Get the action triggered when the checkbox value changed",
    "Set the action triggered when the checkbox value changed",
    "ChangedAction", "The action name");

UI_FLOW_NODE__GET_AND_SET_ENTITY_ID_AS_ELEMENT_ID(Checkbox, CheckedEntity,
    "Get the child element to show when the checkbox is in the on state",
    "Set the child element to show when the checkbox is in the on state",
    "CheckedElement", "The child element to show when the checkbox is in the on state");

UI_FLOW_NODE__GET_AND_SET_ENTITY_ID_AS_ELEMENT_ID(Checkbox, UncheckedEntity,
    "Get the child element to show when the checkbox is in the off state",
    "Set the child element to show when the checkbox is in the off state",
    "UncheckedElement", "The child element to show when the checkbox is in the off state");