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
        Q_ENUM(WindowDecorationButton)

        explicit DockBarButton(DockBarButton::WindowDecorationButton buttonType, QWidget* parent = nullptr, bool darkStyle = false);

        DockBarButton::WindowDecorationButton buttonType() const { return m_buttonType; }

        /*
        * Expose the button type using a QT property so that test automation can read it
        */
        Q_PROPERTY(WindowDecorationButton buttonType MEMBER m_buttonType CONSTANT)

    Q_SIGNALS:
        void buttonPressed(const DockBarButton::WindowDecorationButton type);

    protected:
        void paintEvent(QPaintEvent* event) override;

    private:
        void handleButtonClick();
        const DockBarButton::WindowDecorationButton m_buttonType;
        bool m_isDarkStyle;
    };
} // namespace AzQtComponents