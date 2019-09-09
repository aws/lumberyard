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

#include <QCursor>
#include <QDoubleSpinBox>
#include <QPointer>
#include <QLineEdit>

class QAbstractSpinBox;
class QLineEdit;
class QMouseEvent;
class QPainter;
class QSettings;
class QStyleOption;
class QStyleOptionComplex;
class QAction;
class QProxyStyle;
class QMenu;

namespace AzQtComponents
{
    class Style;
    class SpinBoxWatcher;

    namespace internal
    {
        class SpinBoxLineEdit;
    }

    /**
     * Class to provide extra functionality for working with QSpinBox objects.
     *
     * The SpinBox class provides the ability to connect to a signal for when the
     * value being modified is committed. Similar to how a QLineEdit control has a
     * signal for editingFinished, you can connect to SpinBox::valueChangeEnded, which
     * will be called consistently, regardless of if the value changes as a result of
     * a text change by the user, the mouse wheel turning, the up and down arrow keys or
     * the up and down buttons being clicked.
     *
     * This control also has signals for undo and redo, that can be overridden in the
     * case that a global, or external, undo/redo system should be used.
     * If the lineEdit is being edited (meaning the current text hasn't been committed
     * and the editingFinished signal hasn't been emitted) then the lineEdit's
     * internal undo stack will be used. If the value is committed, and the lineEdit's
     * undo stack is clear, then the signals will be emitted for globalUndoTriggered
     * and globalRedoTriggered.
     * To set the enabled state of the undo and redo right click context menu items,
     * connect to the contextMenuAboutToShow signal and set the state of the parameter
     * actions.
     *
     * This control also changes the cursor to indicate, around the edges of the
     * control, that a left mouse button press and drag to the left or right will
     * change the value.
     *
     * SpinBox controls are styled in SpinBox.qss and configured in SpinBoxConfig.ini
     *
     */
    class AZ_QT_COMPONENTS_API SpinBox
        : public QSpinBox
    {
        Q_OBJECT
    public:
        Q_PROPERTY(bool undoAvailable READ isUndoAvailable)
        Q_PROPERTY(bool redoAvailable READ isRedoAvailable)

        struct Config
        {
            int pixelsPerStep;
            QCursor scrollCursor;
        };

        /*!
         * Loads the button config data from a settings object.
         */
        static Config loadConfig(QSettings& settings);

        /*!
         * Returns default button config data.
         */
        static Config defaultConfig();

        explicit SpinBox(QWidget* parent = nullptr);

        bool isUndoAvailable() const;
        bool isRedoAvailable() const;

    Q_SIGNALS:
        void valueChangeBegan();
        void valueChangeEnded();

        void globalUndoTriggered();
        void globalRedoTriggered();

        /// Always connect to this signal in the main UI thread, with a direct connection; do not use Qt::QueuedConnection or Qt::BlockingQueuedConnection
        /// as the parameters will only be valid for a short time
        void contextMenuAboutToShow(QMenu* menu, QAction* undoAction, QAction* redoAction);

    protected:
        friend class Style;
        friend class EditorProxyStyle;
        friend class SpinBoxWatcher;

        static QPointer<SpinBoxWatcher> s_spinBoxWatcher;
        static unsigned int s_watcherReferenceCount;
        static void initializeWatcher();
        static void uninitializeWatcher();

        static bool drawSpinBox(const QProxyStyle* style, const QStyleOption* option, QPainter* painter, const QWidget* widget, const Config& config);
        static QRect editFieldRect(const QProxyStyle* style, const QStyleOptionComplex* option, const QWidget* widget, const Config& config);

        static bool polish(QProxyStyle* style, QWidget* widget, const Config& config);
        static bool unpolish(QProxyStyle* style, QWidget* widget, const Config& config);

        static void initStyleOption(QAbstractSpinBox* spinBox, QStyleOptionSpinBox* option);

        void focusInEvent(QFocusEvent* e) override;
        void contextMenuEvent(QContextMenuEvent* ev) override;

        internal::SpinBoxLineEdit* m_lineEdit = nullptr;

        friend class DoubleSpinBox;
    };

