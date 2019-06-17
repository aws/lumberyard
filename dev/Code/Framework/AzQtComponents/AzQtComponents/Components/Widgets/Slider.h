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
#include <QStyle>
#include <QSlider>
#include <QColor>
#include <QRect>
#include <QPoint>

/**
* AUTOMOC
*/


class QSettings;
class QStyleOptionSlider;

namespace AzQtComponents
{
    class Style;

    /**
     * Custom slider control, wrapping Lumberyard functionality.
     * Also provides all of the painting code for LY's sliders.
     *
     * Has similar signals and methods as QSlider, which is the control this class wraps.
     *
     */
    class AZ_QT_COMPONENTS_API Slider
        : public QWidget
    {
        Q_OBJECT
        Q_PROPERTY(Qt::Orientation orientation READ orientation WRITE setOrientation)
        Q_PROPERTY(int toolTipOffsetX READ toolTipOffsetX WRITE setToolTipOffsetX)
        Q_PROPERTY(int toolTipOffsetY READ toolTipOffsetY WRITE setToolTipOffsetY)

    public:

        struct Border
        {
            int thickness;
            QColor color;
            qreal radius;
        };

        struct GradientSliderConfig
        {
            int thickness;
            int length;
            Border grooveBorder;
            Border handleBorder;
        };

        struct SliderConfig
        {
            struct HandleConfig
            {
                QColor color;
                QColor colorDisabled;

                int size;
                int disabledMargin;
            };

            struct GrooveConfig
            {
                QColor color;
                QColor colorHovered;

                int width;
                int midMarkerHeight;
            };

            HandleConfig handle;
            GrooveConfig grove;
        };

        struct Config
        {
            GradientSliderConfig gradientSlider;
            SliderConfig slider;

            QPoint horizontalToolTipOffset;
            QPoint verticalToolTipOffset;
        };

        Slider(QWidget* parent = nullptr);
        Slider(Qt::Orientation orientation, QWidget* parent = nullptr);

        QSize sizeHint() const override;
        QSize minimumSizeHint() const override;

        void setTracking(bool enable);
        bool hasTracking() const;

        void setOrientation(Qt::Orientation orientation);
        Qt::Orientation orientation() const;

        QPoint toolTipOffset() const { return m_toolTipOffset; }
        void setToolTipOffset(const QPoint& toolTipOffset) { m_toolTipOffset = toolTipOffset; }

        int toolTipOffsetX() const { return m_toolTipOffset.x(); }
        void setToolTipOffsetX(int toolTipOffsetX) { m_toolTipOffset.setX(toolTipOffsetX); }

        int toolTipOffsetY() const { return m_toolTipOffset.y(); }
        void setToolTipOffsetY(int toolTipOffsetY) { m_toolTipOffset.setY(toolTipOffsetY); }

        /*!
         * Apply custom formatting to the hover tooltip.
         * For example, for percentages:
         * 
         * SliderInt* slider = new SliderInt();
         * slider->setRange(0, 100);
         * slider->setToolTipFormatting("", "%");
         * 
         */
        void setToolTipFormatting(QString prefix, QString postFix);

        /*!
         * Shows a hover tooltip with the right positioning on top of a slider
         */
        static void showHoverToolTip(QString toolTipText, const QPoint& globalPosition, QSlider* slider, QWidget* toolTipParentWidget, int width, int height, const QPoint& toolTipOffset);

        /*!
         * Returns the value along the slider (between min and max) corresponding to the input pos
         */
        static int valueFromPosition(QSlider* slider, const QPoint& pos, int width, int height, int bottom);

        /*!
         * Applies the Mid Point styling to a Slider.
         */
        static void applyMidPointStyle(Slider* slider);

        /*!
         * Loads the button config data from a settings object.
         */
        static Config loadConfig(QSettings& settings);

        /*!
         * Returns default button config data.
         */
        static Config defaultConfig();

    Q_SIGNALS:
        void sliderPressed();
        void sliderMoved(int position);
        void sliderReleased();

        void actionTriggered(int action);

    protected:
        virtual QString hoverValueText(int sliderValue) const = 0;

        bool eventFilter(QObject* watched, QEvent* event) override;

        QSlider* m_slider = nullptr;
        QString m_toolTipPrefix;
        QString m_toolTipPostfix;

    private:
        friend class Style;

        static int sliderThickness(const Style* style, const QStyleOption* option, const QWidget* widget, const Config& config);
        static int sliderLength(const Style* style, const QStyleOption* option, const QWidget* widget, const Config& config);

        static QRect sliderHandleRect(const Style* style, const QStyleOptionSlider* option, const QWidget* widget, const Config& config);
        static QRect sliderGrooveRect(const Style* style, const QStyleOptionSlider* option, const QWidget* widget, const Config& config);

        // methods used by Style
        static bool polish(Style* style, QWidget* widget, const Config& config);
        static bool drawSlider(const Style* style, const QStyleOption* option, QPainter* painter, const QWidget* widget, const Config& config);

        static void drawMidPointMarker(QPainter* painter, const Config& config, Qt::Orientation orientation, const QRect& grooveRect, const QColor& midMarkerColor);
        static void drawMidPointGrooveHighlights(QPainter* painter, const Config& config, const QRect& grooveRect, const QRect& handleRect, const QColor& handleColor, bool isHovered, const QPoint& mousePos, Qt::Orientation orientation);
        static void drawGrooveHighlights(QPainter* painter, const Slider::Config& config, const QRect& grooveRect, const QRect& handleRect, const QColor& handleColor, bool isHovered, const QPoint& mousePos, Qt::Orientation orientation);
        static void drawHandle(const QStyleOption* option, QPainter* painter, const Config& config, const QRect& handleRect, const QColor& handleColor);

        static int styleHintAbsoluteSetButtons();

        int valueFromPos(const QPoint &pos) const;

        QPoint m_mousePos;

        QPoint m_toolTipOffset;
    };




