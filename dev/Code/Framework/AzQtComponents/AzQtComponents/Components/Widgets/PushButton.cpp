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

#include <AzQtComponents/Components/Widgets/PushButton.h>
#include <AzQtComponents/Components/Style.h>

#include <QStyleFactory>

#include <QToolButton>
#include <QPushButton>
#include <QPainter>
#include <QStyleOption>
#include <QVariant>
#include <QSettings>
#include <QPixmap>
#include <QImage>
#include <QPixmapCache>

namespace AzQtComponents
{

static QString g_smallIconClass = QStringLiteral("SmallIcon");
static QString g_attachedButtonClass = QStringLiteral("AttachedButton");

void PushButton::applyPrimaryStyle(QPushButton* button)
{
    button->setDefault(true);
}

void PushButton::applySmallIconStyle(QToolButton* button)
{
    Style::addClass(button, g_smallIconClass);
}

void PushButton::applyAttachedStyle(QToolButton* button)
{
    Style::addClass(button, g_attachedButtonClass);
}

template <typename Button>
bool buttonHasMenu(Button* button)
{
    if (button != nullptr)
    {
        return button->menu();
    }

    return false;
}

bool PushButton::polish(Style* style, QWidget* widget, const PushButton::Config& config)
{
    QToolButton* toolButton = qobject_cast<QToolButton*>(widget);
    QPushButton* pushButton = qobject_cast<QPushButton*>(widget);
    if ((style->hasClass(widget, g_smallIconClass) && (toolButton != nullptr)) || (style->hasClass(widget, g_attachedButtonClass) && (pushButton != nullptr)))
    {
        widget->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum);

        bool hasMenu = buttonHasMenu(toolButton) || buttonHasMenu(pushButton);
        widget->setMaximumSize(config.smallIcon.width + (hasMenu ? config.smallIcon.arrowWidth : 0), config.smallIcon.frame.height);

        style->repolishOnSettingsChange(widget);

        return true;
    }
    else if (pushButton != nullptr)
    {
        widget->setMaximumHeight(config.defaultFrame.height);
        return true;
    }

    return false;
}

int PushButton::buttonMargin(const Style* style, const QStyleOption* option, const QWidget* widget, const PushButton::Config& config)
{
    Q_UNUSED(option);

    if (style->hasClass(widget, g_smallIconClass))
    {
        return config.smallIcon.frame.margin;
    }
    else if (style->hasClass(widget, g_attachedButtonClass))
    {
        return 0; // TODO
    }
    
    return config.defaultFrame.margin;
}

QSize PushButton::sizeFromContents(const Style* style, QStyle::ContentsType type, const QStyleOption* option, const QSize& size, const QWidget* widget, const PushButton::Config& config)
{
    QSize sz = style->QProxyStyle::sizeFromContents(type, option, size, widget);

    if (style->hasClass(widget, g_smallIconClass) || style->hasClass(widget, g_attachedButtonClass))
    {
        sz.setHeight(config.smallIcon.frame.height);
    }
    else
    {
        const QPushButton* pushButton = qobject_cast<const QPushButton*>(widget);
        if (pushButton != nullptr)
        {
            sz.setHeight(config.defaultFrame.height);
        }
    }

    return sz;
}

bool PushButton::drawPushButtonBevel(const Style* style, const QStyleOption* option, QPainter* painter, const QWidget* widget, const PushButton::Config& config)
{
    Q_UNUSED(style);

    QRectF r = option->rect.adjusted(0, 0, -1, -1);

    bool isSmallIconButton = style->hasClass(widget, g_smallIconClass);

    QColor gradientStartColor;
    QColor gradientEndColor;
    const auto* buttonOption = qstyleoption_cast<const QStyleOptionButton*>(option);

    bool isDefault = buttonOption && (buttonOption->features & QStyleOptionButton::DefaultButton);
    const bool isPrimary = isDefault || (style->hasClass(widget, QLatin1String("Primary")));
    bool isDisabled = !(option->state & QStyle::State_Enabled);

    Border border = isDisabled ? config.disabledBorder : config.defaultBorder;

    selectColors(option, isPrimary ? config.primary : config.secondary, isDisabled, gradientStartColor, gradientEndColor);

    if (widget->hasFocus())
    {
        border = config.focusedBorder;
    }

    float radius = isSmallIconButton ? config.smallIcon.frame.radius : config.defaultFrame.radius;
    drawFilledFrame(painter, r, gradientStartColor, gradientEndColor, border, radius);

    return true;
}

