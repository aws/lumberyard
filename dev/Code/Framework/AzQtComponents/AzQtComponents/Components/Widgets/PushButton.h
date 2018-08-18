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
#include <QProxyStyle>

class QPushButton;
class QToolButton;
class QSettings;
class QRectF;
class QStyleOptionToolButton;
class QStyleOptionComplex;

namespace AzQtComponents
{
    class Style;

    /**
     * Special class to handle styling and painting of PushButtons.
     * 
     * We have to use default QPushButton's throughout the engine, because QMessageBox's and other built-in
     * Qt things don't provide an easy hook to override the type used.
     * So, this class, full of static member functions, provides a place to collect all of the logic related
     * to painting/styling QPushButton's
     * 
     * The built-in Qt version of QPushButton painting is not very good. The edges of the frame aren't anti-aliased
     * and don't look quite right.
     * 
     * Unfortunately, Qt (as of 5.6) doesn't provide any hooks to query stylesheet values once loaded.
     * To avoid writing and maintaining our own css parsing and rule engine, we just override the painting
     * directly, and allow settings to be customized via an ini file.
     *
     * Some of the styling of QPushButton and QToolButton is done via CSS, in PushButton.qss
     * Everything else that can be configured is in PushButtonConfig.ini
     *
     * There are currently three built-in styles:
     *   Our default QPushButton style
     *   Our Primary QPushButton style, specified via pushButton->setDefault(true);
     *   Small Icon QToolButton style, specified via PushButton::applySmallIconStyle(toolButton);
     *
     */
    class AZ_QT_COMPONENTS_API PushButton
    {
    public:

        struct Gradient
        {
            QColor start;
            QColor end;
        };

        struct ColorSet
        {
            Gradient disabled;
            Gradient sunken;
            Gradient hovered;
            Gradient normal;
        };

        struct Border
        {
            int thickness;
            QColor color;
        };

        struct Frame
        {
            int height;
            int radius;
            int margin;
        };

        struct SmallIcon
        {
            QColor enabledArrowColor;
            QColor disabledArrowColor;
            Frame frame;
            int width;
            int arrowWidth;
        };

        struct Config
        {
            ColorSet primary;
            ColorSet secondary;
            Border defaultBorder;
            Border disabledBorder;
            Border focusedBorder;
            Frame defaultFrame;
            SmallIcon smallIcon;
        };

        /*!
        * Applies the Primary styling to a QPushButton.
        * Explicit way to call QPushButton::setDefault(true);
        */
        static void applyPrimaryStyle(QPushButton* button);

        /*!
        * Applies the SmallIcon styling to a QToolButton.
        * Same as
        *   AzQtComponents::Style::addClass(button, "SmallIcon");
        */
        static void applySmallIconStyle(QToolButton* button);

        /*!
        * Applies the attached button styling to a QToolButton.
        * Same as
        *   AzQtComponents::Style::addClass(button, "AttachedButton");
        */
        static void applyAttachedStyle(QToolButton* button);

        /*!
        * Loads the button config data from a settings object.
        */
        static Config loadConfig(QSettings& settings);

        /*!
        * Returns default button config data.
        */
        static Config defaultConfig();

    private:
        friend class Style;

        // methods used by Style
        static bool polish(Style* style, QWidget* widget, const Config& config);

        static int buttonMargin(const Style* style, const QStyleOption* option, const QWidget* widget, const Config& config);

        static QSize sizeFromContents(const Style* style, QStyle::ContentsType type, const QStyleOption* option, const QSize& size, const QWidget* widget, const Config& config);

        static bool drawPushButtonBevel(const Style* style, const QStyleOption* option, QPainter* painter, const QWidget* widget, const Config& config);

        static bool drawToolButton(const Style* style, const QStyleOptionComplex* option, QPainter* painter, const QWidget* widget, const Config& config);
        static bool drawIndicatorArrow(const Style* style, const QStyleOption* option, QPainter* painter, const QWidget* widget, const Config& config);

        static bool drawPushButtonFocusRect(const Style* style, const QStyleOption* option, QPainter* painter, const QWidget* widget, const Config& config);


        // internal methods
        static void drawSmallIconButton(const Style* style, const QStyleOption* option, QPainter* painter, const QWidget* widget, const Config& config, bool drawFrame = true);
        static void drawSmallIconFrame(const Style* style, const QStyleOption* option, const QRect& frame, QPainter* painter, const Config& config);
        static void drawSmallIconLabel(const Style* style, const QStyleOptionToolButton* buttonOption, QStyle::State state, const QRect& buttonArea, QPainter* painter, const QWidget* widget, const Config& config);
        static void drawSmallIconArrow(const Style* style, const QStyleOptionToolButton* buttonOption, QStyle::State state, const QRect& buttonArea, QPainter* painter, const QWidget* widget, const Config& config);

        static void drawFilledFrame(QPainter* painter, const QRectF& rect, const QColor& gradientStartColor, const QColor& gradientEndColor, const Border& border, float radius);
        static void selectColors(const QStyleOption* option, const ColorSet& colorSet, bool isDisabled, QColor& gradientStartColor, QColor& gradientEndColor);
    };
} // namespace AzQtComponents
