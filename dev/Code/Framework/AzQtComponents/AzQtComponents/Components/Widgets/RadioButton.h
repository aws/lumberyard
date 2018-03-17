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
#include <QStyle>

class QSettings;

namespace AzQtComponents
{
    class Style;

    /**
     * Special class to handle styling and painting of RadioButtons.
     *
     * Some of the styling of QRadioButton are done via CSS, in RadioButton.qss
     * Everything else that can be configured is in RadioButtonConfig.ini
     *
     */
    class AZ_QT_COMPONENTS_API RadioButton
    {
    public:

        struct Config
        {
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

        // methods used by Style
        static bool polish(Style* style, QWidget* widget, const Config& config);

        static QSize sizeFromContents(const Style* style, QStyle::ContentsType type, const QStyleOption* option, const QSize& size, const QWidget* widget, const Config& config);

        static bool drawRadioButton(const Style* style, const QStyleOption* option, QPainter* painter, const QWidget* widget, const Config& config);
        static bool drawRadioButtonLabel(const Style* style, const QStyleOption* option, QPainter* painter, const QWidget* widget, const Config& config);
    };
} // namespace AzQtComponents
