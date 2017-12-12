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

#include <AzQtComponents/AzQtComponentsAPI.h>

#include <QWidget>
#include <QPixmap>
#include <QPushButton>

class QEvent;
class QMouseEvent;
class QWidget;

namespace AzQtComponents
{
    class AZ_QT_COMPONENTS_API DockBarButton
        : public QPushButton
    {
        Q_OBJECT
    public:
        enum WindowDecorationButton
        {
            CloseButton,
            MaximizeButton,
            MinimizeButton,
            DividerButton
        };

        explicit DockBarButton(DockBarButton::WindowDecorationButton buttonType, QWidget* parent = nullptr);

    protected:
        void enterEvent(QEvent*) override;
        void leaveEvent(QEvent*) override;
        void mousePressEvent(QMouseEvent* event) override;
        void mouseReleaseEvent(QMouseEvent* event) override;

    Q_SIGNALS:
        void buttonPressed(const DockBarButton::WindowDecorationButton type);

    private:
        QPixmap pixmapForButton(bool pressed = false, bool hovered = false) const;
        void handleButtonClick();
        const DockBarButton::WindowDecorationButton m_buttonType;
    };
} // namespace AzQtComponents