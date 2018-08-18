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

#include <AzQtComponents/Components/Widgets/ColorPicker/ColorController.h>
#include <AzQtComponents/Utilities/ColorUtilities.h>
#include <AzQtComponents/Utilities/Conversions.h>
#include <AzCore/Math/MathUtils.h>

namespace AzQtComponents
{
namespace Internal
{

    class ColorController::ColorState
    {
    public:
        ColorState()
            : m_rgb(AZ::Color::CreateZero())
        {
        }
        ~ColorState() = default;

        ColorState& operator=(const ColorState&) = default;


        const AZ::Color& rgb() const
        {
            return m_rgb;
        }

        float hue() const
        {
            // This is a holdover from the QColor origins of ColorController (QColor expects hue to be in [0, 1.0] but we store degrees)
            return m_hue / 360.0f;
        }

        float hslS() const
        {
            return m_hsl.saturation;
        }

        float hslL() const
        {
            return m_hsl.lightness;
        }

        float hsvS() const
        {
            return m_hsv.saturation;
        }

        float hsvV() const
        {
            return m_hsv.value;
        }

        void setRgb(const AZ::Color& color)
        {
            m_rgb = color;
            propagateRgb();
        }

        void setR(float r)
        {
            m_rgb.SetR(r);
            propagateRgb();
        }

        void setG(float g)
        {
            m_rgb.SetG(g);
            propagateRgb();
        }

        void setB(float b)
        {
            m_rgb.SetB(b);
            propagateRgb();
        }

        void setA(float a)
        {
            m_rgb.SetA(a);
        }

        void setHslH(float h)
        {
            // This is a holdover from the QColor origins of ColorController (QColor expects hue to be in [0, 1.0] but we store degrees)
            m_hue = h * 360.0f;
            propagateHsl();
        }

        void setHslS(float s)
        {
            m_hsl.saturation = s;
            propagateHsl();
        }

        void setHslL(float l)
        {
            m_hsl.lightness = l;
            propagateHsl();
        }

        void setHsvH(float h)
        {
            // This is a holdover from the QColor origins of ColorController (QColor expects hue to be in [0, 1.0] but we store degrees)
            m_hue = h * 360.0f;
            propagateHsv();
        }

        void setHsvS(float s)
        {
            m_hsv.saturation = s;
            propagateHsv();
        }

        void setHsvV(float v)
        {
            m_hsv.value = v;
            propagateHsv();
        }

    private:
        void propagateRgb()
        {
            // See https://en.wikipedia.org/wiki/HSL_and_HSV#General_approach through https://en.wikipedia.org/wiki/HSL_and_HSV#Lightness
            const float r = m_rgb.GetR();
            const float g = m_rgb.GetG();
            const float b = m_rgb.GetB();

            // Compute Hue and Chroma
            const float M = std::max({r, g, b});
            const float m = std::min({r, g, b});
            const float C = M - m;

            float Hprime = 0.0;
            if (qFuzzyCompare(0.0f, C))
            {
                Hprime = 0.0f;
            }
            else if (M == r)
            {
                Hprime = std::fmod((g - b) / C, 6.0f);
            }
            else if (M == g)
            {
                Hprime = (b - r) / C + 2.0f;
            }
            else if (M == b)
            {
                Hprime = (r - g) / C + 4.0f;
            }
            m_hue = 60.0f * Hprime;
            m_hue = AZ::GetClamp(m_hue, 0.0f, 360.0f);

            // Compute Lightness
            m_hsv.value = M;
            m_hsv.value = AZ::GetClamp(m_hsv.value, 0.0f, 1.0f);

            m_hsl.lightness = (M + m) / 2.0f;
            m_hsl.lightness = AZ::GetClamp(m_hsl.lightness, 0.0f, 12.5f);

            // Compute saturation
            if (qFuzzyCompare(0.0f, m_hsv.value))
            {
                m_hsv.saturation = 0.0f;
            }
            else
            {
                m_hsv.saturation = C / m_hsv.value;
            }
            m_hsv.saturation = AZ::GetClamp(m_hsv.saturation, 0.0f, 1.0f);

            if (qFuzzyCompare(0.0f, m_hsl.lightness))
            {
                m_hsl.saturation = 0.0f;
            }
            else if (m_hsl.lightness >= 1.0f)
            {
                m_hsl.saturation = 1.0f;
            }
            else
            {
                m_hsl.saturation = C / (1.0f - std::abs(m_hsl.lightness * 2.0f - 1.0f));
            }
            m_hsl.saturation = AZ::GetClamp(m_hsl.saturation, 0.0f, 1.0f);
        }

