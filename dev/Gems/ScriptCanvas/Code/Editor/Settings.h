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

#include <AzCore/UserSettings/UserSettings.h>

#include <QByteArray>
#include <QMainWindow>

namespace AZ
{
    class ReflectContext;
}

namespace ScriptCanvasEditor
{
    namespace EditorSettings
    {
        class WindowSavedState
            : public AZ::UserSettings
        {
        public:
            AZ_RTTI(WindowSavedState, "{67DACC4D-B92C-4B5A-8884-6AF7C7B74246}", AZ::UserSettings);
            AZ_CLASS_ALLOCATOR(WindowSavedState, AZ::SystemAllocator, 0);

            WindowSavedState() = default;

            void Init(const QByteArray& windowState, const QByteArray& windowGeometry);
            void Restore(QMainWindow* window);

            static void Reflect(AZ::ReflectContext* context);

        private:

            const AZStd::vector<AZ::u8>& GetWindowState();

            AZStd::vector<AZ::u8> m_storedWindowState;
            AZStd::vector<AZ::u8> m_windowGeometry;
            AZStd::vector<AZ::u8> m_windowState;

        };

        class PreviewSettings
            : public AZ::UserSettings
        {
        public:
            AZ_RTTI(PreviewSettings, "{D8D5453C-BFB8-4C71-BBAF-0F10FDD69B3F}", AZ::UserSettings);
            AZ_CLASS_ALLOCATOR(PreviewSettings, AZ::SystemAllocator, 0);

            PreviewSettings()
                : m_showPreviewMessage(true)
                , m_snapDistance(10.0)
                , m_showExcludedNodes(false)
            {}

            bool m_showPreviewMessage;
            double m_snapDistance;
            bool m_showExcludedNodes; //! During preview we're excluding some behavior context nodes that may not work perfectly in Script Canvas.

            static void Reflect(AZ::ReflectContext* context)
            {
                AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context);
                if (serialize)
                {
                    serialize->Class<PreviewSettings, AZ::UserSettings>()
                        ->Version(3)
                        ->Field("m_showPreviewMessage", &PreviewSettings::m_showPreviewMessage)
                        ->Field("m_snapDistance", &PreviewSettings::m_snapDistance)
                        ->Field("m_showExcludedNodes", &PreviewSettings::m_showExcludedNodes)
                        ;

                    AZ::EditContext* editContext = serialize->GetEditContext();
                    if (editContext)
                    {
                        editContext->Class<PreviewSettings>("General Editor Preferences", "Preferences relating to the Script Canvas editor.")
                            ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                            ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                            ->Attribute(AZ::Edit::Attributes::Visibility, AZ_CRC("PropertyVisibility_ShowChildrenOnly", 0xef428f20))
                            ->DataElement(AZ::Edit::UIHandlers::Default, &PreviewSettings::m_showPreviewMessage, "Show Preview Message", "Show the Script Canvas (PREVIEW) welcome message.")
                            ->DataElement(AZ::Edit::UIHandlers::Default, &PreviewSettings::m_snapDistance, "Connection Snap Distance", "The distance from a slot under which connections will snap to it.")
                            ->Attribute(AZ::Edit::Attributes::Min, 10.0)
                        ->DataElement(AZ::Edit::UIHandlers::Default, &PreviewSettings::m_showExcludedNodes, "Show nodes excluded from preview", "Show nodes that have been excluded from preview because they may not work correctly in Script Canvas yet.")
                            ;
                    }
                }
            }

        };


    }
}