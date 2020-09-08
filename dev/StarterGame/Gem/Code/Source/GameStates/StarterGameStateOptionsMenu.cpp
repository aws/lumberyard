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

#include <GameStates/StarterGameStateOptionsMenu.h>

#include <LyShine/Bus/UiCheckboxBus.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
namespace StarterGame
{
    ////////////////////////////////////////////////////////////////////////////////////////////////
    void OnInvertYCheckBoxStateChanged(AZ::EntityId entityId, AZ::Vector2, bool state)
    {
        // ToDo: Change the value
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void StarterGameStateOptionsMenu::LoadOptionsMenuCanvas()
    {
        GameStateOptionsMenu::LoadOptionsMenuCanvas();

        // Setup the 'Invert Y' checkbox.
        AZ::EntityId invertYCheckBoxElementId;
        UiCanvasBus::EventResult(invertYCheckBoxElementId,
                                 m_optionsMenuCanvasEntityId,
                                 &UiCanvasInterface::FindElementEntityIdByName,
                                 "InvertYAxisCheckBox");
        UiCheckboxBus::Event(invertYCheckBoxElementId, &UiCheckboxInterface::SetStateChangeCallback, OnInvertYCheckBoxStateChanged);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void StarterGameStateOptionsMenu::UnloadOptionsMenuCanvas()
    {
        GameStateOptionsMenu::UnloadOptionsMenuCanvas();
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    const char* StarterGameStateOptionsMenu::GetOptionsMenuCanvasAssetPath()
    {
        return "@assets@/ui/optionsmenu/optionsmenu.uicanvas";
    }
}
