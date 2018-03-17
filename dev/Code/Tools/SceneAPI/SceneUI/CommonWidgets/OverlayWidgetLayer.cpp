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

#include <QEvent>
#include <QPushButton>
#include <QMessageBox>
#include <AzCore/Casting/numeric_cast.h>
#include <CommonWidgets/ui_OverlayWidgetLayer.h>
#include <SceneAPI/SceneUI/CommonWidgets/OverlayWidgetLayer.h>

namespace AZ
{
    namespace SceneAPI
    {
        namespace UI
        {
            const char* OverlayWidgetLayer::s_layerStyle = "background-color:rgba(0, 0, 0, 179)";

            OverlayWidgetLayer::Button::Button(QPushButton* button, OverlayWidgetButton::Callback callback,
                OverlayWidgetButton::EnabledCheck enabledCheck, bool triggersPop)
                : m_button(button)
                , m_callback(callback)
                , m_enabledCheck(enabledCheck)
                , m_triggersPop(triggersPop)
            {
            }

            OverlayWidgetLayer::OverlayWidgetLayer(OverlayWidget* parent, QWidget* centerWidget, QWidget* breakoutWidget, 
                const char* title, const OverlayWidgetButtonList& buttons)
                : QFrame(parent)
                , m_parent(parent)
                , m_ui(new Ui::OverlayWidgetLayer())
                , m_breakoutDialog(nullptr)
                , m_breakoutCloseButtonIndex(-1)
            {
                // If there's no parent to dock the center widget in and there's no widget
                // assigned to breakout, put the center widget in the breakout window.
                if (!parent && !breakoutWidget)
                {
                    breakoutWidget = centerWidget;
                    centerWidget = nullptr;
                }
                
                if (breakoutWidget)
                {
                    setStyleSheet(s_layerStyle);
                    setLayout(new QHBoxLayout());
                    if (centerWidget)
                    {
                        layout()->addWidget(centerWidget);
                        centerWidget->installEventFilter(this);
                    }

                    m_breakoutDialog = new QDialog(parent ? parent : QApplication::activeWindow());
                    connect(m_breakoutDialog, &QDialog::finished, this, &OverlayWidgetLayer::PopLayer);
                    m_ui->setupUi(m_breakoutDialog);
                    m_ui->m_centerLayout->addWidget(breakoutWidget);
                    m_breakoutDialog->installEventFilter(this);
                    breakoutWidget->installEventFilter(this);

                    AddButtons(*m_ui.data(), buttons, true);

                    m_breakoutDialog->setWindowTitle(title);
                    m_breakoutDialog->setMinimumSize(640, 480);
                    m_breakoutDialog->show();
                }
                else
                {
                    m_ui->setupUi(this);
                    if (centerWidget)
                    {
                        m_ui->m_centerLayout->addWidget(centerWidget);
                        centerWidget->installEventFilter(this);
                    }
                    else
                    {
                        setStyleSheet(s_layerStyle);
                    }
                    AddButtons(*m_ui.data(), buttons, parent == nullptr);
                }
            }

            OverlayWidgetLayer::~OverlayWidgetLayer() = default;

            void OverlayWidgetLayer::Refresh()
            {
                for (Button& button : m_buttons)
                {
                    if (button.m_button && button.m_enabledCheck)
                    {
                        button.m_button->setEnabled(button.m_enabledCheck());
                    }
                }
            }

            void OverlayWidgetLayer::PopLayer()
            {
                // Can't use scope signal blocker here as signals do need to be set to child widgets.
                if (m_breakoutDialog)
                {
                    m_breakoutDialog->removeEventFilter(this);
                    m_breakoutDialog->close();
                }

                if (m_parent)
                {
                    m_parent->PopLayer(this);
                }
            }

            bool OverlayWidgetLayer::CanClose() const
            {
                if (m_breakoutCloseButtonIndex >= 0)
                {
                    const Button& button = m_buttons[m_breakoutCloseButtonIndex];
                    if (button.m_enabledCheck)
                    {
                        return button.m_enabledCheck();
                    }
                }
                return true;
            }

            bool OverlayWidgetLayer::eventFilter(QObject* object, QEvent* event)
            {
                if (event->type() == QEvent::Close)
                {
                    // There's a breakout window with close button
                    if (CanClose())
                    {
                        if (m_breakoutCloseButtonIndex >= 0)
                        {
                            Button& button = m_buttons[m_breakoutCloseButtonIndex];
                            if (button.m_callback)
                            {
                                button.m_callback();
                            }
                        }

                        PopLayer();
                        return true;
                    }
                    else
                    {
                        QMessageBox::critical(m_breakoutDialog, "Unable to close",
                            "Closing this window is currently not possible.", QMessageBox::Ok, QMessageBox::Ok);
                        event->ignore();
                        return true;
                    }
                }
                return QWidget::eventFilter(object, event);
            }

            void OverlayWidgetLayer::AddButtons(Ui::OverlayWidgetLayer& widget, const OverlayWidgetButtonList& buttons, bool hasBreakoutWindow)
            {
                bool hasButtons = false;
                for (const OverlayWidgetButton* info : buttons)
                {
                    if (info->m_isCloseButton && hasBreakoutWindow && m_breakoutCloseButtonIndex == -1)
                    {
                        m_breakoutCloseButtonIndex = aznumeric_caster(m_buttons.size());
                        m_buttons.emplace_back(nullptr, info->m_callback, info->m_enabledCheck, true);
                    }
                    
                    QPushButton* button = new QPushButton(info->m_text.c_str());
                    size_t index = m_buttons.size();
                    m_buttons.emplace_back(button, info->m_callback, info->m_enabledCheck, info->m_triggersPop);
                    connect(button, &QPushButton::clicked, [this, index]()
                    {
                        ButtonClicked(index);
                    });
                    widget.m_controlsLayout->addWidget(button);
                    if (info->m_enabledCheck)
                    {
                        button->setEnabled(info->m_enabledCheck());
                    }
                    hasButtons = true;
                }
                m_ui->m_controls->setVisible(hasButtons);
            }

            void OverlayWidgetLayer::ButtonClicked(size_t index)
            {
                AZ_Assert(index < m_buttons.size(), "Invalid index for button in overlay layer.");

                Button& button = m_buttons[index];
                if (button.m_enabledCheck)
                {
                    if (!button.m_enabledCheck())
                    {
                        return;
                    }
                }

                if (button.m_callback)
                {
                    button.m_callback();
                }

                if (button.m_triggersPop)
                {
                    PopLayer();
                }
            }
        } // namespace UI
    } // namespace SceneAPI
} // namespace AZ

#include <CommonWidgets/OverlayWidgetLayer.moc>