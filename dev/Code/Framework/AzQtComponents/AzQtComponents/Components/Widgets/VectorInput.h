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
#include <AzQtComponents/Components/Widgets/SpinBox.h>

class QLabel;

namespace AzQtComponents
{

    class Style;

    /**
     * Specialized SpinBox class showing a label on the left of the edit field.
     * To be used for vector inputs.
     */

    class AZ_QT_COMPONENTS_API VectorInput
        : public SpinBox
    {
        Q_OBJECT

    public:
        enum class Coordinate
        {
            X,
            Y,
            Z
        };
        Q_ENUM(Coordinate)

    public:
        explicit VectorInput(QWidget* parent = nullptr);

        void setCoordinate(Coordinate coord);

    protected:
        friend class Style;

        static QRect editFieldRect(const QProxyStyle* style, const QStyleOptionComplex* option, const QWidget* widget, const SpinBox::Config& config);

        static bool polish(QProxyStyle* style, QWidget* widget, const Config& config);
        static bool unpolish(QProxyStyle* style, QWidget* widget, const Config& config);

        static void initStaticVars(int labelSize);

    private:
        QLabel* m_label;
    };

    /**
     * Specialized DoubleSpinBox class showing a label on the left of the edit field.
     * To be used for vector inputs.
     */

    class AZ_QT_COMPONENTS_API DoubleVectorInput
        : public DoubleSpinBox
    {
        Q_OBJECT

    public:
        explicit DoubleVectorInput(QWidget* parent = nullptr);

        void setCoordinate(VectorInput::Coordinate coord);

    private:
        QLabel* m_label;
    };
}

Q_DECLARE_METATYPE(AzQtComponents::VectorInput::Coordinate)