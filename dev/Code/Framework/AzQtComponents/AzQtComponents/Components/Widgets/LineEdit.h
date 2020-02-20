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
#include <QSize>

class QLineEdit;
class QSettings;
class QPainter;
class QStyleOption;

namespace AzQtComponents
{
    class Style;
    class LineEditWatcher;
    class BrowseEdit;

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
            QColor hoverBackgroundColor;
            QColor hoverBorderColor;
            int hoverLineWidth;
            QColor focusedBorderColor;
            int focusedLineWidth;
            QColor errorBorderColor;
            int errorLineWidth;
            QColor placeHolderTextColor;
            bool clearButtonAutoEnabled;
            QString clearImage;
            QSize clearImageSize;
            QString errorImage;
            QSize errorImageSize;
            int iconSpacing;
            int iconMargin;

            int getLineWidth(const QStyleOption* option, bool hasError) const;
            QColor getBorderColor(const QStyleOption* option, bool hasError) const;
            QColor getBackgroundColor(const QStyleOption* option, bool hasError, const QWidget* widget) const;
        };

        /*!
         * Adds a search icon to the left of the QLineEdit
         */
        static void applySearchStyle(QLineEdit* lineEdit);

        /*!
         * Removes the search icon from the left of the QLineEdit
         */
        static void removeSearchStyle(QLineEdit* lineEdit);

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

        /*!
         * Set the message to display in a tooltip if the QLineEdit validator
         * detects an error.
         */
        static void setErrorMessage(QLineEdit* lineEdit, const QString& error);

        /*!
         * Side buttons (clear/error buttons) are enabled by default. Allow the
         * developer to disable them.
         */
        static void setSideButtonsEnabled(QLineEdit* lineEdit, bool enabled);

    private:
        friend class Style;
        friend class BrowseEdit;
        AZ_PUSH_DISABLE_WARNING(4251, "-Wunknown-warning-option") // needs to have dll-interface to be used by clients of class 'AzQtComponents::LineEdit'
        static QPointer<LineEditWatcher> s_lineEditWatcher;
        AZ_POP_DISABLE_WARNING
        static unsigned int s_watcherReferenceCount;

        static void initializeWatcher();
        static void uninitializeWatcher();

        static bool drawFrame(const Style* style, const QStyleOption* option, QPainter* painter, const QWidget* widget, const Config& config);

        static bool polish(Style* style, QWidget* widget, const Config& config);
        static bool unpolish(Style* style, QWidget* widget, const Config& config);

        static void applyClearButtonStyle(QLineEdit* lineEdit, const Config& config);
        static void applyErrorStyle(QLineEdit* lineEdit, const Config& config);

        static QIcon clearButtonIcon(const QStyleOption* option, const QWidget* widget, const Config& config);

        static bool sideButtonsEnabled(QLineEdit* lineEdit);
    };

} // namespace AzQtComponents
