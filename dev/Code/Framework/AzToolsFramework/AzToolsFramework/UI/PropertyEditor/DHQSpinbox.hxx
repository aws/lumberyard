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

#include <AzCore/base.h>
#include <AzCore/Memory/SystemAllocator.h>

AZ_PUSH_DISABLE_WARNING(4251 4800, "-Wunknown-warning-option") // disable warnings spawned by QT
#include <QtWidgets/QSpinBox>
AZ_POP_DISABLE_WARNING

namespace AzToolsFramework
{
    class DHQSpinbox
        : public QSpinBox
    {
    public:
        AZ_CLASS_ALLOCATOR(DHQSpinbox, AZ::SystemAllocator, 0);

        explicit DHQSpinbox(QWidget* parent = nullptr);

        QSize minimumSizeHint() const override;

        bool event(QEvent* event) override;

        bool HasSelectedText() const;
        int LastValue() const;

    protected:
        QPoint m_lastMousePosition;
        bool m_mouseCaptured = false;

        // QSpinBox
        bool eventFilter(QObject* object, QEvent* event) override;
        void focusInEvent(QFocusEvent* event) override;
        void wheelEvent(QWheelEvent* event) override;

    private:
        int m_lastValue;
    };

    class DHQDoubleSpinbox
        : public QDoubleSpinBox
    {
        Q_OBJECT
    public:
        AZ_CLASS_ALLOCATOR(DHQDoubleSpinbox, AZ::SystemAllocator, 0);

        explicit DHQDoubleSpinbox(QWidget* parent = nullptr);

        QSize minimumSizeHint() const override;

        bool event(QEvent* event) override;

        // QDoubleSpinBox
        QString textFromValue(double value) const override;

        bool HasSelectedText() const;
        double LastValue() const;

        void SetDisplayDecimals(int precision);

    protected:
        // QDoubleSpinBox
        bool eventFilter(QObject* object, QEvent* event) override;
        void focusInEvent(QFocusEvent* event) override;
        void focusOutEvent(QFocusEvent* event) override;
        void wheelEvent(QWheelEvent* event) override;

        QPoint m_lastMousePosition;
        bool m_mouseCaptured = false;

    private Q_SLOTS:
        void UpdateToolTip(double value);

    private:
        QString StringValue(double value, bool truncated = false) const;

        QString m_lastSuffix;
        double m_lastValue;
        int m_displayDecimals;
    };

    inline int DHQSpinbox::LastValue() const
    {
        return m_lastValue;
    }

    inline double DHQDoubleSpinbox::LastValue() const
    {
        return m_lastValue;
    }
} // namespace AzToolsFramework