    /**
     * Class to provide extra functionality for working with QDoubleSpinBox objects.
     *
     * The DoubleSpinBox class provides the ability to connect to a signal for when the
     * value being modified is committed. Similar to how a QLineEdit control has a
     * signal for editingFinished, you can connect to DoubleSpinBox::valueChangeEnded, which
     * will be called consistently, regardless of if the value changes as a result of
     * a text change by the user, the mouse wheel turning, the up and down arrow keys or
     * the up and down buttons being clicked.
     *
     * This control also has signals for undo and redo, that can be overridden in the
     * case that a global, or external, undo/redo system should be used.
     * If the lineEdit is being edited (meaning the current text hasn't been committed
     * and the editingFinished signal hasn't been emitted) then the lineEdit's
     * internal undo stack will be used. If the value is committed, and the lineEdit's
     * undo stack is clear, then the signals will be emitted for globalUndoTriggered
     * and globalRedoTriggered.
     * To set the enabled state of the undo and redo right click context menu items,
     * connect to the contextMenuAboutToShow signal and set the state of the parameter
     * actions.
     *
     * This control also changes the cursor to indicate, around the edges of the
     * control, that a left mouse button press and drag to the left or right will
     * change the value.
     *
     * DoubleSpinBox controls are styled in SpinBox.qss and configured in SpinBoxConfig.ini
     *
     */
    class AZ_QT_COMPONENTS_API DoubleSpinBox
        : public QDoubleSpinBox
    {
        Q_OBJECT
    public:
        enum Option
        {
            SHOW_ONE_DECIMAL_PLACE_ALWAYS, // indicates that zeros after the decimal place should be shown (i.e. 2.00 won't be truncated to 2)
        };
        Q_DECLARE_FLAGS(Options, Option)
        Q_FLAG(Options)

        Q_PROPERTY(bool undoAvailable READ isUndoAvailable)
        Q_PROPERTY(bool redoAvailable READ isRedoAvailable)
        Q_PROPERTY(Options options READ options WRITE setOptions)
        Q_PROPERTY(int displayDecimals READ displayDecimals WRITE setDisplayDecimals)

        explicit DoubleSpinBox(QWidget* parent = nullptr);

        QString textFromValue(double value) const override;

        bool isUndoAvailable() const;
        bool isRedoAvailable() const;

        Options options() const { return m_options; }
        void setOptions(Options options) { m_options = options; }

        int displayDecimals() const { return m_displayDecimals; }
        void setDisplayDecimals(int displayDecimals) { m_displayDecimals = displayDecimals; }

        bool isEditing() const;

    Q_SIGNALS:
        void valueChangeBegan();
        void valueChangeEnded();

        void globalUndoTriggered();
        void globalRedoTriggered();

        /// Always connect to this signal in the main UI thread, with a direct connection; do not use Qt::QueuedConnection or Qt::BlockingQueuedConnection
        /// as the parameters will only be valid for a short time
        void contextMenuAboutToShow(QMenu* menu, QAction* undoAction, QAction* redoAction);

    protected:
        friend class SpinBoxWatcher;

        void focusInEvent(QFocusEvent* e) override;
        void contextMenuEvent(QContextMenuEvent* ev) override;
        QString stringValue(double value, bool truncated = false) const;
        void updateToolTip(double value);

        internal::SpinBoxLineEdit* m_lineEdit = nullptr;
        
        int m_displayDecimals;
        Options m_options;
    };

    namespace internal
    {
        /**
         * Internal class that helps with getting undo and redo to work
         * with SpinBox and DoubleSpinBox. Exposed here so that Qt's moc
         * will run over the class and generate the signal methods.
         *
         * DO NOT USE!
         *
         */
        class SpinBoxLineEdit : public QLineEdit
        {
            Q_OBJECT

        public:
            using QLineEdit::QLineEdit;

            bool event(QEvent* ev) override;
            void keyPressEvent(QKeyEvent* ev) override;

            bool overrideUndoRedo() const;

        Q_SIGNALS:
            void globalUndoTriggered();
            void globalRedoTriggered();
            void selectAllTriggered();
        };
    } // namespace internal
} // namespace AzQtComponents

Q_DECLARE_OPERATORS_FOR_FLAGS(AzQtComponents::DoubleSpinBox::Options)
