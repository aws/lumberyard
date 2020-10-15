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

#include <AzToolsFramework/Application/ToolsApplication.h>

namespace AZ
{
    namespace SerializeContextTools
    {
        class Application final
            : public AzToolsFramework::ToolsApplication
        {
        public:
            Application(int* argc, char*** argv);
            ~Application() override = default;

            void StartCommon(AZ::Entity* systemEntity) override;
            
            const char* GetGameRoot() const;
            const char* GetConfigFilePath() const;

        private:
            bool ReadGameFolderFromBootstrap(char* result) const;

            char m_gameRoot[AZ_MAX_PATH_LEN] = { 0 };
            char m_configFilePath[AZ_MAX_PATH_LEN] = { 0 };
        };
    } // namespace SerializeContextTools
} // namespace AZ