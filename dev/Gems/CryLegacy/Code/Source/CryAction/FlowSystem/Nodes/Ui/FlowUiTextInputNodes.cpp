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
#include <LyShine/Bus/UiTextInputBus.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
// Get/Set flow nodes
////////////////////////////////////////////////////////////////////////////////////////////////////

UI_FLOW_NODE__GET_AND_SET_COLOR_AS_VEC3_AND_FLOAT(TextInput, TextSelectionColor,
    "Get the color to be used for the text background when it is selected",
    "Set the color to be used for the text background when it is selected",
    "Color", "The R G B value (0 - 255) of the text selection of element [ElementID]",
    "Alpha", "The alpha value (0 - 255) of the text selection of element [ElementID]");

UI_FLOW_NODE__GET_AND_SET_COLOR_AS_VEC3_AND_FLOAT(TextInput, TextCursorColor,
    "Get the color to be used for the text cursor",
    "Set the color to be used for the text cursor",
    "Color", "The R G B value (0 - 255) of the cursor of element [ElementID]",
    "Alpha", "The alpha value (0 - 255) of the cursor of element [ElementID]");

UI_FLOW_NODE__GET_AND_SET_AZ_STRING(TextInput, Text,
    "Get the text string being displayed/edited by the element",
    "Set the text string being displayed/edited by the element",
    "Value", "The text string being displayed/edited by element [ElementID]");

UI_FLOW_NODE__GET_AND_SET_FLOAT(TextInput, CursorBlinkInterval,
    "Get the cursor blink interval of the text input",
    "Set the cursor blink interval of the text input",
    "CursorBlinkInterval", "The cursor blink in interval of element [ElementID]");

UI_FLOW_NODE__GET_AND_SET_INT(TextInput, MaxStringLength,
    "Get the maximum number of characters that can be entered",
    "Set the maximum number of characters that can be entered",
    "MaxStringLength", "The maximum number of characters that can be entered. A value of 0 means none allowed, -1 means no limit");

UI_FLOW_NODE__GET_AND_SET_AZ_STRING(TextInput, ChangeAction,
    "Get the action triggered when the text is changed",
    "Set the action triggered when the text is changed",
    "ChangeAction", "The action name");

UI_FLOW_NODE__GET_AND_SET_AZ_STRING(TextInput, EndEditAction,
    "Get the action triggered when the editing of text is finished",
    "Set the action triggered when the editing of text is finished",
    "EndEditAction", "The action name");

UI_FLOW_NODE__GET_AND_SET_AZ_STRING(TextInput, EnterAction,
    "Get the action triggered when enter is pressed",
    "Set the action triggered when enter is pressed",
    "EnterAction", "The action name");

UI_FLOW_NODE__GET_AND_SET_ENTITY_ID_AS_ELEMENT_ID(TextInput, TextEntity,
    "Get the text element",
    "Set the text element",
    "TextElement", "The text element");

UI_FLOW_NODE__GET_AND_SET_ENTITY_ID_AS_ELEMENT_ID(TextInput, PlaceHolderTextEntity,
    "Get the placeholder text element",
    "Set the placeholder text element",
    "PlaceHolderTextElement", "The placeholder text element");

UI_FLOW_NODE__GET_AND_SET_BOOL(TextInput, IsPasswordField,
    "Get whether the text input is configured as a password field",
    "Set whether the text input is configured as a password field",
    "IsPasswordField", "Whether element [ElementID] is configured as a password field");