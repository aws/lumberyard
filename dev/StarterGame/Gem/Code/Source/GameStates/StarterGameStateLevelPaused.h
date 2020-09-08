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

#include <GameStateSamples/GameStateLevelPaused.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
namespace StarterGame
{
    ////////////////////////////////////////////////////////////////////////////////////////////////
    class StarterGameStateLevelPaused : public GameStateSamples::GameStateLevelPaused
    {
    public:
        ////////////////////////////////////////////////////////////////////////////////////////////
        // Allocator
        AZ_CLASS_ALLOCATOR(StarterGameStateLevelPaused, AZ::SystemAllocator, 0);

        ////////////////////////////////////////////////////////////////////////////////////////////
        // Type Info
        AZ_RTTI(StarterGameStateLevelPaused, "{8B71B7FB-3A42-4C54-8389-2EBB0763CF70}", GameStateLevelPaused);

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Default constructor
        StarterGameStateLevelPaused() = default;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Default destructor
        ~StarterGameStateLevelPaused() override = default;

    protected:
        ////////////////////////////////////////////////////////////////////////////////////////////
        //! \ref GameStateSamples::GameStateLevelPaused::LoadPauseMenuCanvas
        void LoadPauseMenuCanvas() override;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! \ref GameStateSamples::GameStateLevelPaused::UnloadPauseMenuCanvas
        void UnloadPauseMenuCanvas() override;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! \ref GameStateSamples::GameStateLevelPaused::GetPauseMenuCanvasAssetPath
        const char* GetPauseMenuCanvasAssetPath() override;
    };
} // namespace StarterGame
