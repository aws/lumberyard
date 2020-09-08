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

AZ_PUSH_DISABLE_WARNING(4251, "-Wunknown-warning-option") // 4251: class '...' needs to have dll-interface to be used by clients of class '...'
#include <QPointer>
#include <QColor>
#include <QStyle>
#include <QValidator>
AZ_POP_DISABLE_WARNING

class QComboBox;
class QSettings;

namespace AzQtComponents
{
    class Style;
    class ComboBoxWatcher;

    class AZ_QT_COMPONENTS_API ComboBoxValidator
        : public QValidator
    {
        Q_OBJECT

    public:
        explicit ComboBoxValidator(QObject* parent = nullptr)
            : QValidator(parent)
        {

        }

        virtual QValidator::State validateIndex(int) const
        {
            return QValidator::Acceptable;
        };

        QValidator::State validate(QString&, int&) const override
        {
            return QValidator::Acceptable;
        }
    };

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
            QColor placeHolderTextColor;
            QColor framelessTextColor;
            QString errorImage;
            QSize errorImageSize;
            int errorImageSpacing;
            int itemPadding;
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
        * Adds a validator to the checkbox.
        * QComboBox implementation only sets a validator to the underlying
        * QLineEdit, meant for editable QComboBox widgets, and the validator
        * gets deleted together with the owning QLineEdit when the QComboBox
        * widget is set to not editable.
        * This function binds a validator to the QComboBox instead, and it
        * won't be deleted until the QComboBox itself is destroyed.
        */
       static void setValidator(QComboBox* cb, QValidator* validator);

        /*!
        * Forces the ComboBox to set its internal error state to "error"
        */
       static void setError(QComboBox* cb, bool error);

        /*!
         * Applies the CustomCheckState styling to a QCheckBox.
         * With this style applied, the checkmark will be defined by the model,
         * and won't be automatically checked for the current item.
         * Same as
         *   AzQtComponents::Style::addClass(comboBox, "CustomCheckState")
         */
        static void addCustomCheckStateStyle(QComboBox* cb);

    private:
        friend class Style;

        AZ_PUSH_DISABLE_WARNING(4251, "-Wunknown-warning-option") // 4251: class '...' needs to have dll-interface to be used by clients of class '...'
        static QPointer<ComboBoxWatcher> s_comboBoxWatcher;
        AZ_POP_DISABLE_WARNING
        
        static unsigned int s_watcherReferenceCount;

        static void initializeWatcher();
        static void uninitializeWatcher();

        static bool polish(Style* style, QWidget* widget, const Config& config);
        static bool unpolish(Style* style, QWidget* widget, const Config& config);
        static QSize sizeFromContents(const Style* style, QStyle::ContentsType type, const QStyleOption* option, const QSize& size, const QWidget* widget, const ComboBox::Config& config);
        static QRect comboBoxListBoxPopupRect(const Style* style, const QStyleOption* option, const QWidget* widget, const Config& config);
        static bool drawComboBox(const Style* style, const QStyleOptionComplex* option, QPainter* painter, const QWidget* widget, const Config& config);
        static bool drawComboBoxLabel(const Style* style, const QStyleOption* option, QPainter* painter, const QWidget* widget, const Config& config);
        static bool drawIndicatorArrow(const Style* style, const QStyleOption* option, QPainter* painter, const QWidget* widget, const Config& config);
        static bool drawItemCheckIndicator(const Style* style, const QStyleOption* option, QPainter* painter, const QWidget* widget, const Config& config);

        static void addErrorButton(QComboBox* cb, const Config& config);
    };

} // namespace AzQtComponents

Q_DECLARE_METATYPE(AzQtComponents::ComboBoxValidator*)