    class AZ_QT_COMPONENTS_API SliderInt
        : public Slider
    {
        Q_OBJECT

        Q_PROPERTY(int minimum READ minimum WRITE setMinimum)
        Q_PROPERTY(int maximum READ maximum WRITE setMaximum)
        Q_PROPERTY(int value READ value WRITE setValue NOTIFY valueChanged USER true)

    public:
        SliderInt(QWidget* parent = nullptr);
        SliderInt(Qt::Orientation orientation, QWidget* parent = nullptr);

        void setValue(int value);
        int value() const;

        void setMinimum(int min);
        int minimum() const;

        void setMaximum(int max);
        int maximum() const;

        void setRange(int min, int max);

    Q_SIGNALS:
        void valueChanged(int value);

        void rangeChanged(int min, int max);

    protected:
        QString hoverValueText(int sliderValue) const override;
    };




    class AZ_QT_COMPONENTS_API SliderDouble
        : public Slider
    {
        Q_OBJECT

        Q_PROPERTY(double minimum READ minimum WRITE setMinimum)
        Q_PROPERTY(double maximum READ maximum WRITE setMaximum)
        Q_PROPERTY(int numSteps READ numSteps WRITE setNumSteps)
        Q_PROPERTY(double value READ value WRITE setValue NOTIFY valueChanged USER true)
        Q_PROPERTY(int decimals READ decimals WRITE setDecimals);

    public:
        SliderDouble(QWidget* parent = nullptr);
        SliderDouble(Qt::Orientation orientation, QWidget* parent = nullptr);

        void setValue(double value);
        double value() const;

        double minimum() const;
        void setMinimum(double min);

        double maximum() const;
        void setMaximum(double max);

        int numSteps() const;
        void setNumSteps(int steps);

        void setRange(double min, double max, int numSteps = 100);

        int decimals() const { return m_decimals; }
        void setDecimals(int decimals) { m_decimals = decimals; }

    Q_SIGNALS:
        void valueChanged(double value);

        void rangeChanged(double min, double max, int numSteps);

    protected:
        QString hoverValueText(int sliderValue) const override;

    private:
        double m_minimum = 0.0;
        double m_maximum = 1.0;
        int m_numSteps = 100;
        int m_decimals;
    };

} // namespace AzQtComponents
