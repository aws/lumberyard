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

#include <AzQtComponents/Components/Widgets/VectorInput.h>
#include <AzQtComponents/Components/Style.h>

#include <QLabel>
#include <QStyleOptionSpinBox>
#include <QDebug>

namespace AzQtComponents
{

static constexpr const char* g_CoordinatePropertyName = "Coordinate";
static constexpr const char* g_CoordLabelObjectName = "coordLabel";

static int g_CoordLabelMargins = 2; // Keep this up to date with css
// To be populated by VectorInput::initStaticVars
static int g_labelSize = -1;

VectorInput::VectorInput(QWidget* parent)
    : SpinBox(parent)
    , m_label(new QLabel(this))
{
    m_label->setObjectName(g_CoordLabelObjectName);
    setCoordinate(Coordinate::X);
}

QRect VectorInput::editFieldRect(const QProxyStyle* style, const QStyleOptionComplex* option, const QWidget* widget, const SpinBox::Config& config)
{
    Q_UNUSED(config);

    if (auto spinBoxOption = qstyleoption_cast<const QStyleOptionSpinBox *>(option))
    {
        auto rect = SpinBox::editFieldRect(style, option, widget, config);
        rect.adjust(config.labelSize, 0, 0, 0);
        return rect;
    }

    return QRect();
}

bool VectorInput::polish(QProxyStyle* style, QWidget* widget, const SpinBox::Config& config)
{
    if (!qobject_cast<VectorInput*>(widget) && !qobject_cast<DoubleVectorInput*>(widget))
    {
        return false;
    }

    if (SpinBox::polish(style, widget, config))
    {
        if (auto label = widget->findChild<QLabel*>(g_CoordLabelObjectName))
        {
            label->move(0, 0);
            label->resize(g_labelSize + g_CoordLabelMargins * 2, g_labelSize + g_CoordLabelMargins * 2);
            label->setAlignment(Qt::AlignHCenter);
            return true;
        }
    }

    return false;
}

void VectorInput::setCoordinate(VectorInput::Coordinate coordinate)
{
    setProperty(g_CoordinatePropertyName, QVariant::fromValue(coordinate));
    switch (coordinate)
    {
        case Coordinate::X:
            m_label->setText("X");
            break;
        case Coordinate::Y:
            m_label->setText("Y");
            break;
        case Coordinate::Z:
            m_label->setText("Z");
            break;
        default:
            Q_UNREACHABLE();
            break;
    }
}

void VectorInput::initStaticVars(int labelSize)
{
    g_labelSize = labelSize;
}

DoubleVectorInput::DoubleVectorInput(QWidget* parent)
    : DoubleSpinBox(parent)
    , m_label(new QLabel(this))
{
    m_label->setObjectName(g_CoordLabelObjectName);
    setCoordinate(VectorInput::Coordinate::X);
}

void DoubleVectorInput::setCoordinate(VectorInput::Coordinate coordinate)
{
    setProperty(g_CoordinatePropertyName, QVariant::fromValue(coordinate));
    switch (coordinate)
    {
        case VectorInput::Coordinate::X:
            m_label->setText("X");
            break;
        case VectorInput::Coordinate::Y:
            m_label->setText("Y");
            break;
        case VectorInput::Coordinate::Z:
            m_label->setText("Z");
            break;
        default:
            Q_UNREACHABLE();
            break;
    }
}

}

#include <Components/Widgets/VectorInput.moc>