        void propagateToRgb(float C, float m)
        {
            // See https://en.wikipedia.org/wiki/HSL_and_HSV#Converting_to_RGB

            float Hprime = m_hue / 60.0f;
            float X = C * (1.0f - std::abs(std::fmod(Hprime, 2.0f) - 1.0f));
            if (Hprime <= 1.0f)
            {
                m_rgb.SetR(C + m);
                m_rgb.SetG(C + m);
                m_rgb.SetB(m);
            }
            else if (Hprime <= 2.0f)
            {
                m_rgb.SetR(X + m);
                m_rgb.SetG(C + m);
                m_rgb.SetB(m);
            }
            else if (Hprime <= 3.0)
            {
                m_rgb.SetR(m);
                m_rgb.SetG(C + m);
                m_rgb.SetB(X + m);
            }
            else if (Hprime <= 4.0f)
            {
                m_rgb.SetR(m);
                m_rgb.SetG(X + m);
                m_rgb.SetB(C + m);
            }
            else if (Hprime <= 5.0f)
            {
                m_rgb.SetR(X + m);
                m_rgb.SetG(m);
                m_rgb.SetB(C + m);
            }
            else if (Hprime < 6.0f)
            {
                m_rgb.SetR(C + m);
                m_rgb.SetG(m);
                m_rgb.SetB(X + m);
            }
        }

        void propagateHsl()
        {
            // See https://en.wikipedia.org/wiki/HSL_and_HSV#From_HSL
            float L = std::min(1.0f, m_hsl.lightness);
            float C = (1.0f - std::abs(2.0f * L - 1.0f)) * m_hsl.saturation;
            float m = L - (0.5f * C);
            propagateToRgb(C, m);


            // See http://home.kpn.nl/vanadovv/color/ColorMath.html#hdir (function HSV_HSL)
            float lightness = 2.0f * m_hsl.lightness;
            float saturation = m_hsl.saturation;
            if (lightness <= 1.0f)
            {
                saturation *= lightness;
            }
            else
            {
                saturation *= 2.0f - lightness;
            }
            float value = (lightness + saturation) / 2.0f;
            saturation = (2.0f * saturation) / (lightness + saturation);

            m_hsv.saturation = AZ::GetClamp(saturation, 0.0f, 1.0f);
            m_hsv.value = AZ::GetClamp(value, 0.0f, 12.5f);
        }

        void propagateHsv()
        {
            // See https://en.wikipedia.org/wiki/HSL_and_HSV#From_HSV
            float C = m_hsv.value * m_hsv.saturation;
            float m = m_hsv.value - C;
            propagateToRgb(C, m);

            // See http://home.kpn.nl/vanadovv/color/ColorMath.html#hdir (function HSL_HSV)
            float lightness = (2.0f - m_hsv.saturation) * m_hsv.value;
            float saturation =  m_hsv.saturation * m_hsv.value;
            if (lightness <= 1.0f)
            {
                saturation /= lightness;
            }
            else
            {
                saturation /= 2.0f - lightness;
            }
            lightness /= 2.0f;

            m_hsl.saturation = AZ::GetClamp(saturation, 0.0f, 1.0f);
            m_hsl.lightness = AZ::GetClamp(lightness, 0.0f, 12.5f);
        }

        AZ::Color m_rgb;

