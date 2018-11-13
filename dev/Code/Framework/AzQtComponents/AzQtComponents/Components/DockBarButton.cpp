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

#include <QEvent>
#include <QMouseEvent>
#include <QWidget>


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
        setStyleSheet("QPushButton { border: 0px; background: transparent; min-width: 0px;}");

        // Set our default icon and widget size based on the icon size
        QPixmap icon(pixmapForButton());
        setIcon(icon);
        setFixedSize(icon.size());

        // Enable mouse events even when no mouse buttons are held down (so we can change the button icon when hovered over)
        setMouseTracking(true);
        
        // Handle when our button is clicked
        QObject::connect(this, &QPushButton::clicked, this, &DockBarButton::handleButtonClick);

        // Our dock bar buttons only need click focus, they don't need to accept
        // focus by tabbing
        setFocusPolicy(Qt::ClickFocus);
    }

    /**
     * Handle widget enter events
     */
    void DockBarButton::enterEvent(QEvent*)
    {
        // Set the on hover icon when the mouse is over our button
        setIcon(pixmapForButton(false, true));
    }

    /**
     * Handle widget leave events
     */
    void DockBarButton::leaveEvent(QEvent*)
    {
        // Set the default icon when our button is no longer hovered over
        setIcon(pixmapForButton());
    }

    /**
     * Handle mouse press events
     */
    void DockBarButton::mousePressEvent(QMouseEvent* event)
    {
        // Set the pressed icon
        setIcon(pixmapForButton(true));

        // Allow the mouse press event to continue propogation
        QPushButton::mousePressEvent(event);
    }

    /**
     * Handle mouse release events
     */
    void DockBarButton::mouseReleaseEvent(QMouseEvent* event)
    {
        // Set the default icon when we our button is no longer pressed
        setIcon(pixmapForButton());

        // Allow the mouse release event to continue propogation
        QPushButton::mouseReleaseEvent(event);
    }

    /**
     * Return the appropriate icon for the specified button state
     */
    QPixmap DockBarButton::pixmapForButton(bool pressed, bool hovered) const
    {
        // Construct the icon path suffix from the button state and dark style flag
        QString suffix;
        
        if (isEnabled())
        {
            suffix = pressed ? QString("_press")
                : hovered ? QString("_hover")
                : QString("");
        }

        if (m_isDarkStyle)
        {
            suffix.append("_dark");
        }

        // Return the appropriate icon based on the button state and our button type
        switch (m_buttonType)
        {
        case DockBarButton::CloseButton:
            return QPixmap(QString(":/stylesheet/img/titlebar/titlebar_close%1.png").arg(suffix));
        case DockBarButton::MaximizeButton:
            return QPixmap(QString(":/stylesheet/img/titlebar/titlebar_maximize%1.png").arg(suffix));
        case DockBarButton::MinimizeButton:
            return QPixmap(QString(":/stylesheet/img/titlebar/titlebar_minimize%1.png").arg(suffix));
        case DockBarButton::DividerButton:
            break;
        }

        return {};
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