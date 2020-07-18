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
#pragma once

#include <GameStateSamples/GameStateOptionsMenu.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
namespace StarterGame
{
    ////////////////////////////////////////////////////////////////////////////////////////////////
    class StarterGameStateOptionsMenu : public GameStateSamples::GameStateOptionsMenu
    {
    public:
        ////////////////////////////////////////////////////////////////////////////////////////////
        // Allocator
        AZ_CLASS_ALLOCATOR(StarterGameStateOptionsMenu, AZ::SystemAllocator, 0);

        ////////////////////////////////////////////////////////////////////////////////////////////
        // Type Info
        AZ_RTTI(StarterGameStateOptionsMenu, "{5CB9056A-5EF4-4645-8435-FBB49ADC5A80}", GameStateOptionsMenu);

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Default constructor
        StarterGameStateOptionsMenu() = default;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Default destructor
        ~StarterGameStateOptionsMenu() override = default;

    protected:
        ////////////////////////////////////////////////////////////////////////////////////////////
        //! \ref GameStateSamples::GameStateOptionsMenu::LoadOptionsMenuCanvas
        void LoadOptionsMenuCanvas() override;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! \ref GameStateSamples::GameStateOptionsMenu::UnloadOptionsMenuCanvas
        void UnloadOptionsMenuCanvas() override;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! \ref GameStateSamples::GameStateOptionsMenu::GetOptionsMenuCanvasAssetPath
        const char* GetOptionsMenuCanvasAssetPath() override;
    };
} // namespace StarterGame
