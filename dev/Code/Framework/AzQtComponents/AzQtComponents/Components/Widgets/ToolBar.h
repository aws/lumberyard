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

#include <QColor>

class QSettings;
class QStyleOption;
class QToolBar;
class QWidget;

namespace AzQtComponents
{
    class Style;

    /**
     * Class to provide extra functionality for working with QToolBar controls.
     *
     * QToolBar controls are styled in QToolBar.qss
     *
     */
    class AZ_QT_COMPONENTS_API ToolBar
    {
    public:

        struct ToolBarConfig
        {
            int itemSpacing;

            QColor backgroundColor;
            qreal borderWidth;
            QColor borderColor;
        };

        struct Config
        {
            ToolBarConfig primary;
            ToolBarConfig secondary;

            int defaultSpacing;
        };

        enum class ToolBarIconSize
        {
            IconNormal,
            IconLarge,
            Default = IconNormal
        };

        /*!
        * Loads the config data from a settings object.
        */
        static Config loadConfig(QSettings& settings);

        /*!
        * Returns default config data.
        */
        static Config defaultConfig();

        /*!
        * Applies the MainToolBar styling to a QToolBar.
        * Same as
        *   AzQtComponents::Style::addClass(toolBar, "MainToolBar");
        */
        static void addMainToolBarStyle(QToolBar* toolbar);

        /*!
        * Applies the proper class to a QToolBar given an icon size.
        * Same as
        *   AzQtComponents::Style::addClass(toolBar, "{IconNormal, IconLarge}");
        */
        static void setToolBarIconSize(QToolBar* toolbar, ToolBarIconSize size);

    private:
        friend class Style;
        friend class EditorProxyStyle;

        static bool polish(Style* style, QWidget* widget, const Config& config);

        static int itemSpacing(const Style* style, const QStyleOption* option, const QWidget* widget, const Config& config);

        static constexpr const int g_ui10DefaultIconSize = 20;
        static constexpr const int g_ui10LargeIconSize = 32;

        static constexpr const char* g_ui10IconSizeProperty = "Ui10IconSize";
    };

} // namespace AzQtComponents
