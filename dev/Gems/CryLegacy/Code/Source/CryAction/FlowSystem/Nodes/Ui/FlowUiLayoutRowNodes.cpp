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
#include <LyShine/Bus/UiLayoutRowBus.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
// Get/Set flow nodes
////////////////////////////////////////////////////////////////////////////////////////////////////

UI_FLOW_NODE__GET_AND_SET_PADDING_AS_INTS(LayoutRow, Padding);

UI_FLOW_NODE__GET_AND_SET_FLOAT(LayoutRow, Spacing,
    "Get the spacing (in pixels) between child elements",
    "Set the spacing (in pixels) between child elements",
    "Spacing", "The spacing (in pixels) between child elements of element [ElementID]");

UI_FLOW_NODE__GET_AND_SET_ENUM_AS_INT(LayoutRow, Order, UiLayoutInterface::HorizontalOrder,
    "Get the horizontal order for the layout",
    "Set the horizontal order for the layout",
    "Order", "LeftToRight=0, RightToLeft=1");