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
#include <AzQtComponents/Components/Widgets/SpinBox.h>
#include <QWidget>


namespace AzQtComponents
{
    /**
     *This class provide a QWidget which contains a spinbox and a slider
     */
    class AZ_QT_COMPONENTS_API SliderCombo
        : public QWidget
    {
        Q_OBJECT
        Q_PROPERTY(int minimum READ minimum WRITE setMinimum)
        Q_PROPERTY(int maximum READ maximum WRITE setMaximum)
        Q_PROPERTY(int value READ value WRITE setValue NOTIFY valueChanged)
        Q_PROPERTY(int softMinimum READ softMinimum WRITE setSoftMinimum)
        Q_PROPERTY(int softMaximum READ softMaximum WRITE setSoftMaximum)
    public:
        explicit SliderCombo(QWidget *parent = nullptr);
        ~SliderCombo();

        void setValue(int value);
        Q_REQUIRED_RESULT int value() const;

        void setMinimum(int min);
        Q_REQUIRED_RESULT int minimum() const;

        void setMaximum(int max);
        Q_REQUIRED_RESULT int maximum() const;

        void setRange(int min, int max);

        void setSoftMinimum(int min);
        Q_REQUIRED_RESULT int softMinimum() const;

        void setSoftMaximum(int max);
        Q_REQUIRED_RESULT int softMaximum() const;

        Q_REQUIRED_RESULT bool hasSoftMinimum() const { return m_useSoftMinimum; }
        Q_REQUIRED_RESULT bool hasSoftMaximum() const { return m_useSoftMaximum; }

        void setSoftRange(int min, int max);

        SliderInt* slider() const;
        SpinBox* spinbox() const;

    Q_SIGNALS:
        void valueChanged();
        void editingFinished();

    protected:
        void focusInEvent(QFocusEvent* event) override;

    private Q_SLOTS:
        void updateSpinBox();
        void updateSlider();

    private:
        void refreshUi();

        SliderInt* m_slider = nullptr;
        SpinBox* m_spinbox = nullptr;
        int m_sliderMin = 0;
        int m_sliderMax = 100;
        bool m_useSoftMinimum = false;
        bool m_useSoftMaximum = false;
        int m_minimum = 0;
        int m_maximum = 100;
        int m_softMinimum = 0;
        int m_softMaximum = 100;
        int m_value = 0;
    };

    /**
     * This class provide a QWidget which contains a spinbox and a slider
     */
    class AZ_QT_COMPONENTS_API SliderDoubleCombo
        : public QWidget
    {
        Q_OBJECT
        Q_PROPERTY(double minimum READ minimum WRITE setMinimum)
        Q_PROPERTY(double maximum READ maximum WRITE setMaximum)
        Q_PROPERTY(int numSteps READ numSteps WRITE setNumSteps)
        Q_PROPERTY(int decimals READ decimals WRITE setDecimals)
        Q_PROPERTY(double value READ value WRITE setValue NOTIFY valueChanged)
        Q_PROPERTY(double softMinimum READ softMinimum WRITE setSoftMinimum)
        Q_PROPERTY(double softMaximum READ softMaximum WRITE setSoftMaximum)
        Q_PROPERTY(double curveMidpoint READ curveMidpoint WRITE setCurveMidpoint)

    public:
        explicit SliderDoubleCombo(QWidget *parent = nullptr);
        ~SliderDoubleCombo();

        void setValue(double value);
        Q_REQUIRED_RESULT double value() const;

        void setMinimum(double min);
        Q_REQUIRED_RESULT double minimum() const;

        void setMaximum(double max);
        Q_REQUIRED_RESULT double maximum() const;

        void setRange(double min, double max);

        void setSoftMinimum(double min);
        Q_REQUIRED_RESULT double softMinimum() const;

        void setSoftMaximum(double max);
        Q_REQUIRED_RESULT double softMaximum() const;

        Q_REQUIRED_RESULT bool hasSoftMinimum() const { return m_useSoftMinimum; }
        Q_REQUIRED_RESULT bool hasSoftMaximum() const { return m_useSoftMaximum; }

        void setSoftRange(double min, double max);

        Q_REQUIRED_RESULT int numSteps() const;
        void setNumSteps(int steps);

        Q_REQUIRED_RESULT int decimals() const;
        void setDecimals(int decimals);

        Q_REQUIRED_RESULT double curveMidpoint() const;
        void setCurveMidpoint(double midpoint);

        SliderDouble* slider() const;
        DoubleSpinBox* spinbox() const;

    Q_SIGNALS:
        void valueChanged();
        void editingFinished();

    protected:
        void focusInEvent(QFocusEvent* event) override;

    private Q_SLOTS:
        void updateSpinBox();
        void updateSlider();

    private:
        void refreshUi();

        SliderDouble* m_slider = nullptr;
        DoubleSpinBox* m_spinbox = nullptr;
        double m_sliderMin = 0.0;
        double m_sliderMax = 100.0;
        bool m_useSoftMinimum = false;
        bool m_useSoftMaximum = false;
        double m_minimum = 0.0;
        double m_maximum = 100.0;
        double m_softMinimum = 0.0;
        double m_softMaximum = 100.0;
        double m_value = 0.0;
    };
} // namespace AzQtComponents
