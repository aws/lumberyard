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

#include <QWidget>
#include <QPointer>
#include <QChildEvent>

class QSettings;

namespace AzQtComponents
{
    class TitleBar;

    class AZ_QT_COMPONENTS_API WindowDecorationWrapper
        : public QWidget
    {
        Q_OBJECT

    public:

        enum Option
        {
            OptionNone = 0,
            OptionAutoAttach = 1, // Auto attaches to a widget. To use this feature pass the wrapper as the widget's parent.
                                  // The wrapper will wait for a ChildEvent and add the decorations at that point.
            OptionAutoTitleBarButtons = 2, // Removes minimize and maximize button depending on the guest widget's flags and size constraints
            OptionDisabled = 4 // don't activate the custom title bar at all
        };
        Q_DECLARE_FLAGS(Options, Option)

        explicit WindowDecorationWrapper(Options options = OptionAutoTitleBarButtons, QWidget* parent = nullptr);

        /**
         * Destroys the wrapper and the guest if it's still alive.
         */
        ~WindowDecorationWrapper();

        /**
         * Sets the widget which should get custom window decorations.
         * You can only call this setter once.
         */
        void setGuest(QWidget*);
        QWidget* guest() const;

        /**
         * Returns true if there's a guest widget, which means either one was set with setGuest()
         * or we're using OptionAutoAttach and we got a ChildEvent.
         */
        bool isAttached() const;

        /**
         * Returns the title bar widget.
         */
        TitleBar* titleBar() const;

        /**
         * Enables restoring/saving geometry on creation/destruction.
         */
        void enableSaveRestoreGeometry(const QString& organization,
            const QString& app,
            const QString& key);

        /**
        * Enables restoring/saving geometry on creation/destruction, using the default settings values for organization and application.
        */
        void enableSaveRestoreGeometry(const QString& key);

        /**
         * Returns true if restoring/saving geometry on creation/destruction is enabled.
         * It's disabled by default.
         */
        bool saveRestoreGeometryEnabled() const;

        /**
        * Restores geometry and shows window
        */
        bool restoreGeometryFromSettings();

        QMargins margins() const;

    protected:
        bool event(QEvent* ev) override;
        void paintEvent(QPaintEvent*) override;
        bool eventFilter(QObject* watched, QEvent* event) override;
        void resizeEvent(QResizeEvent* ev) override;
        void childEvent(QChildEvent* ev) override;
        void closeEvent(QCloseEvent* ev) override;
        bool nativeEvent(const QByteArray& eventType, void* message, long* result) override;

    private:
        friend class StyledDockWidget;
        friend class EditorProxyStyle;
        static bool handleNativeEvent(const QByteArray& eventType, void* message, long* result, const QWidget* widget);
        static QMargins win10TitlebarHeight(QWindow *w);
        static Qt::WindowFlags specialFlagsForOS();
        QWidget* topLevelParent();
        void centerInParent();
        bool autoAttachEnabled() const;
        bool autoTitleBarButtonsEnabled() const;
        void updateTitleBarButtons();
        QSize nonContentsSize() const;
        void saveGeometryToSettings();
        void applyFlagsAndAttributes();
        bool canResize() const;
        bool canResizeHeight() const;
        bool canResizeWidth() const;
        void onWindowTitleChanged(const QString& title);
        void adjustTitleBarGeometry();
        void adjustWrapperGeometry();
        void adjustSizeGripGeometry();
        void adjustWidgetGeometry();
        void updateConstraints();
        bool m_initialized = false;
        Options m_options = OptionNone;
        TitleBar* const m_titleBar;
        QWidget* m_guestWidget = nullptr;
        QSettings* m_settings = nullptr;
        QString m_settingsKey;
        bool m_shouldCenterInParent = false;
        bool m_restoringGeometry = false;
    };

    Q_DECLARE_OPERATORS_FOR_FLAGS(WindowDecorationWrapper::Options)
} // namespace AzQtComponents
