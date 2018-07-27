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
#include <LyShine/Bus/UiScrollBoxBus.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
// Get/Set flow nodes
////////////////////////////////////////////////////////////////////////////////////////////////////

UI_FLOW_NODE__GET_AND_SET_VECTOR2_AS_FLOATS(ScrollBox, ScrollOffset,
    "Get the scroll offset of the ScrollBox",
    "Set the scroll offset of the ScrollBox",
    "HorizOffset", "The horizontal scroll offset of element [ElementID]",
    "VertOffset", "The vertical scroll offset of element [ElementID]");

UI_FLOW_NODE__GET_AND_SET_BOOL(ScrollBox, IsHorizontalScrollingEnabled,
    "Get whether the ScrollBox allows horizontal scrolling",
    "Set whether the ScrollBox allows horizontal scrolling",
    "Enabled", "Whether horizontal scrolling is enabled");

UI_FLOW_NODE__GET_AND_SET_BOOL(ScrollBox, IsVerticalScrollingEnabled,
    "Get whether the ScrollBox allows vertical scrolling",
    "Set whether the ScrollBox allows vertical scrolling",
    "Enabled", "Whether vertical scrolling is enabled");

UI_FLOW_NODE__GET_AND_SET_BOOL(ScrollBox, IsScrollingConstrained,
    "Get whether the ScrollBox restricts scrolling to the content area",
    "Set whether the ScrollBox restricts scrolling to the content area",
    "IsConstrained", "Whether scrolling is constrained");

UI_FLOW_NODE__GET_AND_SET_ENUM_AS_INT(ScrollBox, SnapMode, UiScrollBoxInterface::SnapMode,
    "Get the snap mode for the ScrollBox",
    "Set the snap mode for the ScollBox",
    "SnapMode", "None=0, Children=1, Grid=2");

UI_FLOW_NODE__GET_AND_SET_VECTOR2_AS_FLOATS(ScrollBox, SnapGrid,
    "Get the snapping grid of the ScrollBox",
    "Set the snapping grid of the ScrollBox",
    "HorizSpacing", "The horizontal grid spacing of element [ElementID]",
    "VertSpacing", "The vertical grid spacing of element [ElementID]");

UI_FLOW_NODE__GET_AND_SET_ENUM_AS_INT(ScrollBox, HorizontalScrollBarVisibility, UiScrollBoxInterface::ScrollBarVisibility,
    "Get the visibility behavior for the horizontal ScrollBar of the ScrollBox",
    "Set the visibility behavior for the horizontal ScrollBar of the ScrollBox",
    "ScrollBarVisibility", "AlwaysVisible=0, AutoHide=1, AutoHideAndResizeViewArea=2");

UI_FLOW_NODE__GET_AND_SET_ENUM_AS_INT(ScrollBox, VerticalScrollBarVisibility, UiScrollBoxInterface::ScrollBarVisibility,
    "Get the visibility behavior for the vertical ScrollBar of the ScrollBox",
    "Set the visibility behavior for the vertical ScrollBar of the ScrollBox",
    "ScrollBarVisibility", "AlwaysVisible=0, AutoHide=1, AutoHideAndResizeViewArea=2");

UI_FLOW_NODE__GET_AND_SET_AZ_STRING(ScrollBox, ScrollOffsetChangingActionName,
    "Get the action triggered while the ScrollBox is being dragged",
    "Set the action triggered while the ScrollBox is being dragged",
    "ChangingAction", "The action name");

UI_FLOW_NODE__GET_AND_SET_AZ_STRING(ScrollBox, ScrollOffsetChangedActionName,
    "Get the action triggered when the ScrollBox drag is completed",
    "Set the action triggered when the ScrollBox drag is completed",
    "ChangedAction", "The action name");

UI_FLOW_NODE__GET_AND_SET_ENTITY_ID_AS_ELEMENT_ID(ScrollBox, ContentEntity,
    "Get the content element for the ScrollBox",
    "Set the content element for the ScollBox",
    "Content", "The element that the ScrollBox scrolls");

UI_FLOW_NODE__GET_AND_SET_ENTITY_ID_AS_ELEMENT_ID(ScrollBox, HorizontalScrollBarEntity,
    "Get the horizontal ScrollBar element for the ScrollBox",
    "Set the horizontal ScrollBar element for the ScollBox",
    "HorizontalScrollBar", "The horizontal ScrollBar element of the ScrollBox");

UI_FLOW_NODE__GET_AND_SET_ENTITY_ID_AS_ELEMENT_ID(ScrollBox, VerticalScrollBarEntity,
    "Get the vertical ScrollBar element for the ScrollBox",
    "Set the vertical ScrollBar element for the ScollBox",
    "VerticalScrollBar", "The vertical ScrollBar element of the ScrollBox");

////////////////////////////////////////////////////////////////////////////////////////////////////
// Other flow nodes
////////////////////////////////////////////////////////////////////////////////////////////////////

UI_FLOW_NODE__GET_VECTOR2_AS_FLOATS(ScrollBox, GetNormalizedScrollValue,
    "Get the normalized scroll value from 0 - 1",
    "HorizScrollValue", "The horizontal scroll value from 0 - 1 of element [ElementID]",
    "VertScrollValue", "The vertical scroll value from 0 - 1 of element [ElementID]");

UI_FLOW_NODE__GET_BOOL(ScrollBox, HasHorizontalContentToScroll,
    "Get whether there is content to scroll horizontally",
    "HorizContentToScroll", "Whether there is content to scroll horizontally");

UI_FLOW_NODE__GET_BOOL(ScrollBox, HasVerticalContentToScroll,
    "Get whether there is content to scroll vertically",
    "VerticalContentToScroll", "Whether there is content to scroll vertically");

UI_FLOW_NODE__GET_ENTITY_ID_AS_ELEMENT_ID(ScrollBox, FindClosestContentChildElement,
    "Find the child of the content element that is closest to the content anchors",
    "ClosestElement", "The element currently closest to \"focused\"");
