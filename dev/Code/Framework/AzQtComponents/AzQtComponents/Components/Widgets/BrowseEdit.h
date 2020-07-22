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
#include <AzQtComponents/Components/Widgets/LineEdit.h>
#include <QFrame>
#include <QIcon>
#include <QString>
#include <QColor>

class QSettings;
class QLineEdit;
class QValidator;
class QStyleOption;

namespace AzQtComponents
{
    class Style;

    class AZ_QT_COMPONENTS_API BrowseEdit
        : public QFrame
    {
        Q_OBJECT //AUTOMOC
        Q_PROPERTY(bool acceptableInput READ hasAcceptableInput NOTIFY hasAcceptableInputChanged)
    public:
        typedef LineEdit::Config Config;

        BrowseEdit(QWidget* parent = nullptr);
        ~BrowseEdit();

        // double click of the read-only line edit will trigger the attached button
        void setAttachedButtonIcon(const QIcon& icon);
        QIcon attachedButtonIcon() const;

        // use a separate button, because UX spec calls for a clear button to be enabled when the line edit is read-only
        bool isClearButtonEnabled() const;
        void setClearButtonEnabled(bool enable);

        bool isLineEditReadOnly() const;
        void setLineEditReadOnly(bool readOnly);

        void setPlaceholderText(const QString& placeholderText);
        QString placeholderText() const;

        void setText(const QString& text);
        QString text() const;

        void setErrorToolTip(const QString& toolTipText);
        QString errorToolTip() const;

        // set the validator
        void setValidator(const QValidator* validator);
        const QValidator* validator() const;
        bool hasAcceptableInput() const;

        // Use this if you need to do non-standard things with the cursor
        QLineEdit* lineEdit() const;

        /*!
        * Loads the config data from a settings object.
        */
        static Config loadConfig(QSettings& settings);

        /*!
        * Returns default config data.
        */
        static Config defaultConfig();

        /*!
         * Displays the browse edit as a drop target. The valid argument indicates
         * whether the line edit is a valid drop target or not.
         */
        static void applyDropTargetStyle(BrowseEdit* browseEdit, bool valid);

        /*!
         * Removes the drop target style from the BrowseEdit.
         */
        static void removeDropTargetStyle(BrowseEdit* browseEdit);

    Q_SIGNALS:
        void attachedButtonTriggered();

        void editingFinished();
        void returnPressed();
        void textChanged(const QString& text);
        void textEdited(const QString& text);
        void hasAcceptableInputChanged(bool acceptable);

    protected:
        bool eventFilter(QObject* watched, QEvent* event) override;
        void mouseDoubleClickEvent(QMouseEvent* event) override;
        bool event(QEvent* event) override;
        void setHasAcceptableInput(bool acceptable);

    private Q_SLOTS:
        void onTextChanged(const QString& text);

    private:
        friend class Style;
        struct InternalData;

        static bool polish(Style* style, QWidget* widget, const Config& config, const LineEdit::Config& lineEditConfig);
        static bool unpolish(Style* style, QWidget* widget, const Config& config, const LineEdit::Config& lineEditConfig);

        static bool drawFrame(const Style* style, const QStyleOption* option, QPainter* painter, const QWidget* widget, const Config& config);

        AZ_PUSH_DISABLE_WARNING(4251, "-Wunknown-warning-option") // 4251: 'AzQtComponents::BrowseEdit::m_data': class 'QScopedPointer<AzQtComponents::BrowseEdit::InternalData,QScopedPointerDeleter<T>>' needs to have dll-interface to be used by clients of class 'AzQtComponents::BrowseEdit'
        QScopedPointer<InternalData> m_data;
        AZ_POP_DISABLE_WARNING
    };

} // namespace AzQtComponents
