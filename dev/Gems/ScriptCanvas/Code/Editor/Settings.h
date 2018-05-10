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

#include <AzCore/Math/Uuid.h>
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

        class ScriptCanvasEditorSettings
            : public AZ::UserSettings
        {
        public:
            AZ_RTTI(ScriptCanvasEditorSettings, "{D8D5453C-BFB8-4C71-BBAF-0F10FDD69B3F}", AZ::UserSettings);
            AZ_CLASS_ALLOCATOR(ScriptCanvasEditorSettings, AZ::SystemAllocator, 0);

            ScriptCanvasEditorSettings();

            double m_snapDistance;

            bool m_showPreviewMessage;
            bool m_showExcludedNodes; //! During preview we're excluding some behavior context nodes that may not work perfectly in Script Canvas.

            bool m_allowBookmarkViewpointControl;

            bool m_enableNodeDragCoupling;
            int m_dragNodeCouplingTimeMS;

            bool m_enableNodeDragConnectionSplicing;
            int m_dragNodeConnectionSplicingTimeMS;

            bool m_enableNodeDropConnectionSplicing;
            int m_dropNodeConnectionSplicingTimeMS;

            AZStd::unordered_set<AZ::Uuid> m_pinnedDataTypes;

            static void Reflect(AZ::ReflectContext* context)
            {
                AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context);
                if (serialize)
                {
                    serialize->Class<ScriptCanvasEditorSettings, AZ::UserSettings>()
                        ->Version(5)
                        ->Field("m_showPreviewMessage", &ScriptCanvasEditorSettings::m_showPreviewMessage)
                        ->Field("m_snapDistance", &ScriptCanvasEditorSettings::m_snapDistance)
                        ->Field("m_showExcludedNodes", &ScriptCanvasEditorSettings::m_showExcludedNodes)
                        ->Field("m_pinnedDataTypes", &ScriptCanvasEditorSettings::m_pinnedDataTypes)
                        ->Field("m_allowBookmarkViewpointControl", &ScriptCanvasEditorSettings::m_allowBookmarkViewpointControl)
                        ->Field("m_enableNodeDragCoupling", &ScriptCanvasEditorSettings::m_enableNodeDragCoupling)
                        ->Field("m_dragNodeCouplingTime", &ScriptCanvasEditorSettings::m_dragNodeCouplingTimeMS)
                        ->Field("m_enableNodeDragConnectionSplicing", &ScriptCanvasEditorSettings::m_enableNodeDragConnectionSplicing)
                        ->Field("m_dragNodeConnectionSplicingTime", &ScriptCanvasEditorSettings::m_dragNodeConnectionSplicingTimeMS)
                        ->Field("m_enableNodeDropConnectionSplicing", &ScriptCanvasEditorSettings::m_enableNodeDropConnectionSplicing)
                        ->Field("m_dropNodeConnectionSplicingTime", &ScriptCanvasEditorSettings::m_dropNodeConnectionSplicingTimeMS)
                        ;

                    AZ::EditContext* editContext = serialize->GetEditContext();
                    if (editContext)
                    {
                        editContext->Class<ScriptCanvasEditorSettings>("Script Canvas Editor Preferences", "Preferences relating to the Script Canvas editor.")
                            ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                            ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                            ->Attribute(AZ::Edit::Attributes::Visibility, AZ_CRC("PropertyVisibility_ShowChildrenOnly", 0xef428f20))
                            ->DataElement(AZ::Edit::UIHandlers::Default, &ScriptCanvasEditorSettings::m_showPreviewMessage, "Show Preview Message", "Show the Script Canvas (PREVIEW) welcome message.")
                            ->DataElement(AZ::Edit::UIHandlers::Default, &ScriptCanvasEditorSettings::m_snapDistance, "Connection Snap Distance", "The distance from a slot under which connections will snap to it.")
                                ->Attribute(AZ::Edit::Attributes::Min, 10.0)
                            ->DataElement(AZ::Edit::UIHandlers::Default, &ScriptCanvasEditorSettings::m_showExcludedNodes, "Show nodes excluded from preview", "Show nodes that have been excluded from preview because they may not work correctly in Script Canvas yet.")
                            ->DataElement(AZ::Edit::UIHandlers::Default, &ScriptCanvasEditorSettings::m_allowBookmarkViewpointControl, "Allow Bookmarks Viewport Control", "Will cause the bookmarks to force the viewport into the state determined by the bookmark type")
                            ->DataElement(AZ::Edit::UIHandlers::Default, &ScriptCanvasEditorSettings::m_enableNodeDragCoupling, "Enable Node Coupling On Drag", "Controls whether or not Node's will attempt to create connections between\nthe side dragged over a node after being held for the specified amount of time.")
                            ->DataElement(AZ::Edit::UIHandlers::Default, &ScriptCanvasEditorSettings::m_dragNodeCouplingTimeMS, "Coupling Time", "The amount of time that must elapsed before nodes will try to couple with each other")
                            ->DataElement(AZ::Edit::UIHandlers::Default, &ScriptCanvasEditorSettings::m_enableNodeDragConnectionSplicing, "Enable Connection Splicing On Drag", "Controls whether or not a Node will attempt to splice itself\nonto the connection it is dragged onto after being held for the specified amount of time.")
                            ->DataElement(AZ::Edit::UIHandlers::Default, &ScriptCanvasEditorSettings::m_dragNodeConnectionSplicingTimeMS, "Drag Connection Splice Time", "The amount of time that must elapse before a node will attempt to splice when dragging")
                            ->DataElement(AZ::Edit::UIHandlers::Default, &ScriptCanvasEditorSettings::m_enableNodeDropConnectionSplicing, "Enable Connection Splicing On Drop", "Controls whether or not a Node will attempt to splice itself\nonto the connection it is dropped onto after being held there for the specified amount of time.")
                            ->DataElement(AZ::Edit::UIHandlers::Default, &ScriptCanvasEditorSettings::m_dropNodeConnectionSplicingTimeMS, "Drop Connection Splice Time", "The amount of time that must elapse before a node will attempt to splice when dropping")
                            ;
                    }
                }
            }
        private:
            bool CanModifyDragCouplingTime() const
            {
                return !m_enableNodeDragCoupling;
            }

            bool CanModifyDragSplicingTime() const
            {
                return !m_enableNodeDragConnectionSplicing;
            }

            bool CanModifyDropSplicingTime() const
            {
                return !m_enableNodeDropConnectionSplicing;
            }
        };


    }
}