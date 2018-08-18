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

class QLineEdit;
class QSettings;
class QPainter;
class QStyleOption;

namespace AzQtComponents
{
    class Style;
    class LineEditWatcher;

    /**
     * Class to provide extra functionality for working with Line Edit controls.
     *
     * QLineEdit controls are styled in LineEdit.qss
     *
     */
    class AZ_QT_COMPONENTS_API LineEdit
    {
    public:
        struct Config
        {
            int borderRadius;
            QColor borderColor;
            QColor focusedBorderColor;
            QColor errorBorderColor;
            QColor placeHolderTextColor;
        };

        /*!
         * Adds a search icon to the left of the QLineEdit
         */
        static void applySearchStyle(QLineEdit* lineEdit);

        /*!
        * Uses only the stylesheet file values, does not draw borders
        */
        static void applyDefaultStyle(QLineEdit* lineEdit);

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
        static QPointer<LineEditWatcher> m_lineEditWatcher;

        static void initializeWatcher();
        static void uninitializeWatcher();

        static bool drawFrame(const Style* style, const QStyleOption* option, QPainter* painter, const QWidget* widget, const Config& config);

        static bool polish(Style* style, QWidget* widget, const Config& config);
        static bool unpolish(Style* style, QWidget* widget, const Config& config);

        static QIcon clearButtonIcon(const QStyleOption* option, const QWidget* widget);
    };

} // namespace AzQtComponents