bool PushButton::drawToolButton(const Style* style, const QStyleOptionComplex* option, QPainter* painter, const QWidget* widget, const PushButton::Config& config)
{
    if (style->hasClass(widget, g_attachedButtonClass))
    {
        return false;
    }

    if (style->hasClass(widget, g_smallIconClass))
    {
        drawSmallIconButton(style, option, painter, widget, config);
        return true;
    }

    const bool drawFrame = false;
    drawSmallIconButton(style, option, painter, widget, config, drawFrame);
    return true;
}

bool PushButton::drawIndicatorArrow(const Style* style, const QStyleOption* option, QPainter* painter, const QWidget* widget, const Config& config)
{
    Q_UNUSED(style);
    Q_UNUSED(option);
    Q_UNUSED(painter);
    Q_UNUSED(config);

    // If it's got the small icon class, arrow drawing is already done in drawToolButton
    if (style->hasClass(widget, g_smallIconClass))
    {
        return true;
    }

    return false;
}

bool PushButton::drawPushButtonFocusRect(const Style* /*style*/, const QStyleOption* /*option*/, QPainter* /*painter*/, const QWidget* /*widget*/, const PushButton::Config& /*config*/)
{
    // no frame; handled when the bevel is drawn

    return true;
}

static void ReadFrame(QSettings& settings, const QString& name, PushButton::Frame& frame)
{
    settings.beginGroup(name);
    frame.height = settings.value("Height", frame.height).toInt();
    frame.radius = settings.value("Radius", frame.radius).toInt();
    frame.margin = settings.value("Margin", frame.margin).toInt();
    settings.endGroup();
}

static void ReadColor(QSettings& settings, const QString& name, QColor& color)
{
    // only overwrite the value if it's set; otherwise, it'll stay the default
    if (settings.contains(name))
    {
        color = QColor(settings.value(name).toString());
    }
}

static void ReadGradient(QSettings& settings, const QString& name, PushButton::Gradient& gradient)
{
    settings.beginGroup(name);
    ReadColor(settings, "Start", gradient.start);
    ReadColor(settings, "End", gradient.end);
    settings.endGroup();
}

static void ReadButtonColorSet(QSettings& settings, const QString& name, PushButton::ColorSet& colorSet)
{
    settings.beginGroup(name);
    ReadGradient(settings, "Disabled", colorSet.disabled);
    ReadGradient(settings, "Sunken", colorSet.sunken);
    ReadGradient(settings, "Hovered", colorSet.hovered);
    ReadGradient(settings, "Normal", colorSet.normal);
    settings.endGroup();
}

static void ReadBorder(QSettings& settings, const QString& name, PushButton::Border& border)
{
    settings.beginGroup(name);
    border.thickness = settings.value("Thickness", border.thickness).toInt();
    ReadColor(settings, "Color", border.color);
    settings.endGroup();
}

static void ReadSmallIcon(QSettings& settings, const QString& name, PushButton::SmallIcon& smallIcon)
{
    settings.beginGroup(name);
    ReadFrame(settings, "Frame", smallIcon.frame);
    ReadColor(settings, "EnabledArrowColor", smallIcon.enabledArrowColor);
    ReadColor(settings, "DisabledArrowColor", smallIcon.disabledArrowColor);
    smallIcon.width = settings.value("Width", smallIcon.width).toInt();
    smallIcon.arrowWidth = settings.value("ArrowWidth", smallIcon.arrowWidth).toInt();
    settings.endGroup();
}

