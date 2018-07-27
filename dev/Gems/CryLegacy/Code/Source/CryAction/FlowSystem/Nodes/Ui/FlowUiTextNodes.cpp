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
#include <LyShine/Bus/UiTextBus.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
// Get/Set flow nodes
////////////////////////////////////////////////////////////////////////////////////////////////////

UI_FLOW_NODE__GET_AND_SET_AZ_STRING(Text, Text,
    "Get the text string being displayed by the element",
    "Set the text string being displayed by the element",
    "Value", "The text string being displayed by element [ElementID]");

UI_FLOW_NODE__GET_AND_SET_COLOR_AS_VEC3_AND_FLOAT(Text, Color,
    "Get the color to draw the text string",
    "Set the color to draw the text string",
    "Color", "The R G B value (0 - 255) of the text of element [ElementID]",
    "Alpha", "The alpha value (0 - 255) of the text of element [ElementID]");

UI_FLOW_NODE__GET_AND_SET_LYSHINE_PATH(Text, Font,
    "Get the pathname to the font",
    "Set the pathname to the font",
    "Font", "The pathname to the font used by element [ElementID]");

UI_FLOW_NODE__GET_AND_SET_FLOAT(Text, FontSize,
    "Get the size of the font in points",
    "Set the size of the font in points",
    "FontSize", "The font size of element [ElementID]");

UI_FLOW_NODE__GET_AND_SET_ENUM_AS_INT(Text, OverflowMode, UiTextInterface::OverflowMode,
    "Get the overflow behavior of the text",
    "Set the overflow behavior of the text",
    "OverflowMode", "OverflowText=0, ClipText=1");

UI_FLOW_NODE__GET_AND_SET_ENUM_AS_INT(Text, WrapText, UiTextInterface::WrapTextSetting,
    "Get whether text is wrapped",
    "Set whether text is wrapped",
    "WrapTextSetting", "NoWrap=0, Wrap=1");

UI_FLOW_NODE__GET_AND_SET_ALIGNMENT(Text, TextAlignment,
    "Get the text alignment of the element",
    "Set the text alignment of the element");
