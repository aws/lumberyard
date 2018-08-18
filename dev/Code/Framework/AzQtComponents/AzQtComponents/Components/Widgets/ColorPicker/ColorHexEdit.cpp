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

#include <AzQtComponents/Components/Widgets/ColorPicker/ColorHexEdit.h>

#include <QDoubleValidator>
#include <QGridLayout>
#include <QLineEdit>
#include <QLabel>
#include <QEvent>

#include <functional>

namespace AzQtComponents
{

ColorHexEdit::ColorHexEdit(QWidget* parent)
    : QWidget(parent)
    , m_red(0)
    , m_green(0)
    , m_blue(0)
{
    auto layout = new QGridLayout(this);
    layout->setMargin(0);

    m_edit = new QLineEdit(QStringLiteral("000000"), this);
    m_edit->setValidator(new QRegExpValidator(QRegExp(QStringLiteral("^[0-9A-Fa-f]{0,6}$")), this));
    m_edit->setFixedWidth(50);
    m_edit->setAlignment(Qt::AlignHCenter);
    connect(m_edit, &QLineEdit::textChanged, this, &ColorHexEdit::textChanged);
    connect(m_edit, &QLineEdit::editingFinished, this, [this]() {
        m_valueChanging = false;
        emit valueChangeEnded();
    });
    m_edit->installEventFilter(this);
    layout->addWidget(m_edit, 0, 0);

    auto hexLabel = new QLabel(tr("#"), this);
    hexLabel->setAlignment(Qt::AlignHCenter);
    layout->addWidget(hexLabel, 1, 0);

    initEditValue();
}

ColorHexEdit::~ColorHexEdit()
{
}

qreal ColorHexEdit::red() const
{
    return m_red;
}

qreal ColorHexEdit::green() const
{
    return m_green;
}

qreal ColorHexEdit::blue() const
{
    return m_blue;
}

void ColorHexEdit::setRed(qreal red)
{
    if (qFuzzyCompare(red, m_red))
    {
        return;
    }
    m_red = red;
    initEditValue();
}

void ColorHexEdit::setGreen(qreal green)
{
    if (qFuzzyCompare(green, m_green))
    {
        return;
    }
    m_green = green;
    initEditValue();
}

void ColorHexEdit::setBlue(qreal blue)
{
    if (qFuzzyCompare(blue, m_blue))
    {
        return;
    }
    m_blue = blue;
    initEditValue();
}

bool ColorHexEdit::eventFilter(QObject* watched, QEvent* event)
{
    if (watched == m_edit)
    {
        if (event->type() == QEvent::FocusIn)
        {
            m_valueChanging = true;
            emit valueChangeBegan();
        }
        else if (event->type() == QEvent::FocusOut)
        {
            m_valueChanging = false;
            emit valueChangeEnded();
        }
    }

    return false;
}

void ColorHexEdit::textChanged(const QString& text)
{
    if (!m_valueChanging)
    {
        m_valueChanging = true;
        emit valueChangeBegan();
    }

    const auto rgb = text.toUInt(nullptr, 16);

    const int red = (rgb >> 16) & 0xff;
    const qreal redF = static_cast<qreal>(red)/255.0;
    bool signalRedChanged = false;
    if (!qFuzzyCompare(m_red, redF))
    {
        m_red = redF;
        signalRedChanged = true;
    }

    const int green = (rgb >> 8) & 0xff;
    const qreal greenF = static_cast<qreal>(green)/255.0;
    bool signalGreenChanged = false;
    if (!qFuzzyCompare(m_green, greenF))
    {
        m_green = greenF;
        signalGreenChanged = true;
    }

    const int blue = rgb & 0xff;
    const qreal blueF = static_cast<qreal>(blue)/255.0;
    bool signalBlueChanged = false;
    if (!qFuzzyCompare(m_blue, blueF))
    {
        m_blue = blueF;
        signalBlueChanged = true;
    }

    // need to emit after we're done updating all components
    // or we'll get signalled before we're done

    if (signalRedChanged)
    {
        emit redChanged(m_red);
    }

    if (signalGreenChanged)
    {
        emit greenChanged(m_green);
    }

    if (signalBlueChanged)
    {
        emit blueChanged(m_blue);
    }
}

void ColorHexEdit::initEditValue()
{
    const int red = qRound(m_red*255.0);
    const int green = qRound(m_green*255.0);
    const int blue = qRound(m_blue*255.0);
    const unsigned rgb = blue | (green << 8) | (red << 16);

    if (rgb != m_edit->text().toUInt(nullptr, 16))
    {
        const auto value = QStringLiteral("%1").arg(rgb, 6, 16, QLatin1Char('0'));
        const QSignalBlocker b(m_edit);
        m_edit->setText(value.toUpper());
    }
}

} // namespace AzQtComponents
#include <Components/Widgets/ColorPicker/ColorHexEdit.moc>
