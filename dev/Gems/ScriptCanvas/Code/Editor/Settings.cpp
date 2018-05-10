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

        ////////////////////
        // PreviewSettings
        ////////////////////

        ScriptCanvasEditorSettings::ScriptCanvasEditorSettings()
            : m_snapDistance(10.0)
            , m_showPreviewMessage(true)
            , m_showExcludedNodes(false)
            , m_allowBookmarkViewpointControl(true)
            , m_enableNodeDragCoupling(false)
            , m_dragNodeCouplingTimeMS(1000)
            , m_enableNodeDragConnectionSplicing(true)
            , m_dragNodeConnectionSplicingTimeMS(1000)
            , m_enableNodeDropConnectionSplicing(true)
            , m_dropNodeConnectionSplicingTimeMS(1000)
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
        {

        }

    }
}