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

#include <QPixmap>

class QLineEdit;
class QSettings;
class QPainter;
class QStyleOption;

namespace AzQtComponents
{
    class Style;

    /**
     * Class to provide extra functionality for working with Menu controls.
     *
     * QMenu controls are styled in Menu.qss
     *
     */
    class AZ_QT_COMPONENTS_API Menu
    {
    public:

        struct Margins
        {
            int top;
            int right;
            int bottom;
            int left;
        };

        struct Config
        {
            QPixmap shadowPixmap;
            Margins shadowMargins;
            QColor backgroundColor;
            int radius;
            int horizontalMargin;
            int verticalMargin;
            int horizontalPadding;
            int verticalPadding;
            int subMenuOverlap;
        };

        /*!
        * Loads the config data from a settings object.
        */
        static Config loadConfig(QSettings& settings);

        /*!
        * Returns default config data.
        */
        static Config defaultConfig();

    private:
        friend class Style;

        static bool polish(Style* style, QWidget* widget, const Config& config);
        static bool unpolish(Style* style, QWidget* widget, const Config& config);

        static int horizontalMargin(const Style* style, const QStyleOption* option, const QWidget* widget, const Config& config);
        static int verticalMargin(const Style* style, const QStyleOption* option, const QWidget* widget, const Config& config);
        static int subMenuOverlap(const Style* style, const QStyleOption* option, const QWidget* widget, const Config& config);
        static int verticalShadowMargin(const Style* style, const QStyleOption* option, const QWidget* widget, const Config& config);
        static int horizontalShadowMargin(const Style* style, const QStyleOption* option, const QWidget* widget, const Config& config);

        static bool drawFrame(const Style* style, const QStyleOption* option, QPainter* painter, const QWidget* widget, const Config& config);
    };

} // namespace AzQtComponents
