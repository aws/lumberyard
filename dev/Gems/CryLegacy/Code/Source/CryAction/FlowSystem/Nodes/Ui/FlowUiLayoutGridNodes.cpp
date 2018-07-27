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
#include <LyShine/Bus/UiLayoutGridBus.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
// Get/Set flow nodes
////////////////////////////////////////////////////////////////////////////////////////////////////

UI_FLOW_NODE__GET_AND_SET_PADDING_AS_INTS(LayoutGrid, Padding);

UI_FLOW_NODE__GET_AND_SET_VECTOR2_AS_FLOATS(LayoutGrid, Spacing,
    "Get the spacing (in pixels) between child elements of the layout",
    "Set the spacing (in pixels) between child elements of the layout",
    "HorizSpacing", "The horizontal spacing between child elements of element [ElementID]",
    "VertSpacing", "The vertical spacing between child elements of element [ElementID]");

UI_FLOW_NODE__GET_AND_SET_VECTOR2_AS_FLOATS(LayoutGrid, CellSize,
    "Get the size (in pixels) of a child element in the layout",
    "Set the size (in pixels) of a child element in the layout",
    "CellWidth", "The width (in pixels) of a child element of element [ElementID]",
    "CellHeight", "The height (in pixels) of a child element of element [ElementID]");

UI_FLOW_NODE__GET_AND_SET_ENUM_AS_INT(LayoutGrid, HorizontalOrder, UiLayoutInterface::HorizontalOrder,
    "Get the horizontal order for the layout",
    "Set the horizontal order for the layout",
    "Order", "LeftToRight=0, RightToLeft=1");

UI_FLOW_NODE__GET_AND_SET_ENUM_AS_INT(LayoutGrid, VerticalOrder, UiLayoutInterface::VerticalOrder,
    "Get the vertical order for the layout",
    "Set the vertical order for the layout",
    "Order", "TopToBottom=0, BottomToTop=1");

UI_FLOW_NODE__GET_AND_SET_ENUM_AS_INT(LayoutGrid, StartingDirection, UiLayoutGridInterface::StartingDirection,
    "Get the starting direction for the layout",
    "Set the starting direction for the layout",
    "Direction", "HorizontalOrder=0, VerticalOrder=1");