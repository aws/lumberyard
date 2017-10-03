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

#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <AzToolsFramework/UI/UICore/TargetSelectorButton.hxx>
#include <AzQtComponents/Components/StyledDockWidget.h>
#include <Debugger/Bus.h>

namespace Ui
{
    class Debugging;
}

namespace ScriptCanvasEditor
{
    namespace Widget
    {
        class Debugging 
            : public AzQtComponents::StyledDockWidget
            , ScriptCanvas::Debugger::NotificationBus::Handler
        {
            Q_OBJECT

        public:

            Debugging(QWidget* parent = nullptr);

        protected:

            enum class State
            {
                Detached,
                Attached
            };

            State m_state;

            void onConnectReleased();

            AZStd::unique_ptr<Ui::Debugging> ui;

            // ScriptCanvas::Debugger::NotificationBus::Handler
            void OnAttach(const AZ::EntityId&) override;
            void OnDetach(const AZ::EntityId&) override;

            //
        };
    }

}