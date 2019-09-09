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

#ifndef AZGAMEFRAMEWORK_GAMEAPPLICATION_H
#define AZGAMEFRAMEWORK_GAMEAPPLICATION_H

#include <AzCore/base.h>
#include <AzFramework/Application/Application.h>

#pragma once

namespace AzGameFramework
{
    class GameApplication
        : public AzFramework::Application
    {
    public:

        AZ_CLASS_ALLOCATOR(GameApplication, AZ::SystemAllocator, 0);

        GameApplication();
        ~GameApplication();

        AZ::ComponentTypeList GetRequiredSystemComponents() const override;
        void CreateStaticModules(AZStd::vector<AZ::Module*>& outModules) override;

        static void GetGameDescriptorPath(char(&outConfigFilename)[AZ_MAX_PATH_LEN], const char* gameName)
        {
            azstrcpy(outConfigFilename, AZ_MAX_PATH_LEN, gameName);
            azstrcat(outConfigFilename, AZ_MAX_PATH_LEN, "/Config/Game.xml");
            AZStd::to_lower(outConfigFilename, outConfigFilename + strlen(outConfigFilename));
        }

    protected:

        //////////////////////////////////////////////////////////////////////////
        // AzFramework::Application
        void StartCommon(AZ::Entity* systemEntity) override;
        //////////////////////////////////////////////////////////////////////////
    };
} // namespace AzGameFramework

#endif // AZGAMEFRAMEWORK_GAMEAPPLICATION_H
