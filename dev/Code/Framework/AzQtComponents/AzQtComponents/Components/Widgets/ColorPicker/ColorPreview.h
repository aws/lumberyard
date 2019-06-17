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
#include <AzCore/Math/Color.h>
#include <QFrame>

namespace AzQtComponents
{
    class Swatch;

    class AZ_QT_COMPONENTS_API ColorPreview
        : public QFrame
    {
        Q_OBJECT
        Q_PROPERTY(AZ::Color currentColor READ currentColor WRITE setCurrentColor)
        Q_PROPERTY(AZ::Color selectedColor READ selectedColor WRITE setSelectedColor)
        Q_PROPERTY(bool gammaEnabled READ isGammaEnabled WRITE setGammaEnabled)
        Q_PROPERTY(qreal gamma READ gamma WRITE setGamma);

    public:
        explicit ColorPreview(QWidget* parent = nullptr);
        ~ColorPreview() override;

        AZ::Color currentColor() const;
        AZ::Color selectedColor() const;
        bool isGammaEnabled() const;
        qreal gamma() const;

    public Q_SLOTS:
        void setCurrentColor(const AZ::Color& color);
        void setSelectedColor(const AZ::Color& color);
        void setGammaEnabled(bool enabled);
        void setGamma(qreal gamma);

    Q_SIGNALS:
        void colorSelected(const AZ::Color& color);
        void colorContextMenuRequested(const QPoint& pos, const AZ::Color& color);

    protected:
        QSize sizeHint() const override;
        void paintEvent(QPaintEvent* e) override;
        void mousePressEvent(QMouseEvent* e) override;
        void mouseReleaseEvent(QMouseEvent* e) override;
        void mouseMoveEvent(QMouseEvent* e) override;

    private:
        AZ::Color colorUnderPoint(const QPoint& p);

        AZ::Color m_currentColor;
        AZ::Color m_selectedColor;
        bool m_gammaEnabled;
        qreal m_gamma;
        QPoint m_dragStartPosition;
        Swatch* m_draggedSwatch;
    };
} // namespace AzQtComponents
