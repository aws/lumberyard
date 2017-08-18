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
#include <AzCore/Memory/SystemAllocator.h>
#include <SceneAPI/SceneUI/SceneUIConfiguration.h>

namespace AZ
{
    namespace SceneAPI
    {
        namespace UI
        {
            class OverlayWidget;
            class ReportCard;

            // Qt Code gen for ReportWidget
            namespace Ui
            {
                class ReportWidget;
            }
            class ReportWidget : public QWidget
            {
                Q_OBJECT
            public:
                enum class Host : bool
                {
                    Center,
                    BreakoutWindow
                };
                
                AZ_CLASS_ALLOCATOR(ReportWidget, SystemAllocator, 0)

                SCENE_UI_API explicit ReportWidget(QWidget* parent);
                SCENE_UI_API ~ReportWidget();

                SCENE_UI_API void SetTaskDescription(const AZStd::string& taskDescription);
                SCENE_UI_API void AddReportCard(ReportCard* card);

                SCENE_UI_API void ReportToOverlay(OverlayWidget* overlay, Host host = Host::Center);

                // Utility functions to for quick single card reporting.
                SCENE_UI_API static void ReportAssert(const AZStd::string& title, const AZStd::string& message, OverlayWidget* overlay = nullptr);
                SCENE_UI_API static void ReportError(const AZStd::string& title, const AZStd::string& message, OverlayWidget* overlay = nullptr);
                SCENE_UI_API static void ReportWarning(const AZStd::string& title, const AZStd::string& message, OverlayWidget* overlay = nullptr);
                SCENE_UI_API static void ReportLog(const AZStd::string& title, const AZStd::string& message, OverlayWidget* overlay = nullptr);

            protected:
                SCENE_UI_API static void ReportSingleCard(ReportCard* card, const AZStd::string& title, const AZStd::string& message, OverlayWidget* overlay);

                QScopedPointer<Ui::ReportWidget> ui;
            };
        } // UI
    } // SceneAPI
} // AZ