PushButton::Config PushButton::loadConfig(QSettings& settings)
{
    Config config = defaultConfig();

    ReadButtonColorSet(settings, "PrimaryColorSet", config.primary);
    ReadButtonColorSet(settings, "SecondaryColorSet", config.secondary);

    ReadBorder(settings, "Border", config.defaultBorder);
    ReadBorder(settings, "DisabledBorder", config.disabledBorder);
    ReadBorder(settings, "FocusedBorder", config.focusedBorder);
    ReadFrame(settings, "DefaultFrame", config.defaultFrame);
    ReadSmallIcon(settings, "SmallIcon", config.smallIcon);

    return config;
}

PushButton::Config PushButton::defaultConfig()
{
    Config config;

    config.primary.disabled.start = QColor("#978DAA");
    config.primary.disabled.end = QColor("#978DAA");

    config.primary.sunken.start = QColor("#4D2F7B");
    config.primary.sunken.end = QColor("#4D2F7B");

    config.primary.hovered.start = QColor("#9F7BD7");
    config.primary.hovered.end = QColor("#8B6EBA");

    config.primary.normal.start = QColor("#8156CF");
    config.primary.normal.end = QColor("#6441A4");

    config.secondary.disabled.start = QColor("#808080");
    config.secondary.disabled.end = QColor("#808080");

    config.secondary.sunken.start = QColor("#444444");
    config.secondary.sunken.end = QColor("#444444");

    config.secondary.hovered.start = QColor("#777777");
    config.secondary.hovered.end = QColor("#666666");

    config.secondary.normal.start = QColor("#666666");
    config.secondary.normal.end = QColor("#555555");

    config.defaultBorder.thickness = 1;
    config.defaultBorder.color = QColor("#000000");

    config.disabledBorder.thickness = config.defaultBorder.thickness;
    config.disabledBorder.color = QColor("#222222");

    config.focusedBorder.thickness = 2;
    config.focusedBorder.color = QColor("#C8ABFF");

    config.defaultFrame.height = 32;
    config.defaultFrame.radius = 4;
    config.defaultFrame.margin = 4;

    config.smallIcon.frame.height = 24;
    config.smallIcon.frame.radius = 2;
    config.smallIcon.frame.margin = 4;
    config.smallIcon.enabledArrowColor = QColor("#FFFFFF");
    config.smallIcon.disabledArrowColor = QColor("#BBBBBB");
    config.smallIcon.width = 24;
    config.smallIcon.arrowWidth = 10;

    return config;
}

void PushButton::drawSmallIconButton(const Style* style, const QStyleOption* option, QPainter* painter, const QWidget* widget, const PushButton::Config& config, bool drawFrame)
{
    // Draws the icon and the arrow drop down together, as per the spec, which calls for no separating borders
    // between the icon and the arrow.
    // There aren't enough controls exposed via CSS to do this in a stylesheet.

    if (const QStyleOptionToolButton* buttonOption = qstyleoption_cast<const QStyleOptionToolButton*>(option))
    {
        QRect buttonArea = style->subControlRect(QStyle::CC_ToolButton, buttonOption, QStyle::SC_ToolButton, widget);
        QRect menuArea = style->subControlRect(QStyle::CC_ToolButton, buttonOption, QStyle::SC_ToolButtonMenu, widget);
        QRect totalArea = buttonArea;

        int menuButtonIndicator = style->pixelMetric(QStyle::PM_MenuButtonIndicator, buttonOption, widget);

        if (!(buttonOption->subControls & QStyle::SC_ToolButtonMenu) && (buttonOption->features & QStyleOptionToolButton::HasMenu))
        {
            // replicating what the Qt Fusion style does
            QRect ir = buttonOption->rect;
            menuArea = QRect(ir.right() + 5 - menuButtonIndicator, ir.y() + ir.height() - menuButtonIndicator + 4, menuButtonIndicator - 6, menuButtonIndicator - 6);
        }

        QStyle::State bflags = buttonOption->state & ~QStyle::State_Sunken;

        if (bflags & QStyle::State_AutoRaise)
        {
            if (!(bflags & QStyle::State_MouseOver) || !(bflags & QStyle::State_Enabled))
            {
                bflags &= ~QStyle::State_Raised;
            }
        }

        QStyle::State mflags = bflags;
        if (buttonOption->state & QStyle::State_Sunken)
        {
            if (buttonOption->activeSubControls & QStyle::SC_ToolButton)
            {
                bflags |= QStyle::State_Sunken;
            }

            mflags |= QStyle::State_Sunken;
        }

        // check if we need to deal with the menu at all
        if ((buttonOption->subControls & QStyle::SC_ToolButtonMenu) || (buttonOption->features & QStyleOptionToolButton::HasMenu))
        {
            totalArea = buttonArea.united(menuArea);
        }

        if (drawFrame)
        {
            drawSmallIconFrame(style, option, totalArea, painter, config);
        }

        drawSmallIconLabel(style, buttonOption, bflags, buttonArea, painter, widget, config);

        drawSmallIconArrow(style, buttonOption, mflags, menuArea, painter, widget, config);
    }
}

