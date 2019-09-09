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

#include <QPointer>
#include <QColor>
#include <QStyle>

class QSettings;

namespace AzQtComponents
{
    class Style;

    /**
     * Class to provide extra functionality for working with ComboBox controls.
     *
     * QComboBox controls are styled in ComboBox.qss
     *
     */
    class AZ_QT_COMPONENTS_API ComboBox
    {
    public:
        struct Config
        {
            int boxShadowXOffset;
            int boxShadowYOffset;
            int boxShadowBlurRadius;
            QColor boxShadowColor;
            QColor placeHolderTextColor;
            QColor framelessTextColor;
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
        static QSize sizeFromContents(const Style* style, QStyle::ContentsType type, const QStyleOption* option, const QSize& size, const QWidget* widget, const ComboBox::Config& config);
        static bool drawComboBox(const Style* style, const QStyleOptionComplex* option, QPainter* painter, const QWidget* widget, const Config& config);
        static bool drawComboBoxLabel(const Style* style, const QStyleOption* option, QPainter* painter, const QWidget* widget, const Config& config);
        static bool drawIndicatorArrow(const Style* style, const QStyleOption* option, QPainter* painter, const QWidget* widget, const Config& config);
    };

} // namespace AzQtComponents
