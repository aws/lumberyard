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

/**
* AUTOMOC
*/


class QSettings;

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

    public:

        struct Config
        {
        };

        Slider(QWidget* parent = nullptr);
        Slider(Qt::Orientation orientation, QWidget* parent = nullptr);

        QSize sizeHint() const override;
        QSize minimumSizeHint() const override;

        void setTracking(bool enable);
        bool hasTracking() const;

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

        // TODO:
        // custom painting
        // tooltip indicating value
        // mid-point version

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
        virtual QString hoverValueText() const = 0;

        bool eventFilter(QObject* watched, QEvent* event) override;

        QSlider* m_slider = nullptr;
        QString m_toolTipPrefix;
        QString m_toolTipPostfix;

    private:
        friend class Style;

        // methods used by Style
        static bool polish(Style* style, QWidget* widget, const Config& config);
        static bool drawSlider(const Style* style, const QStyleOption* option, QPainter* painter, const QWidget* widget, const Config& config);
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
        QString hoverValueText() const override;
    };




    class AZ_QT_COMPONENTS_API SliderDouble
        : public Slider
    {
        Q_OBJECT

        Q_PROPERTY(double minimum READ minimum WRITE setMinimum)
        Q_PROPERTY(double maximum READ maximum WRITE setMaximum)
        Q_PROPERTY(int numSteps READ numSteps WRITE setNumSteps)
        Q_PROPERTY(double value READ value WRITE setValue NOTIFY valueChanged USER true)

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

    Q_SIGNALS:
        void valueChanged(double value);

        void rangeChanged(double min, double max, int numSteps);

    protected:
        QString hoverValueText() const override;

    private:
        double m_minimum = 0.0;
        double m_maximum = 1.0;
        int m_numSteps = 100;
    };

} // namespace AzQtComponents