void PushButton::drawSmallIconFrame(const Style* style, const QStyleOption* option, const QRect& frame, QPainter* painter, const PushButton::Config& config)
{
    Q_UNUSED(style);

    bool isDisabled = !(option->state & QStyle::State_Enabled);

    PushButton::Border border = isDisabled ? config.disabledBorder : config.defaultBorder;

    if (option->state & QStyle::State_HasFocus)
    {
        border = config.focusedBorder;
    }

    QColor gradientStartColor;
    QColor gradientEndColor;

    selectColors(option, config.secondary, isDisabled, gradientStartColor, gradientEndColor);

    float radius = config.smallIcon.frame.radius;
    drawFilledFrame(painter, frame, gradientStartColor, gradientEndColor, border, radius);
}

void PushButton::drawSmallIconLabel(const Style* style, const QStyleOptionToolButton* buttonOption, QStyle::State state, const QRect& buttonArea, QPainter* painter, const QWidget* widget, const PushButton::Config& config)
{
    Q_UNUSED(config);

    QStyleOptionToolButton label = *buttonOption;
    label.state = state;
    int fw = style->pixelMetric(QStyle::PM_DefaultFrameWidth, buttonOption, widget);
    label.rect = buttonArea.adjusted(fw, fw, -fw, -fw);
    style->drawControl(QStyle::CE_ToolButtonLabel, &label, painter, widget);
}

static QPixmap initializeDownArrowPixmap(const QColor& arrowColor)
{
    // use the arrow stored in Qt in the fusion style
    QString arrowFileName = QLatin1String(":/qt-project.org/styles/commonstyle/images/fusion_arrow.png");

    QImage image(arrowFileName);

    Q_ASSERT(image.format() == QImage::Format_ARGB32);

    int width = image.width();
    int height = image.height();
    int source = arrowColor.rgba();

    unsigned char sourceRed = qRed(source);
    unsigned char sourceGreen = qGreen(source);
    unsigned char sourceBlue = qBlue(source);

    // we only care about the alpha channel, because it's got the arrow info
    // apply the input color to everything but the alpha
    for (int y = 0; y < height; ++y)
    {
        QRgb* data = reinterpret_cast<QRgb*>(image.scanLine(y));
        for (int x = 0; x < width; x++) {
            QRgb col = data[x];
            unsigned char red = sourceRed;
            unsigned char green = sourceGreen;
            unsigned char blue = sourceBlue;
            unsigned char alpha = qAlpha(col);
            data[x] = qRgba(red, green, blue, alpha);
        }
    }

    // pre-multiply the alpha, so that it works when we paint it
    image = image.convertToFormat(QImage::Format_ARGB32_Premultiplied);

    // the original image is pointing up, so we do a rotation
    int rotation = 180;
    if (rotation != 0)
    {
        QTransform transform;
        transform.translate(-image.width() / 2, -image.height() / 2);
        transform.rotate(rotation);
        transform.translate(image.width() / 2, image.height() / 2);
        image = image.transformed(transform);
    }

    return QPixmap::fromImage(image);
}

