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

#include <QObject>
#include <AzCore/Math/Color.h>
#include <QColor>

namespace AzQtComponents
{
    namespace Internal
    {
        /**
         * Internal controller class used by the ColorPicker dialog. It's not meant to
         * be used anywhere else, thus the lack of AZ_QT_COMPONENTS_API.
         */
        class ColorController : public QObject
        {
            Q_OBJECT
            Q_PROPERTY(AZ::Color color READ color WRITE setColor NOTIFY colorChanged)

        public:
            explicit ColorController(QObject* parent = nullptr);
            ~ColorController() override;

            const AZ::Color color() const;

            float red() const;
            float green() const;
            float blue() const;

            float hslHue() const;
            float hslSaturation() const;
            float lightness() const;

            float hsvHue() const;
            float hsvSaturation() const;
            float value() const;

            float alpha() const;

        Q_SIGNALS:
            void colorChanged(const AZ::Color& color);

            void redChanged(float red);
            void greenChanged(float green);
            void blueChanged(float blue);

            void hslHueChanged(float hue);
            void hslSaturationChanged(float saturation);
            void lightnessChanged(float lightness);

            void hsvHueChanged(float hue);
            void hsvSaturationChanged(float saturation);
            void valueChanged(float value);

            void alphaChanged(float alpha);

        public Q_SLOTS:
            void setColor(const AZ::Color& color);

            void setRed(float red);
            void setGreen(float green);
            void setBlue(float blue);

            void setHslHue(float hue);
            void setHslSaturation(float saturation);
            void setLightness(float saturation);

            void setHsvHue(float hue);
            void setHsvSaturation(float saturation);
            void setValue(float value);

            void setAlpha(float alpha);

        private:
            class ColorState;
            void emitRgbaChangedSignals(const ColorState& previousColor);
            void emitHslChangedSignals(const ColorState& previousColor);
            void emitHsvChangedSignals(const ColorState& previousColor);

            QScopedPointer<ColorState> m_state;
        };
    }
} // namespace AzQtComponents
