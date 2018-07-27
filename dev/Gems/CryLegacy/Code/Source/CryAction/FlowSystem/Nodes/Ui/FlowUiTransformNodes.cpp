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
#include <LyShine/Bus/UiTransformBus.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
// Get/Set flow nodes
////////////////////////////////////////////////////////////////////////////////////////////////////

UI_FLOW_NODE__GET_AND_SET_FLOAT(Transform, ZRotation,
    "Get the z rotation",
    "Set the z rotation",
    "Value", "The z rotation of element [ElementID]");

UI_FLOW_NODE__GET_AND_SET_VECTOR2_AS_FLOATS(Transform, CanvasPosition,
    "Get the position of the element in canvas space",
    "Set the position of the element in canvas space",
    "XPosition", "The x position of the element in canvas space",
    "YPosition", "The y position of the element in canvas space");

UI_FLOW_NODE__GET_AND_SET_VECTOR2_AS_FLOATS(Transform, LocalPosition,
    "Get the relative position of the element from the center of the element's anchors",
    "Set the relative position of the element from the center of the element's anchors",
    "XPosition", "The relative x position of the element",
    "YPosition", "The relative y position of the element");

UI_FLOW_NODE__SET_VECTOR2_AS_FLOATS(Transform, MoveCanvasPositionBy,
    "Move the element in canvas space",
    "XOffset", "The x offset",
    "YOffset", "The y offset");

UI_FLOW_NODE__SET_VECTOR2_AS_FLOATS(Transform, MoveLocalPositionBy,
    "Move the element relative to the center of the element's anchors",
    "XOffset", "The x offset",
    "YOffset", "The y offset");