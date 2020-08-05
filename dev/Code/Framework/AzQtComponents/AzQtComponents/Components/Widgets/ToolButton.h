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

class QSettings;
class QSize;
class QStyleOption;
class QWidget;

namespace AzQtComponents
{
    class Style;

    class AZ_QT_COMPONENTS_API ToolButton
    {
    public:
        struct Config
        {
            int buttonIconSize;
            int defaultButtonMargin;
            int menuIndicatorWidth;
            QColor checkedStateBackgroundColor;
            QString menuIndicatorIcon;
            QSize menuIndicatorIconSize;
        };

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

        static bool polish(Style* style, QWidget* widget, const Config& config);

        static int buttonIconSize(const Style* style, const QStyleOption* option, const QWidget* widget, const Config& config);
        static int buttonMargin(const Style* style, const QStyleOption* option, const QWidget* widget, const Config& config);
        static int menuButtonIndicatorWidth(const Style* style, const QStyleOption* option, const QWidget* widget, const Config& config);

        static QSize sizeFromContents(const Style* style, QStyle::ContentsType type, const QStyleOption* option, const QSize& contentSize, const QWidget* widget, const Config& config);

        static QRect subControlRect(const Style* style, const QStyleOptionComplex* option, QStyle::SubControl subControl, const QWidget* widget, const Config& config);

        static bool drawToolButton(const Style* style, const QStyleOptionComplex* option, QPainter* painter, const QWidget* widget, const Config& config);
        static bool drawIndicatorArrowDown(const Style* style, const QStyleOption* option, QPainter* painter, const QWidget* widget, const Config& config);

    };
} // namespace AzQtComponents
