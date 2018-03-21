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

#include <QWidget>
#include <QFrame>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/std/containers/vector.h>
#include <SceneAPI/SceneUI/SceneUIConfiguration.h>
#include <SceneAPI/SceneUI/CommonWidgets/OverlayWidget.h>

class QPushButton;

namespace AZ
{
    namespace SceneAPI
    {
        namespace UI
        {
            class OverlayWidget;
            // QT space
            namespace Ui
            {
                class OverlayWidgetLayer;
            }

            // Layer for the OverlayWidget. Do not use directly but use the functions in OverlayWidget.
            class OverlayWidgetLayer : public QFrame
            {
                struct Button
                {
                    OverlayWidgetButton::Callback m_callback;
                    OverlayWidgetButton::EnabledCheck m_enabledCheck;
                    QPushButton* m_button = { nullptr };
                    bool m_triggersPop = { false };

                    Button(QPushButton* button, OverlayWidgetButton::Callback callback, OverlayWidgetButton::EnabledCheck enabledCheck, bool triggersPop);
                };

                Q_OBJECT
            public:
                AZ_CLASS_ALLOCATOR(OverlayWidgetLayer, SystemAllocator, 0)

                OverlayWidgetLayer(OverlayWidget* parent, QWidget* centerWidget, QWidget* breakoutWidget, 
                    const char* title, const OverlayWidgetButtonList& buttons);
                ~OverlayWidgetLayer() override;
                
                virtual void Refresh();
                virtual bool CanClose() const;
                virtual void PopLayer();

            protected:
                virtual void AddButtons(Ui::OverlayWidgetLayer& widget, const OverlayWidgetButtonList& buttons, bool hasBreakoutWindow);
                virtual void ButtonClicked(size_t index);
                
                bool eventFilter(QObject* object, QEvent* event) override;

                static const char* s_layerStyle;

                AZStd::vector<Button> m_buttons;
                QScopedPointer<Ui::OverlayWidgetLayer> m_ui;
                OverlayWidget* m_parent;
                QDialog* m_breakoutDialog;
                int m_breakoutCloseButtonIndex;
            };
        } // namespace UI
    } // namespace SceneAPI
} // namespace AZ