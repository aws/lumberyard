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

#include <AzGameFramework/Application/GameApplication.h>
#include <IGameFramework.h>
#include <ITimer.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
namespace LumberyardLauncher
{
    ////////////////////////////////////////////////////////////////////////////////////////////////
    void RunMainLoop(AzGameFramework::GameApplication& gameApplication)
    {
        // Ideally we'd just call GameApplication::RunMainLoop instead, but
        // we'd have to stop calling IGameFramework::PreUpdate / PostUpdate
        // directly, and instead have something subscribe to the TickBus in
        // order to call them, using order ComponentTickBus::TICK_FIRST - 1
        // and ComponentTickBus::TICK_LAST + 1 to ensure they get called at
        // the same time as they do now. Also, we'd need to pass a function
        // pointer to AzGameFramework::GameApplication::MainLoop that would
        // be used to call ITimer::GetFrameTime (unless we could also shift
        // our frame time to be managed by AzGameFramework::GameApplication
        // instead, which probably isn't going to happen anytime soon given
        // how many things depend on the ITimer interface).
        bool continueRunning = true;
        ISystem* system = gEnv ? gEnv->pSystem : nullptr;
        IGameFramework* gameFramework = (gEnv && gEnv->pGame) ? gEnv->pGame->GetIGameFramework() : nullptr;
        while (continueRunning)
        {
            // Pump the system event loop
            gameApplication.PumpSystemEventLoopUntilEmpty();

            // Update the AzFramework system tick bus
            gameApplication.TickSystem();

            // Pre-update CryEngine
            if (gameFramework)
            {
                // If the legacy game framework exists it updates CrySystem...
                continueRunning = gameFramework->PreUpdate(true, 0);
            }
            else if (system)
            {
                // ...otherwise we need to update it here.
                system->UpdatePreTickBus();
            }

            // Update the AzFramework application tick bus
            gameApplication.Tick(gEnv->pTimer->GetFrameTime());

            // Post-update CryEngine
            if (gameFramework)
            {
                // If the legacy game framework exists it updates CrySystem...
                continueRunning = gameFramework->PostUpdate(true, 0) && continueRunning;
            }
            else if (system)
            {
                // ...otherwise we need to update it here.
                system->UpdatePostTickBus();
            }

            // Check for quit requests
            continueRunning = !gameApplication.WasExitMainLoopRequested() && continueRunning;
        }
    }
}
