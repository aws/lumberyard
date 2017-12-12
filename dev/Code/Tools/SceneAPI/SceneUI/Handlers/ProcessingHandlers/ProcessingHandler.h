#pragma once

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

#include <QObject>
#include <AzCore/std/string/string.h>
#include <SceneAPI/SceneUI/SceneUIConfiguration.h>

namespace AzToolsFramework
{
    namespace Logging
    {
        class LogEntry;
    }
}

namespace AZ
{
    namespace SceneAPI
    {
        namespace SceneUI
        {
            class SCENE_UI_API ProcessingHandler : public QObject
            {
                Q_OBJECT
            public:
                explicit ProcessingHandler(QObject* parent = nullptr);
                ~ProcessingHandler() override = default;
                virtual void BeginProcessing() = 0;

            signals:
                void AddInfo(const AZStd::string& infoMessage);
                void AddWarning(const AZStd::string& warningMessage);
                void AddError(const AZStd::string& errorMessage);
                void AddAssert(const AZStd::string& assertMessage);
                void AddLogEntry(const AzToolsFramework::Logging::LogEntry& entry);

                void UserMessageUpdated(const AZStd::string& userMessage);
                void SubtextUpdated(const AZStd::string& userMessage);

                void ProcessingComplete();
            };
        }
    }
}