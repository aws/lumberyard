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

#define AZ_LOADSCREENCOMPONENT_ENABLED      (1)

#if AZ_LOADSCREENCOMPONENT_ENABLED

#include <AzCore/Component/ComponentBus.h>

class LoadScreenInterface
    : public AZ::ComponentBus
{
public:

    virtual ~LoadScreenInterface() {}

    //! Invoked when the load screen should be updated and rendered.
    virtual void UpdateAndRender() = 0;

    //! Invoked when the game load screen should become visible.
    virtual void GameStart() = 0;

    //! Invoked when the level load screen should become visible.
    virtual void LevelStart() = 0;

    //! Invoked when the load screen should be paused.
    virtual void Pause() = 0;

    //! Invoked when the load screen should be resumed.
    virtual void Resume() = 0;

    //! Invoked when the load screen should be stopped.
    virtual void Stop() = 0;

    //! Invoked to find out if loading screen is playing.
    virtual bool IsPlaying() = 0;
};

using LoadScreenBus = AZ::EBus<LoadScreenInterface>;

#endif // if AZ_LOADSCREENCOMPONENT_ENABLED
