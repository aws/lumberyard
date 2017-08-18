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

#include <QWidget.h>
#include <AzCore/std/string/string.h>
#include <AzCore/std/string/string.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <SceneAPI/SceneUI/SceneUIConfiguration.h>

namespace AzToolsFramework
{
    namespace Debug
    {
        class TraceContextStackInterface;
    }
    namespace Logging
    {
        class LogEntry;
    }
}

namespace AZ
{
    namespace SceneAPI
    {
        namespace UI
        {
            // Qt Code gen for ReportCard
            namespace Ui
            {
                class ReportCard;
            }
            class ReportCard : public QWidget
            {
            public:
                enum class Type
                {
                    Log,
                    Warning,
                    Error,
                    Assert
                };

                Q_OBJECT
            public:
                AZ_CLASS_ALLOCATOR(ReportCard, SystemAllocator, 0)

                SCENE_UI_API explicit ReportCard(Type type);
                SCENE_UI_API explicit ReportCard(const AzToolsFramework::Logging::LogEntry& entry);

                SCENE_UI_API void SetType(Type type);
                SCENE_UI_API void SetMessage(const AZStd::string& message);
                SCENE_UI_API void SetContext(const AzToolsFramework::Debug::TraceContextStackInterface& stack, bool includeUuids = true);
                SCENE_UI_API void AddContextLine(const AZStd::string& key, const AZStd::string& value);
                SCENE_UI_API void SetTimeStamp(const AZStd::string& timeStamp);
                SCENE_UI_API void SetTimeStamp(const time_t& timeStamp);
                // Sets the time stamp based on milliseconds since epoch (Qt style).
                SCENE_UI_API void SetTimeStamp(u64 timeStamp);
                SCENE_UI_API void SetTopLineVisible(bool visible);

            protected:
                SCENE_UI_API void OnExpandOrCollapseContext(bool isExpanded);

                QScopedPointer<Ui::ReportCard> ui;
            };
        } // UI
    } // SceneAPI
} // AZ