static void drawArrow(const Style* style, QPainter* painter, const QRect& rect, const QColor& arrowColor)
{
    Q_UNUSED(style);

    QString downArrowCacheKey = QStringLiteral("AzQtComponents::PushButton::DownArrow%1").arg(arrowColor.rgba());
    QPixmap arrowPixmap;
    
    if (!QPixmapCache::find(downArrowCacheKey, &arrowPixmap))
    {
        arrowPixmap = initializeDownArrowPixmap(arrowColor);
        QPixmapCache::insert(downArrowCacheKey, arrowPixmap);
    }

    QRect arrowRect;
    int imageMax = qMin(arrowPixmap.height(), arrowPixmap.width());
    int rectMax = qMin(rect.height(), rect.width());
    int size = qMin(imageMax, rectMax);

    arrowRect.setWidth(size);
    arrowRect.setHeight(size);
    if (arrowPixmap.width() > arrowPixmap.height())
    {
        arrowRect.setHeight(arrowPixmap.height() * size / arrowPixmap.width());
    }
    else
    {
        arrowRect.setWidth(arrowPixmap.width() * size / arrowPixmap.height());
    }

    arrowRect.moveTopLeft(rect.center() - arrowRect.center());

    // force the arrow to the bottom
    arrowRect.moveTop(rect.bottom() - arrowRect.height() - 1);
    arrowRect.moveLeft(rect.left());

    QRectF offsetArrowRect = arrowRect;

    painter->save();
    painter->setRenderHint(QPainter::SmoothPixmapTransform);
    painter->setRenderHint(QPainter::Antialiasing);
    painter->drawPixmap(offsetArrowRect, arrowPixmap, QRectF(QPointF(0, 0), arrowPixmap.size()));
    painter->restore();
}

void PushButton::drawSmallIconArrow(const Style* style, const QStyleOptionToolButton* buttonOption, QStyle::State state, const QRect& inputArea, QPainter* painter, const QWidget* widget, const PushButton::Config& config)
{
    Q_UNUSED(config);
    Q_UNUSED(widget);
    Q_UNUSED(style);

    // non-const version of area rect
    QRect menuArea = inputArea;

    bool isDisabled = !(state & QStyle::State_Enabled);
    QColor color = isDisabled ? config.smallIcon.disabledArrowColor : config.smallIcon.enabledArrowColor;

    bool paintArrow = false;
    if (buttonOption->subControls & QStyle::SC_ToolButtonMenu)
    {
        if (state & (QStyle::State_Sunken | QStyle::State_On | QStyle::State_Raised))
        {
            paintArrow = true;
        }
    }
    else if (buttonOption->features & QStyleOptionToolButton::HasMenu)
    {
        paintArrow = true;
    }

    if (paintArrow)
    {
        drawArrow(style, painter, menuArea, color);
    }
}

void PushButton::drawFilledFrame(QPainter* painter, const QRectF& rect, const QColor& gradientStartColor, const QColor& gradientEndColor, const PushButton::Border& border, float radius)
{
    painter->save();

    QPainterPath path;
    painter->setRenderHint(QPainter::Antialiasing);
    QPen pen(border.color, border.thickness);
    pen.setCosmetic(true);
    painter->setPen(pen);

    // offset the frame so that it smudges and anti-aliases a bit better
    path.addRoundedRect(rect.adjusted(0.5, 0.5, -0.5f, -0.5f), radius, radius);

    QLinearGradient background(rect.topLeft(), rect.bottomLeft());
    background.setColorAt(0, gradientStartColor);
    background.setColorAt(1, gradientEndColor);
    painter->fillPath(path, background);
    painter->drawPath(path);
    painter->restore();
}

void PushButton::selectColors(const QStyleOption* option, const PushButton::ColorSet& colorSet, bool isDisabled, QColor& gradientStartColor, QColor& gradientEndColor)
{
    if (isDisabled)
    {
        gradientStartColor = colorSet.disabled.start;
        gradientEndColor = colorSet.disabled.end;
    }
    else if (option->state & QStyle::State_Sunken)
    {
        gradientStartColor = colorSet.sunken.start;
        gradientEndColor = colorSet.sunken.end;
    }
    else if (option->state & QStyle::State_MouseOver)
    {
        gradientStartColor = colorSet.hovered.start;
        gradientEndColor = colorSet.hovered.end;
    }
    else
    {
        gradientStartColor = colorSet.normal.start;
        gradientEndColor = colorSet.normal.end;
    }
}

} // namespace AzQtComponents
