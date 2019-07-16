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

#include <AzQtComponents/Components/Widgets/Slider.h>
#include <AzQtComponents/Components/Widgets/GradientSlider.h>
#include <AzQtComponents/Components/Style.h>
#include <AzQtComponents/Components/ConfigHelpers.h>
#include <AzQtComponents/Utilities/Conversions.h>

#include <QStyleFactory>

#include <QMouseEvent>
#include <QApplication>
#include <QPainter>
#include <QStyleOption>
#include <QVariant>
#include <QSettings>
#include <QVBoxLayout>
#include <QEvent>
#include <QToolTip>

namespace AzQtComponents
{

static QString g_horizontalSliderClass = QStringLiteral("HorizontalSlider");
static QString g_verticalSliderClass = QStringLiteral("VerticalSlider");

static void ReadBorder(QSettings& settings, const QString& name, Slider::Border& border)
{
    settings.beginGroup(name);
    ConfigHelpers::read<int>(settings, QStringLiteral("Thickness"), border.thickness);
    ConfigHelpers::read<QColor>(settings, QStringLiteral("Color"), border.color);
    ConfigHelpers::read<qreal>(settings, QStringLiteral("Radius"), border.radius);
    settings.endGroup();
}

static void ReadGradientSlider(QSettings& settings, const QString& name, Slider::GradientSliderConfig& gradientSlider)
{
    settings.beginGroup(name);
    ConfigHelpers::read<int>(settings, QStringLiteral("Thickness"), gradientSlider.thickness);
    ConfigHelpers::read<int>(settings, QStringLiteral("Length"), gradientSlider.length);
    ReadBorder(settings, QStringLiteral("GrooveBorder"), gradientSlider.grooveBorder);
    ReadBorder(settings, QStringLiteral("HandleBorder"), gradientSlider.handleBorder);
    settings.endGroup();
}

static void ReadGroove(QSettings& settings, const QString& name, Slider::SliderConfig::GrooveConfig& groove)
{
    settings.beginGroup(name);

    ConfigHelpers::read<QColor>(settings, QStringLiteral("Color"), groove.color);
    ConfigHelpers::read<QColor>(settings, QStringLiteral("ColorHovered"), groove.colorHovered);
    ConfigHelpers::read<int>(settings, QStringLiteral("Width"), groove.width);
    ConfigHelpers::read<int>(settings, QStringLiteral("MidMarkerHeight"), groove.midMarkerHeight);

    settings.endGroup();
}

static void ReadHandle(QSettings& settings, const QString& name, Slider::SliderConfig::HandleConfig& handle)
{
    settings.beginGroup(name);

    ConfigHelpers::read<QColor>(settings, QStringLiteral("Color"), handle.color);
    ConfigHelpers::read<QColor>(settings, QStringLiteral("DisabledColor"), handle.colorDisabled);
    ConfigHelpers::read<int>(settings, QStringLiteral("Size"), handle.size);
    ConfigHelpers::read<int>(settings, QStringLiteral("DisabledMargin"), handle.disabledMargin);

    settings.endGroup();
}

static void ReadSlider(QSettings& settings, const QString& name, Slider::SliderConfig& slider)
{
    settings.beginGroup(name);

    ReadHandle(settings, QStringLiteral("Handle"), slider.handle);
    ReadGroove(settings, QStringLiteral("Groove"), slider.grove);

    settings.endGroup();
}

static QString g_midPointStyleClass = QStringLiteral("MidPoint");

Slider::Slider(QWidget* parent)
    : Slider(Qt::Horizontal, parent)
{
}

Slider::Slider(Qt::Orientation orientation, QWidget* parent)
    : QWidget(parent)
{
    QVBoxLayout* noFrameLayout = new QVBoxLayout(this);
    noFrameLayout->setMargin(0);
    noFrameLayout->setContentsMargins(0, 0, 0, 0);
    setLayout(noFrameLayout);

    m_slider = new QSlider(orientation, this);
    noFrameLayout->addWidget(m_slider);

    m_slider->installEventFilter(this);

    connect(m_slider, &QSlider::sliderPressed, this, &Slider::sliderPressed);
    connect(m_slider, &QSlider::sliderMoved, this, &Slider::sliderMoved);
    connect(m_slider, &QSlider::sliderReleased, this, &Slider::sliderReleased);
    connect(m_slider, &QSlider::actionTriggered, this, &Slider::actionTriggered);

    m_slider->setMouseTracking(true);
}

QSize Slider::sizeHint() const
{
    return m_slider->sizeHint();
}

QSize Slider::minimumSizeHint() const
{
    return m_slider->minimumSizeHint();
}

void Slider::setTracking(bool enable)
{
    m_slider->setTracking(enable);
}

bool Slider::hasTracking() const
{
    return m_slider->hasTracking();
}

void Slider::setOrientation(Qt::Orientation orientation)
{
    m_slider->setOrientation(orientation);
}

Qt::Orientation Slider::orientation() const
{
    return m_slider->orientation();
}

void Slider::applyMidPointStyle(Slider* slider)
{
    slider->setProperty("class", g_midPointStyleClass);
    slider->m_slider->setProperty("class", g_midPointStyleClass);
}

void Slider::setToolTipFormatting(QString prefix, QString postFix)
{
    m_toolTipPrefix = prefix;
    m_toolTipPostfix = postFix;
}

Slider::Config Slider::loadConfig(QSettings& settings)
{
    Config config = defaultConfig();

    ReadGradientSlider(settings, "GradientSlider", config.gradientSlider);
    ReadSlider(settings, "Slider", config.slider);
    ConfigHelpers::read<QPoint>(settings, QStringLiteral("HorizontalToolTipOffset"), config.horizontalToolTipOffset);
    ConfigHelpers::read<QPoint>(settings, QStringLiteral("VerticalToolTipOffset"), config.verticalToolTipOffset);

    return config;
}

Slider::Config Slider::defaultConfig()
{

   Config config;

    config.gradientSlider.thickness = 16;
    config.gradientSlider.length = 6;
    config.gradientSlider.grooveBorder.thickness = 1;
    config.gradientSlider.grooveBorder.color = QColor(0x33, 0x33, 0x33);
    config.gradientSlider.grooveBorder.radius = 1.5;
    config.gradientSlider.handleBorder.thickness = 1;
    config.gradientSlider.handleBorder.color = QColor(0xff, 0xff, 0xff);
    config.gradientSlider.handleBorder.radius = 1.5;

    config.slider.handle.color = { "#B48BFF" };
    config.slider.handle.colorDisabled = { "#999999" };
    config.slider.handle.size = 12;
    config.slider.handle.disabledMargin = 4;

    config.slider.grove.color = { "#999999" };
    config.slider.grove.colorHovered = { "#DBCCF5" };
    config.slider.grove.width = 2;
    config.slider.grove.midMarkerHeight = 16;

    // numbers chosen because they place the tooltip at a spot that looks decent
    config.horizontalToolTipOffset = QPoint(-8, -16);
    config.verticalToolTipOffset = QPoint(8, -24);

    return config;
}

int Slider::valueFromPosition(QSlider* slider, const QPoint& pos, int width, int height, int bottom)
{
    if (slider->orientation() == Qt::Horizontal)
    {
        return QStyle::sliderValueFromPosition(slider->minimum(), slider->maximum(), pos.x(), width);
    }
    else
    {
        return QStyle::sliderValueFromPosition(slider->minimum(), slider->maximum(), bottom - pos.y(), height);
    }
}

int Slider::valueFromPos(const QPoint& pos) const
{
    return valueFromPosition(m_slider, pos, width(), height(), rect().bottom());
}

void Slider::showHoverToolTip(QString toolTipText, const QPoint& globalPosition, QSlider* slider, QWidget* toolTipParentWidget, int width, int height, const QPoint& toolTipOffset)
{
    QPoint toolTipPosition;

    // QToolTip::showText puts the tooltip (2, 16) pixels down from the specified point - it's hardcoded.
    // We want the tooltip to always appear above the slider, so we offset it according to our settings (which
    // are solely there to put the tooltip in a readable spot - somewhere tracking the mouse x position directly
    // above a horizontal slider, and tracking the mouse y position directly to the right of vertical sliders).
    if (slider->orientation() == Qt::Horizontal)
    {
        toolTipPosition = QPoint(QCursor::pos().x() + toolTipOffset.x(), globalPosition.y() - height + toolTipOffset.y());
    }
    else
    {
        toolTipPosition = QPoint(globalPosition.x() + width + toolTipOffset.x(), QCursor::pos().y() + toolTipOffset.y());
    }

    QToolTip::showText(toolTipPosition, toolTipText, toolTipParentWidget);

    slider->update();
}

bool Slider::eventFilter(QObject* watched, QEvent* event)
{
    if (isEnabled())
    {
        switch (event->type())
        {
            case QEvent::MouseMove:
            {
                auto mEvent = static_cast<QMouseEvent*>(event);
                m_mousePos = mEvent->pos();

                const QString toolTipText = QStringLiteral("%1%2%3").arg(m_toolTipPrefix, hoverValueText(valueFromPos(m_mousePos)), m_toolTipPostfix);

                QPoint globalPosition = parentWidget()->mapToGlobal(pos());
                showHoverToolTip(toolTipText, globalPosition, m_slider, this, width(), height(), m_toolTipOffset);
            }
            break;

            case QEvent::Leave:
                m_mousePos = QPoint();
                break;
        }
    }
    return QObject::eventFilter(watched, event);
}

int Slider::sliderThickness(const Style* style, const QStyleOption* option, const QWidget* widget, const Config& config)
{
    if (widget && style->hasClass(widget, GradientSlider::s_gradientSliderClass))
    {
        if (qobject_cast<const GradientSlider*>(widget))
        {
            return GradientSlider::sliderThickness(style, option, widget, config);
        }
    }
    else
    {
        return config.slider.handle.size;
    }

    return -1;
}

int Slider::sliderLength(const Style* style, const QStyleOption* option, const QWidget* widget, const Config& config)
{
    if (widget && style->hasClass(widget, GradientSlider::s_gradientSliderClass))
    {
        if (qobject_cast<const GradientSlider*>(widget))
        {
            return GradientSlider::sliderLength(style, option, widget, config);
        }
    }
    else
    {
        return config.slider.handle.size;
    }

    return -1;
}

QRect Slider::sliderHandleRect(const Style* style, const QStyleOptionSlider* option, const QWidget* widget, const Config& config)
{
    Q_ASSERT(widget);

    if (style->hasClass(widget, GradientSlider::s_gradientSliderClass))
    {
        if (qobject_cast<const GradientSlider*>(widget))
        {
            return GradientSlider::sliderHandleRect(style, option, widget, config);
        }
    }
    else
    {
        const auto grooveRect = sliderGrooveRect(style, option, widget, config);
        const auto center = style->QCommonStyle::subControlRect(QStyle::CC_Slider, option, QStyle::SC_SliderHandle, widget).center();
        QRect handle(0, 0, config.slider.handle.size, config.slider.handle.size);
        if (option->orientation == Qt::Horizontal)
        {
            handle.moveCenter({ center.x(), grooveRect.center().y() });
        }
        else
        {
            handle.moveCenter({ grooveRect.center().x(), center.y() });
        }
        return handle;
    }

    return {};
}

QRect Slider::sliderGrooveRect(const Style* style, const QStyleOptionSlider* option, const QWidget* widget, const Config& config)
{
    Q_ASSERT(widget);

    if (style->hasClass(widget, GradientSlider::s_gradientSliderClass))
    {
        if (qobject_cast<const GradientSlider*>(widget))
        {
            return GradientSlider::sliderGrooveRect(style, option, widget, config);
        }
    }
    else
    {
        const auto handleMid = config.slider.handle.size / 2;
        auto rect = style->QCommonStyle::subControlRect(QProxyStyle::CC_Slider, option, QProxyStyle::SC_SliderGroove, widget);        
        if (option->orientation == Qt::Horizontal)
        {
            rect.adjust(handleMid, 0, -handleMid, 0);
            rect.setHeight(config.slider.grove.width);
        }
        else
        {
            rect.adjust(0, handleMid, 0, -handleMid);
            rect.setWidth(config.slider.grove.width);
        }
        rect.moveCenter(widget->rect().center());
        return rect;
    }

    return {};
}

bool Slider::polish(Style* style, QWidget* widget, const Slider::Config& config)
{
    Q_UNUSED(config);

    auto polishSlider = [style](auto slider)
    {
        // Qt's stylesheet parsing doesn't set custom properties on things specified via
        // pseudo-states, such as horizontal/vertical, so we implement our own
        if (slider->orientation() == Qt::Horizontal)
        {
            Style::removeClass(slider, g_verticalSliderClass);
            Style::addClass(slider, g_horizontalSliderClass);
        }
        else
        {
            Style::removeClass(slider, g_horizontalSliderClass);
            Style::addClass(slider, g_verticalSliderClass);
        }
    };

    // Apply the right styles to our QSlider objects, so that css rules can be properly applied
    if (auto slider = qobject_cast<Slider*>(widget))
    {
        polishSlider(slider);
    }

    // Also apply the right styles to our AzQtComponents::Slider container
    if (auto slider = qobject_cast<QSlider*>(widget))
    {
        polishSlider(slider);
    }

    return false;
}

bool Slider::drawSlider(const Style* style, const QStyleOption* option, QPainter* painter, const QWidget* widget, const Slider::Config& config)
{
   const QStyleOptionSlider* sliderOption = static_cast<const QStyleOptionSlider*>(option);
   const QSlider* slider = qobject_cast<const QSlider*>(widget);
   if (slider != nullptr)
    {
        if (style->hasClass(widget, GradientSlider::s_gradientSliderClass))
        {
            Q_ASSERT(widget);
            return GradientSlider::drawSlider(style, static_cast<const QStyleOptionSlider*>(option), painter, widget, config);
        }
        else if (auto sliderWidget = qobject_cast<const Slider*>(widget->parentWidget()))
        {
            const QColor handleColor = option->state & QStyle::State_Enabled ? config.slider.handle.color : config.slider.handle.colorDisabled;
            const QRect grooveRect = sliderGrooveRect(style, sliderOption, widget, config);
            const QRect handleRect = sliderHandleRect(style, sliderOption, widget, config);

            // only show the hover highlight if the mouse is hovered over the control, and the slider isn't being clicked on or dragged
            const bool isHovered = !sliderWidget->m_mousePos.isNull() && !(QApplication::mouseButtons() & Qt::MouseButton::LeftButton);

            //draw the groove background, without highlight colors from the mouse hover or the selected value
            painter->fillRect(grooveRect, config.slider.grove.color);

            if (style->hasClass(widget, g_midPointStyleClass))
            {
                drawMidPointMarker(painter, config, sliderOption->orientation, grooveRect, handleColor);

                drawMidPointGrooveHighlights(painter, config, grooveRect, handleRect, handleColor, isHovered, sliderWidget->m_mousePos, sliderOption->orientation);
            }
            else
            {
                drawGrooveHighlights(painter, config, grooveRect, handleRect, handleColor, isHovered, sliderWidget->m_mousePos, sliderOption->orientation);
            }

            drawHandle(option, painter, config, handleRect, handleColor);

            return true;
        }
    }
    return false;
}

void Slider::drawMidPointMarker(QPainter* painter, const Slider::Config& config, Qt::Orientation orientation, const QRect& grooveRect, const QColor& midMarkerColor)
{
    int width = orientation == Qt::Horizontal ? config.slider.grove.width : config.slider.grove.midMarkerHeight;
    int height = orientation == Qt::Horizontal ? config.slider.grove.midMarkerHeight : config.slider.grove.width;

    auto rect = QRect(0, 0, width, height);
    rect.moveCenter(grooveRect.center());
    painter->fillRect(rect, midMarkerColor);
};

void Slider::drawMidPointGrooveHighlights(QPainter* painter, const Slider::Config& config, const QRect& grooveRect, const QRect& handleRect, const QColor& handleColor, bool isHovered, const QPoint& mousePos, Qt::Orientation orientation)
{
    QPoint valueGrooveStart = orientation == Qt::Horizontal ? QPoint(grooveRect.center().x(), grooveRect.top()) : QPoint(grooveRect.left(), grooveRect.center().y());
    QPoint valueGrooveEnd = orientation == Qt::Horizontal ? QPoint(handleRect.x(), grooveRect.bottom()) : QPoint(grooveRect.center().x(), handleRect.bottom());

    //color the grove from the mid to the handle to show the amount selected
    painter->fillRect(QRect(valueGrooveStart, valueGrooveEnd), handleColor);

    if (isHovered)
    {
        QPoint hoveredEnd;
        QPoint hoveredStart = valueGrooveStart;

        if (orientation == Qt::Horizontal)
        {
            const auto x = qBound(grooveRect.left(), mousePos.x(), grooveRect.right());
            hoveredEnd = QPoint(x, grooveRect.bottom());

            // be careful not to completely cover the value highlight
            if (((handleRect.x() < valueGrooveStart.x()) && (mousePos.x() < handleRect.x())) || ((handleRect.x() > valueGrooveStart.x()) && (mousePos.x() > handleRect.x())))
            {
                hoveredStart.setX(handleRect.x());
            }
        }
        else
        {
            const auto y = qBound(grooveRect.top(), mousePos.y(), grooveRect.bottom());
            hoveredEnd = QPoint(grooveRect.right(), y);

            // be careful not to completely cover the value highlight
            if (((handleRect.y() < valueGrooveStart.y()) && (mousePos.y() < handleRect.y())) || ((handleRect.y() > valueGrooveStart.y()) && (mousePos.y() > handleRect.y())))
            {
                hoveredStart.setY(handleRect.y());
            }
        }

        painter->fillRect(QRect(hoveredStart, hoveredEnd), config.slider.grove.colorHovered);
    }
}

void Slider::drawGrooveHighlights(QPainter* painter, const Slider::Config& config, const QRect& grooveRect, const QRect& handleRect, const QColor& handleColor, bool isHovered, const QPoint& mousePos, Qt::Orientation orientation)
{
    QPoint valueGrooveStart = orientation == Qt::Horizontal ? grooveRect.topLeft() : grooveRect.bottomLeft();
    QPoint handleMiddle = orientation == Qt::Horizontal ? QPoint(handleRect.x(), grooveRect.bottom()) : QPoint(grooveRect.right(), handleRect.y());

    //color the grove from the mid to the handle
    painter->fillRect(QRect(valueGrooveStart, handleMiddle), handleColor);

    if (isHovered)
    {
        QPoint hoveredStart = valueGrooveStart;
        QPoint hoveredEnd;

        if (orientation == Qt::Horizontal)
        {
            const auto x = qBound(grooveRect.left(), mousePos.x(), grooveRect.right());

            if (x > handleMiddle.x())
            {
                hoveredStart.setX(handleMiddle.x());
            }

            hoveredEnd = QPoint(x, grooveRect.bottom());
        }
        else
        {
            const auto y = qBound(grooveRect.top(), mousePos.y(), grooveRect.bottom());

            if (y < handleMiddle.y())
            {
                hoveredStart.setY(handleMiddle.y());
            }

            hoveredEnd = QPoint(grooveRect.right(), y);
        }

        painter->fillRect(QRect(hoveredStart, hoveredEnd), config.slider.grove.colorHovered);
    }
}

void Slider::drawHandle(const QStyleOption* option, QPainter* painter, const Slider::Config& config, const QRect& handleRect, const QColor& handleColor)
{
    //handle
    painter->save();
    painter->setRenderHint(QPainter::Antialiasing, true);
    if (option->state & QStyle::State_Enabled)
    {
        painter->setPen(Qt::NoPen);
    }
    else
    {
        auto pen = painter->pen();
        pen.setBrush(option->palette.background());
        pen.setWidth(config.slider.handle.disabledMargin);
        painter->setPen(pen);
    }
    painter->setBrush(handleColor);
    painter->drawRoundedRect(handleRect, 100.0, 100.0);

    painter->restore();
}

int Slider::styleHintAbsoluteSetButtons()
{
    // Make sliders jump to the value when the user clicks on them instead of the Qt default of moving closer to the clicked location
    return (Qt::LeftButton | Qt::MidButton | Qt::RightButton);
}


SliderInt::SliderInt(QWidget* parent)
    : SliderInt(Qt::Horizontal, parent)
{
}

SliderInt::SliderInt(Qt::Orientation orientation, QWidget* parent)
    : Slider(orientation, parent)
{
    connect(m_slider, &QSlider::valueChanged, this, &SliderInt::valueChanged);
    connect(m_slider, &QSlider::rangeChanged, this, &SliderInt::rangeChanged);
}

void SliderInt::setValue(int value)
{
    m_slider->setValue(value);
}

int SliderInt::value() const
{
    return m_slider->value();
}

void SliderInt::setMinimum(int min)
{
    m_slider->setMinimum(min);
}

int SliderInt::minimum() const
{
    return m_slider->minimum();
}

void SliderInt::setMaximum(int max)
{
    m_slider->setMaximum(max);
}

int SliderInt::maximum() const
{
    return m_slider->maximum();
}

void SliderInt::setRange(int min, int max)
{
    m_slider->setRange(min, max);
}

QString SliderInt::hoverValueText(int sliderValue) const
{
    return QStringLiteral("%1").arg(sliderValue);
}


inline double calculateRealSliderValue(const SliderDouble* slider, int value)
{
    double sliderValue = static_cast<double>(value);
    double range = slider->maximum() - slider->minimum();
    double steps = static_cast<double>(slider->numSteps() - 1);

    double denormalizedValue = ((sliderValue / steps) * range) + slider->minimum();

    return denormalizedValue;
}


SliderDouble::SliderDouble(QWidget* parent)
    : SliderDouble(Qt::Horizontal, parent)
{
}

static const int g_sliderDecimalPrecisonDefault = 7;

SliderDouble::SliderDouble(Qt::Orientation orientation, QWidget* parent)
    : Slider(orientation, parent)
    , m_decimals(g_sliderDecimalPrecisonDefault)
{
    setRange(m_minimum, m_maximum, m_numSteps);

    connect(m_slider, &QSlider::valueChanged, this, [this](int newValue) {
        double denormalizedValue = calculateRealSliderValue(this, newValue);
        Q_EMIT valueChanged(denormalizedValue);
    });
}

void SliderDouble::setValue(double value)
{
    int normalizedValue = static_cast<int>(((value - m_minimum) / (m_maximum - m_minimum)) * static_cast<double>(m_numSteps - 1));

    m_slider->setValue(normalizedValue);
}

double SliderDouble::value() const
{
    return calculateRealSliderValue(this, m_slider->value());
}

double SliderDouble::minimum() const
{
    return m_minimum;
}

void SliderDouble::setMinimum(double min)
{
    setRange(min, m_maximum, m_numSteps);
}

double SliderDouble::maximum() const
{
    return m_maximum;
}

void SliderDouble::setMaximum(double max)
{
    setRange(m_minimum, max, m_numSteps);
}

int SliderDouble::numSteps() const
{
    return m_numSteps;
}

void SliderDouble::setNumSteps(int steps)
{
    setRange(m_minimum, m_maximum, steps);
}

void SliderDouble::setRange(double min, double max, int numSteps)
{
    m_slider->setRange(0, (numSteps - 1));

    m_maximum = max;
    m_minimum = min;
    m_numSteps = numSteps;

    Q_ASSERT(m_numSteps != 0);
    if (m_numSteps == 0)
    {
        m_numSteps = 1;
    }

    Q_EMIT rangeChanged(min, max, numSteps);
}

QString SliderDouble::hoverValueText(int sliderValue) const
{
    // maybe format this, max number of digits?
    QString valueText = toString(calculateRealSliderValue(this, sliderValue), m_decimals, locale());
    return QStringLiteral("%1").arg(valueText);
}

} // namespace AzQtComponents
