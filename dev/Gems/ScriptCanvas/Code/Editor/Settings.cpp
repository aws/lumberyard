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

#include "precompiled.h"

#include "Settings.h"

#include <ScriptCanvas/Data/Data.h>

#include <Editor/View/Widgets/VariablePanel/GraphVariablesTableView.h>

namespace ScriptCanvasEditor
{
    namespace EditorSettings
    {
        void WindowSavedState::Reflect(AZ::ReflectContext* context)
        {
            AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context);
            if (serialize)
            {
                serialize->Class<WindowSavedState, AZ::UserSettings>()
                    ->Version(1)
                    ->Field("m_storedWindowState", &WindowSavedState::m_storedWindowState)
                    ->Field("m_windowGeometry", &WindowSavedState::m_windowGeometry)
                    ;
            }
        }

        void WindowSavedState::Init(const QByteArray& windowState, const QByteArray& windowGeometry)
        {
            m_storedWindowState.clear();

            m_windowState.assign((AZ::u8*)windowState.begin(), (AZ::u8*)windowState.end());
            m_windowGeometry.assign((AZ::u8*)windowGeometry.begin(), (AZ::u8*)windowGeometry.end());

            m_storedWindowState.assign((AZ::u8*)windowState.begin(), (AZ::u8*)windowState.end());
        }

        void WindowSavedState::Restore(QMainWindow* window)
        {
            AZ_Assert(window, "A valid window must be provided to restore its state.");

            QByteArray windowStateData((const char*)GetWindowState().data(), (int)GetWindowState().size());
            window->restoreState(windowStateData);
        }

        const AZStd::vector<AZ::u8>&  WindowSavedState::GetWindowState()
        {
            m_windowState.clear();
            m_windowState.assign(m_storedWindowState.begin(), m_storedWindowState.end());
            return m_windowState;
        }

        ///////////////////////////////
        // ScriptCanvasEditorSettings
        ///////////////////////////////

        bool ScriptCanvasEditorSettings::VersionConverter(AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& classElement)
        {
            if (classElement.GetVersion() <= 5)
            {
                AZ::Crc32 dragCouplingEnabledId = AZ_CRC("m_enableNodeDragCoupling", 0x3edd74aa);
                AZ::Crc32 dragCouplingTimeId = AZ_CRC("m_dragNodeCouplingTime", 0xe6f213ae);

                AZ::Crc32 dragSplicingEnabledId = AZ_CRC("m_enableNodeDragConnectionSplicing", 0x77957b8f);
                AZ::Crc32 dragSplicingTimeId = AZ_CRC("m_dragNodeConnectionSplicingTime", 0x3e3742fb);

                AZ::Crc32 dropSplicingEnabledId = AZ_CRC("m_enableNodeDropConnectionSplicing", 0x371180a9);
                AZ::Crc32 dropSplicingTimeId = AZ_CRC("m_dropNodeConnectionSplicingTime", 0xba85498e);

                ToggleableConfiguration dragCouplingConfiguration(false,1000);
                ToggleableConfiguration dragSplicingConfiguration(true, 1000);
                ToggleableConfiguration dropSplicingConfiguration(true, 1000);

                // Drag Coupling
                AZ::SerializeContext::DataElementNode* dataNode = classElement.FindSubElement(dragCouplingEnabledId);

                if (dataNode)
                {
                    dataNode->GetData(dragCouplingConfiguration.m_enabled);
                }

                classElement.RemoveElementByName(dragCouplingEnabledId);

                dataNode = classElement.FindSubElement(dragCouplingTimeId);

                if (dataNode)
                {
                    dataNode->GetData(dragCouplingConfiguration.m_timeMS);
                }

                classElement.RemoveElementByName(dragCouplingTimeId);

                // Drag Splicing
                dataNode = classElement.FindSubElement(dragSplicingEnabledId);

                if (dataNode)
                {
                    dataNode->GetData(dragSplicingConfiguration.m_timeMS);
                }

                classElement.RemoveElementByName(dragSplicingEnabledId);

                dataNode = classElement.FindSubElement(dragSplicingTimeId);

                if (dataNode)
                {
                    dataNode->GetData(dragSplicingConfiguration.m_timeMS);
                }

                classElement.RemoveElementByName(dragSplicingTimeId);

                // Drop Splicing
                dataNode = classElement.FindSubElement(dropSplicingEnabledId);

                if (dataNode)
                {
                    dataNode->GetData(dropSplicingConfiguration.m_timeMS);
                }

                classElement.RemoveElementByName(dropSplicingEnabledId);

                dataNode = classElement.FindSubElement(dropSplicingTimeId);

                if (dataNode)
                {
                    dataNode->GetData(dropSplicingConfiguration.m_timeMS);
                }

                classElement.RemoveElementByName(dropSplicingTimeId);

                classElement.AddElementWithData(context, "DragCouplingConfiguration", dragCouplingConfiguration);
                classElement.AddElementWithData(context, "DragSplicingConfiguration", dragSplicingConfiguration);
                classElement.AddElementWithData(context, "DropSplicingConfiguration", dropSplicingConfiguration);
            }

            return true;
        }

