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

#include <GameStateSamples/GameStateLevelRunning.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
namespace StarterGame
{
    ////////////////////////////////////////////////////////////////////////////////////////////////
    class StarterGameStateLevelRunning : public GameStateSamples::GameStateLevelRunning
    {
    public:
        ////////////////////////////////////////////////////////////////////////////////////////////
        // Allocator
        AZ_CLASS_ALLOCATOR(StarterGameStateLevelRunning, AZ::SystemAllocator, 0);

        ////////////////////////////////////////////////////////////////////////////////////////////
        // Type Info
        AZ_RTTI(StarterGameStateLevelRunning, "{3D4C2A35-6108-482C-B15C-D97C003193C0}", GameStateLevelRunning);

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Default constructor
        StarterGameStateLevelRunning() = default;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Default destructor
        ~StarterGameStateLevelRunning() override = default;

    protected:
        ////////////////////////////////////////////////////////////////////////////////////////////
        //! \ref GameState::GameState::OnPushed
        void OnPushed() override;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! \ref GameState::GameState::OnPopped
        void OnPopped() override;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! \ref GameState::GameState::OnUpdate
        void OnUpdate() override;

    private:
    };
} // namespace StarterGame
