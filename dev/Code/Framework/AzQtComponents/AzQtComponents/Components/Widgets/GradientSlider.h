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
#include <AzQtComponents/Components/Widgets/Slider.h>
#include <AzCore/std/functional.h>
#include <QSlider>

namespace AzQtComponents
{
    class Style;

    class AZ_QT_COMPONENTS_API GradientSlider
        : public QSlider
    {
        Q_OBJECT

    public:
        using ColorFunction = AZStd::function<QColor(qreal)>;

        explicit GradientSlider(Qt::Orientation orientation, QWidget* parent = nullptr);
        explicit GradientSlider(QWidget* parent = nullptr);
        ~GradientSlider() override;

        void setColorFunction(ColorFunction colorFunction);
        void updateGradient();

    protected:
        void resizeEvent(QResizeEvent* e) override;
        void keyPressEvent(QKeyEvent* e) override;

    private:
        friend class Slider;

        static const char* s_gradientSliderClass;

        // methods called by Slider
        static int sliderThickness(const Style* style, const QStyleOption* option, const QWidget* widget, const Slider::Config& config);
        static int sliderLength(const Style* style, const QStyleOption* option, const QWidget* widget, const Slider::Config& config);
        static QRect sliderHandleRect(const Style* style, const QStyleOptionSlider* option, const QWidget* widget, const Slider::Config& config);
        static QRect sliderGrooveRect(const Style* style, const QStyleOptionSlider* option, const QWidget* widget, const Slider::Config& config);
        static bool drawSlider(const Style* style, const QStyleOptionSlider* option, QPainter* painter, const QWidget* widget, const Slider::Config& config);

        // internal methods
        void initGradientColors();
        QBrush gradientBrush() const;

        ColorFunction m_colorFunction;
        QLinearGradient m_gradient;
    };
} // namespace AzQtComponents
