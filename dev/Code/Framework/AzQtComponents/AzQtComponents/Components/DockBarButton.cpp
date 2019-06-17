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

#include <AzQtComponents/Components/DockBarButton.h>
#include <AzQtComponents/Components/Style.h>

#include <QStyleOptionButton>
#include <QStylePainter>

namespace AzQtComponents
{
    /**
     * Create a dock bar button that can be shared between any kind of docking bars for common actions
     */
    DockBarButton::DockBarButton(DockBarButton::WindowDecorationButton buttonType, QWidget* parent, bool darkStyle)
        : QPushButton(parent)
        , m_buttonType(buttonType)
        , m_isDarkStyle(darkStyle)
    {
        switch (m_buttonType)
        {
            case DockBarButton::CloseButton:
                Style::addClass(this, QStringLiteral("close"));
                break;
            case DockBarButton::MaximizeButton:
                Style::addClass(this, QStringLiteral("maximize"));
                break;
            case DockBarButton::MinimizeButton:
                Style::addClass(this, QStringLiteral("minimize"));
                break;
            case DockBarButton::DividerButton:
                break;
        }

        if (m_isDarkStyle)
        {
            Style::addClass(this, QStringLiteral("dark"));
        }
        
        // Handle when our button is clicked
        QObject::connect(this, &QPushButton::clicked, this, &DockBarButton::handleButtonClick);

        // Our dock bar buttons only need click focus, they don't need to accept
        // focus by tabbing
        setFocusPolicy(Qt::ClickFocus);
    }

    void DockBarButton::paintEvent(QPaintEvent *)
    {
        QStylePainter p(this);
        QStyleOptionButton option;
        initStyleOption(&option);

        // Set the icon based on m_buttonType. This allows the icon to be changed in a QStyle, or in
        // a Qt Style Sheet by changing the titlebar-close-icon, titlebar-maximize-icon, and
        // titlebar-minimize-icon properties.
        // Used in combination with the close, maximize, minimize and dark classes set in the
        // constructor, and :hover and :pressed selectors available to buttons, we have full control
        // of the pixmap in the style sheet.
        switch (m_buttonType)
        {
            case DockBarButton::CloseButton:
                option.icon = style()->standardIcon(QStyle::SP_TitleBarCloseButton, &option, this);
                break;
            case DockBarButton::MaximizeButton:
                option.icon = style()->standardIcon(QStyle::SP_TitleBarMaxButton, &option, this);
                break;
            case DockBarButton::MinimizeButton:
                option.icon = style()->standardIcon(QStyle::SP_TitleBarMinButton, &option, this);
                break;
            default:
                break;
        }

        p.drawControl(QStyle::CE_PushButton, option);
    }

    /**
     * Handle our button clicks by emitting a signal with our button type
     */
    void DockBarButton::handleButtonClick()
    {
        if (!window())
        {
            return;
        }

        emit buttonPressed(m_buttonType);
    }

#include <Components/DockBarButton.moc>
} // namespace AzQtComponents