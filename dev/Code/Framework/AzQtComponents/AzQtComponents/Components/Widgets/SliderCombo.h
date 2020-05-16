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

        SliderInt* slider() const;
        SpinBox* spinbox() const;

    Q_SIGNALS:
        void valueChanged();

    private:
        SliderInt* m_slider = nullptr;
        SpinBox* m_spinbox = nullptr;
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

        Q_REQUIRED_RESULT int numSteps() const;
        void setNumSteps(int steps);

        Q_REQUIRED_RESULT int decimals() const;
        void setDecimals(int decimals);

        SliderDouble* slider() const;
        DoubleSpinBox* spinbox() const;

    Q_SIGNALS:
        void valueChanged();

    private:
        SliderDouble* m_slider = nullptr;
        DoubleSpinBox* m_spinbox = nullptr;
    };
} // namespace AzQtComponents