        float m_hue = {};
        struct {
            float saturation = {};
            float lightness = {};
        } m_hsl;
        struct {
            float saturation = {};
            float value = {};
        } m_hsv;
    };

ColorController::ColorController(QObject* parent)
    : QObject(parent)
    , m_state(new ColorState)
{
}

ColorController::~ColorController()
{
}

const AZ::Color ColorController::color() const
{
    return m_state->rgb();
}

float ColorController::red() const
{
    return m_state->rgb().GetR();
}

float ColorController::green() const
{
    return m_state->rgb().GetG();
}

float ColorController::blue() const
{
    return m_state->rgb().GetB();
}

float ColorController::hslHue() const
{
    return m_state->hue();
}

float ColorController::hslSaturation() const
{
    return m_state->hslS();
}

float ColorController::lightness() const
{
    return m_state->hslL();
}

float ColorController::hsvHue() const
{
    return m_state->hue();
}

float ColorController::hsvSaturation() const
{
    return m_state->hsvS();
}

float ColorController::value() const
{
    return m_state->hsvV();
}

float ColorController::alpha() const
{
    return m_state->rgb().GetA();
}

void ColorController::setColor(const AZ::Color& color)
{
    if (AreClose(color, m_state->rgb()))
    {
        return;
    }

    const ColorState previousColor = *m_state;
    m_state->setRgb(color);

    emitRgbaChangedSignals(previousColor);
    emitHslChangedSignals(previousColor);
    emitHsvChangedSignals(previousColor);
    emit colorChanged(m_state->rgb());
}

void ColorController::setRed(float red)
{
    if (qFuzzyCompare(red, this->red()))
    {
        return;
    }

    const ColorState previousColor = *m_state;
    m_state->setR(red);

    emit redChanged(red);
    emitHslChangedSignals(previousColor);
    emitHsvChangedSignals(previousColor);
    emit colorChanged(m_state->rgb());
}

void ColorController::setGreen(float green)
{
    if (qFuzzyCompare(green, this->green()))
    {
        return;
    }

    const ColorState previousColor = *m_state;
    m_state->setG(green);

    emit greenChanged(green);
    emitHslChangedSignals(previousColor);
    emitHsvChangedSignals(previousColor);
    emit colorChanged(m_state->rgb());
}

void ColorController::setBlue(float blue)
{
    if (qFuzzyCompare(blue, this->blue()))
    {
        return;
    }

    const ColorState previousColor = *m_state;
    m_state->setB(blue);

    emit blueChanged(blue);
    emitHslChangedSignals(previousColor);
    emitHsvChangedSignals(previousColor);
    emit colorChanged(m_state->rgb());
}

void ColorController::setHslHue(float hue)
{
    if (qFuzzyCompare(hue, hslHue()))
    {
        return;
    }

    const ColorState previousColor = *m_state;
    m_state->setHslH(hue);

    emit hslHueChanged(hue);
    emitRgbaChangedSignals(previousColor);
    emitHsvChangedSignals(previousColor);
    emit colorChanged(m_state->rgb());
}

void ColorController::setHslSaturation(float saturation)
{
    if (qFuzzyCompare(saturation, hslSaturation()))
    {
        return;
    }

    const ColorState previousColor = *m_state;
    m_state->setHslS(saturation);

    emit hslSaturationChanged(saturation);
    emitRgbaChangedSignals(previousColor);
    emitHsvChangedSignals(previousColor);
    emit colorChanged(m_state->rgb());
}

void ColorController::setLightness(float lightness)
{
    if (qFuzzyCompare(lightness, this->lightness()))
    {
        return;
    }

    const ColorState previousColor = *m_state;
    m_state->setHslL(lightness);

    emit lightnessChanged(lightness);
    emitRgbaChangedSignals(previousColor);
    emitHsvChangedSignals(previousColor);
    emit colorChanged(m_state->rgb());
}

void ColorController::setHsvHue(float hue)
{
    if (qFuzzyCompare(hue, hsvHue()))
    {
        return;
    }

    const ColorState previousColor = *m_state;
    m_state->setHsvH(hue);

    emit hsvHueChanged(hue);
    emitRgbaChangedSignals(previousColor);
    emitHslChangedSignals(previousColor);
    emit colorChanged(m_state->rgb());
}

void ColorController::setHsvSaturation(float saturation)
{
    if (qFuzzyCompare(saturation, hsvSaturation()))
    {
        return;
    }

    const ColorState previousColor = *m_state;
    m_state->setHsvS(saturation);

    emit hsvSaturationChanged(saturation);
    emitRgbaChangedSignals(previousColor);
    emitHslChangedSignals(previousColor);
    emit colorChanged(m_state->rgb());
}

void ColorController::setValue(float value)
{
    if (qFuzzyCompare(value, this->value()))
    {
        return;
    }

    const ColorState previousColor = *m_state;
    m_state->setHsvV(value);

    emit valueChanged(value);
    emitRgbaChangedSignals(previousColor);
    emitHslChangedSignals(previousColor);
    emit colorChanged(m_state->rgb());
}

void ColorController::setAlpha(float alpha)
{
    if (qFuzzyCompare(alpha, this->alpha()))
    {
        return;
    }

    m_state->setA(alpha);

    emit alphaChanged(alpha);
    emit colorChanged(m_state->rgb());
}

void ColorController::emitRgbaChangedSignals(const ColorState& previousColor)
{
    if (!qFuzzyCompare(static_cast<float>(previousColor.rgb().GetR()), red()))
    {
        emit redChanged(red());
    }

    if (!qFuzzyCompare(static_cast<float>(previousColor.rgb().GetG()), green()))
    {
        emit greenChanged(green());
    }

    if (!qFuzzyCompare(static_cast<float>(previousColor.rgb().GetB()), blue()))
    {
        emit blueChanged(blue());
    }

    if (!qFuzzyCompare(static_cast<float>(previousColor.rgb().GetA()), alpha()))
    {
        emit alphaChanged(alpha());
    }
}

void ColorController::emitHslChangedSignals(const ColorState& previousColor)
{
    if (!qFuzzyCompare(static_cast<float>(previousColor.hue()), hslHue()))
    {
        emit hslHueChanged(hslHue());
    }

    if (!qFuzzyCompare(static_cast<float>(previousColor.hslS()), hslSaturation()))
    {
        emit hslSaturationChanged(hslSaturation());
    }

    if (!qFuzzyCompare(static_cast<float>(previousColor.hslL()), lightness()))
    {
        emit lightnessChanged(lightness());
    }
}

void ColorController::emitHsvChangedSignals(const ColorState& previousColor)
{
    if (!qFuzzyCompare(static_cast<float>(previousColor.hue()), hsvHue()))
    {
        emit hsvHueChanged(hsvHue());
    }

    if (!qFuzzyCompare(static_cast<float>(previousColor.hsvS()), hsvSaturation()))
    {
        emit hsvSaturationChanged(hsvSaturation());
    }

    if (!qFuzzyCompare(static_cast<float>(previousColor.hsvV()), value()))
    {
        emit valueChanged(value());
    }
}

} // namespace Internal
} // namespace AzQtComponents
#include <Components/Widgets/ColorPicker/ColorController.moc>