        void ScriptCanvasEditorSettings::Reflect(AZ::ReflectContext* context)
        {
            AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context);
            if (serialize)
            {
                serialize->Class<ToggleableConfiguration>()
                    ->Version(1)
                    ->Field("Enabled", &ToggleableConfiguration::m_enabled)
                    ->Field("TimeMS", &ToggleableConfiguration::m_timeMS)
                ;

                serialize->Class<ShakeToDespliceSettings>()
                    ->Version(1)
                    ->Field("Enabled", &ShakeToDespliceSettings::m_enabled)
                    ->Field("ShakeCount", &ShakeToDespliceSettings::m_shakeCount)
                    ->Field("ShakeLength", &ShakeToDespliceSettings::m_minimumShakeLengthPercent)
                    ->Field("DeadZone", &ShakeToDespliceSettings::m_deadZonePercent)
                    ->Field("ShakeTime", &ShakeToDespliceSettings::m_maximumShakeTimeMS)
                    ->Field("Straightness", &ShakeToDespliceSettings::m_straightnessPercent)
                ;

                serialize->Class<EdgePanningSettings>()
                    ->Version(1)
                    ->Field("EdgePercent", &EdgePanningSettings::m_edgeScrollPercent)
                    ->Field("ScrollSpeed", &EdgePanningSettings::m_edgeScrollSpeed)
                ;

                serialize->Class<ScriptCanvasEditorSettings, AZ::UserSettings>()
                    ->Version(10, ScriptCanvasEditorSettings::VersionConverter)
                    ->Field("m_showPreviewMessage", &ScriptCanvasEditorSettings::m_showPreviewMessage)
                    ->Field("m_snapDistance", &ScriptCanvasEditorSettings::m_snapDistance)
                    ->Field("m_showExcludedNodes", &ScriptCanvasEditorSettings::m_showExcludedNodes)
                    ->Field("m_pinnedDataTypes", &ScriptCanvasEditorSettings::m_pinnedDataTypes)
                    ->Field("m_allowBookmarkViewpointControl", &ScriptCanvasEditorSettings::m_allowBookmarkViewpointControl)
                    ->Field("DragCouplingConfiguration", &ScriptCanvasEditorSettings::m_dragNodeCouplingConfig)
                    ->Field("DragSplicingConfiguration", &ScriptCanvasEditorSettings::m_dragNodeSplicingConfig)
                    ->Field("DropSplicingConfiguration", &ScriptCanvasEditorSettings::m_dropNodeSplicingConfig)
                    ->Field("AutoSaveConfiguration", &ScriptCanvasEditorSettings::m_autoSaveConfig)
                    ->Field("ShakeToDespliceConfiguration", &ScriptCanvasEditorSettings::m_shakeDespliceConfig)
                    ->Field("VariableColumnSorting", &ScriptCanvasEditorSettings::m_variablePanelSorting)
                    ->Field("ShowWarnings", &ScriptCanvasEditorSettings::m_showValidationWarnings)
                    ->Field("ShowErrors", &ScriptCanvasEditorSettings::m_showValidationErrors)
                    ->Field("AllowNodeNudging", &ScriptCanvasEditorSettings::m_allowNodeNudgingOnSplice)
                    ->Field("AlignmentTime", &ScriptCanvasEditorSettings::m_alignmentTimeMS)
                    ->Field("EdgePanningSettings", &ScriptCanvasEditorSettings::m_edgePanningSettings)
                    ;

                AZ::EditContext* editContext = serialize->GetEditContext();
                if (editContext)
                {
                    editContext->Class<ToggleableConfiguration>("Configuration", "A pair of configuration values for actions that can be enabled/disabled and occur after a certain amount of time.")
                        ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                            ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ->DataElement(AZ::Edit::UIHandlers::Default, &ToggleableConfiguration::m_enabled, "Enabled", "Controls whether or not the action is Enabled.")
                        ->DataElement(AZ::Edit::UIHandlers::Default, &ToggleableConfiguration::m_timeMS, "Time MS", "Controls how long until the action takes place.")
                            ->Attribute(AZ::Edit::Attributes::Suffix, "ms")
                            ->Attribute(AZ::Edit::Attributes::Min, 1)
                        ;

                    editContext->Class<ShakeToDespliceSettings>("Shake To Desplice", "Settings that control various parameters of the shake to desplice feature")
                        ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                            ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ->DataElement(AZ::Edit::UIHandlers::Default, &ShakeToDespliceSettings::m_enabled, "Enabled", "Controls whether or not this feature is enabled")
                        ->DataElement(AZ::Edit::UIHandlers::Default, &ShakeToDespliceSettings::m_shakeCount, "ShakeCount", "Controls the number of shakes that must occur in order to trigger the splice")
                        ->DataElement(AZ::Edit::UIHandlers::Slider, &ShakeToDespliceSettings::m_minimumShakeLengthPercent, "ShakeLength", "Controls how long each motion must be in order to be registered as a shake.")
                            ->Attribute(AZ::Edit::Attributes::Min, 0.0)
                            ->Attribute(AZ::Edit::Attributes::Max, 100.0)
                            ->Attribute(AZ::Edit::Attributes::Step, 1.0)
                        ->DataElement(AZ::Edit::UIHandlers::Slider, &ShakeToDespliceSettings::m_deadZonePercent, "DeadZone", "Controls how far the cursor must move before a check is initiated.")
                            ->Attribute(AZ::Edit::Attributes::Min, 0.0)
                            ->Attribute(AZ::Edit::Attributes::Max, 100.0)
                            ->Attribute(AZ::Edit::Attributes::Step, 1.0)
                        ->DataElement(AZ::Edit::UIHandlers::Default, &ShakeToDespliceSettings::m_maximumShakeTimeMS, "Maximmum Shake Time", "Sets a cap on how long it consider a series of actions as a single shake gesture")
                            ->Attribute(AZ::Edit::Attributes::Suffix, "ms")
                            ->Attribute(AZ::Edit::Attributes::Min, 1)
                        ->DataElement(AZ::Edit::UIHandlers::Slider, &ShakeToDespliceSettings::m_straightnessPercent, "Straightness Percent", "Controls how aligned the individual motions must be in order to qualify as a shake")
                            ->Attribute(AZ::Edit::Attributes::Min, 0.0)
                            ->Attribute(AZ::Edit::Attributes::Max, 100.0)
                            ->Attribute(AZ::Edit::Attributes::Step, 1.0)
                    ;

                    editContext->Class<EdgePanningSettings>("Edge Panning Settings", "Settings that control various parameters of the edge panning feature")
                        ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                            ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ->DataElement(AZ::Edit::UIHandlers::Slider, &EdgePanningSettings::m_edgeScrollPercent, "Percentage from Edge", "The percentage of the visible area to start scrolling when the mouse cursor is dragged into.")
                            ->Attribute(AZ::Edit::Attributes::Min, 0.0)
                            ->Attribute(AZ::Edit::Attributes::Max, 50.0)
                            ->Attribute(AZ::Edit::Attributes::Step, 1.0)
                        ->DataElement(AZ::Edit::UIHandlers::Default, &EdgePanningSettings::m_edgeScrollSpeed, "Scroll Speed", "How fast the scene will scroll when scrolling")
                            ->Attribute(AZ::Edit::Attributes::Min, 1.0)
                        ;

                    editContext->Class<ScriptCanvasEditorSettings>("Script Canvas Editor Preferences", "Preferences relating to the Script Canvas editor.")
                        ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                            ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                            ->Attribute(AZ::Edit::Attributes::Visibility, AZ_CRC("PropertyVisibility_ShowChildrenOnly", 0xef428f20))
                        ->DataElement(AZ::Edit::UIHandlers::Default, &ScriptCanvasEditorSettings::m_showPreviewMessage, "Show Preview Message", "Show the Script Canvas (PREVIEW) welcome message.")
                        ->DataElement(AZ::Edit::UIHandlers::Default, &ScriptCanvasEditorSettings::m_snapDistance, "Connection Snap Distance", "The distance from a slot under which connections will snap to it.")
                            ->Attribute(AZ::Edit::Attributes::Min, 10.0)
                        ->DataElement(AZ::Edit::UIHandlers::Default, &ScriptCanvasEditorSettings::m_showExcludedNodes, "Show nodes excluded from preview", "Show nodes that have been excluded from preview because they may not work correctly in Script Canvas yet.")
                        ->DataElement(AZ::Edit::UIHandlers::Default, &ScriptCanvasEditorSettings::m_allowBookmarkViewpointControl, "Bookmark Zooming", "Will cause the bookmarks to force the viewport into the state determined by the bookmark type\nBookmark Anchors - The viewport that exists when the bookmark is created.\nNode Groups - The area the Node Group covers")
                        ->DataElement(AZ::Edit::UIHandlers::Default, &ScriptCanvasEditorSettings::m_dragNodeCouplingConfig, "Node Coupling Configuration", "Controls for managing Node Coupling.\nNode Coupling is when you are dragging a node and leave it hovered over another Node, we will try to connect the sides you overlapped with each other.")
                        ->DataElement(AZ::Edit::UIHandlers::Default, &ScriptCanvasEditorSettings::m_dragNodeSplicingConfig, "Drag Node Splicing Configuration", "Controls for managing Node Splicing on a Drag.\nNode Splicing on a Drag will let you drag a node onto a connection, and splice that node onto the specified connection.")
                        ->DataElement(AZ::Edit::UIHandlers::Default, &ScriptCanvasEditorSettings::m_dropNodeSplicingConfig, "Drop Node Splicing Configuration", "Controls for managing Node Splicing on a Drag.\nNode Splicing on a drop will let you drop a node onto a connection from the Node Palette, and splice that node onto the specified connection.")
                        ->DataElement(AZ::Edit::UIHandlers::Default, &ScriptCanvasEditorSettings::m_autoSaveConfig, "AutoSave Configuration", "Controls for managing Auto Saving.\nAuto Saving will occur after the specified time of inactivity on a graph.")
                        ->DataElement(AZ::Edit::UIHandlers::Default, &ScriptCanvasEditorSettings::m_shakeDespliceConfig, "Shake To Desplice", "Settings that controls various parameters of the Shake to Desplice feature")
                        ->DataElement(AZ::Edit::UIHandlers::Default, &ScriptCanvasEditorSettings::m_allowNodeNudgingOnSplice, "Allow Node Nudging On Splice", "Controls whether or not nodes that are spliced onto connections will nudge other nodes out of the way to make room for the spliced node.")
                        ->DataElement(AZ::Edit::UIHandlers::Default, &ScriptCanvasEditorSettings::m_alignmentTimeMS, "Alignment Time", "Controls the amount of time nodes will take to slide into place when performing alignment commands")
                            ->Attribute(AZ::Edit::Attributes::Min, 0.0)
                        ->DataElement(AZ::Edit::UIHandlers::Default, &ScriptCanvasEditorSettings::m_edgePanningSettings, "Edge Panning Settings", "Settings that control how the panning at the edge of the scene will be handled.")
                        ;
                }
            }
        }
        
        ScriptCanvasEditorSettings::ScriptCanvasEditorSettings()
            : m_snapDistance(10.0)
            , m_showPreviewMessage(true)
            , m_showExcludedNodes(true)
            , m_allowBookmarkViewpointControl(true)
            , m_allowNodeNudgingOnSplice(true)
            , m_dragNodeCouplingConfig(true, 750)
            , m_dragNodeSplicingConfig(true, 1000)
            , m_dropNodeSplicingConfig(true, 1000)
            , m_autoSaveConfig(false, 2000)
            , m_pinnedDataTypes({
                ScriptCanvas::Data::ToAZType(ScriptCanvas::Data::Type::Number()),
                ScriptCanvas::Data::ToAZType(ScriptCanvas::Data::Type::Boolean()),
                ScriptCanvas::Data::ToAZType(ScriptCanvas::Data::Type::String()),
                ScriptCanvas::Data::ToAZType(ScriptCanvas::Data::Type::Color()),
                ScriptCanvas::Data::ToAZType(ScriptCanvas::Data::Type::EntityID()),
                ScriptCanvas::Data::ToAZType(ScriptCanvas::Data::Type::Transform()),
                ScriptCanvas::Data::ToAZType(ScriptCanvas::Data::Type::Vector2()),
                ScriptCanvas::Data::ToAZType(ScriptCanvas::Data::Type::Vector3()),
                ScriptCanvas::Data::ToAZType(ScriptCanvas::Data::Type::Vector4())
            })
            , m_variablePanelSorting(GraphVariablesModel::Name)
            , m_showValidationWarnings(true)
            , m_showValidationErrors(true)
            , m_alignmentTimeMS(250)
        {

        }
    }
}