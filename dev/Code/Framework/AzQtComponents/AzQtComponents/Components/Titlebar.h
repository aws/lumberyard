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
#include <AzQtComponents/Components/DockBarButton.h>
#include <AzQtComponents/Components/InteractiveWindowGeometryChanger.h>

#include <QWidget>
#include <QPoint>
#include <QPointer>
#include <QTimer>

class QMoveEvent;
class QMouseEvent;
class QMenu;
class QDockWidget;

namespace AzQtComponents
{
    class DockBar;

    class AZ_QT_COMPONENTS_API TitleBar
        : public QWidget
    {
        Q_OBJECT
    public:
        typedef QList<DockBarButton::WindowDecorationButton> WindowDecorationButtons;

        explicit TitleBar(QWidget* parent = nullptr);
        ~TitleBar();
        void setDrawSideBorders(bool);
        void setDrawSimple(bool);
        void setDragEnabled(bool);
        void setTearEnabled(bool);
        QSize sizeHint() const override;
        void setWindowTitleOverride(const QString&);

        /**
         * Sets the titlebar buttons to show.
         * By default shows: | Minimize | Maximize | Close
         *
         * Example:
         *
         * setButtons({ DockBarButton::DividerButton, DockBarButton::MinimizeButton,
         *                        DockBarButton::DividerButton, DockBarButton::MaximizeButton,
         *                        DockBarButton::DividerButton, DockBarButton::CloseButton});
         */
        void setButtons(WindowDecorationButtons);

        void handleClose();
        void handleMaximize();
        void handleMinimize();
        bool hasButton(DockBarButton::WindowDecorationButton) const;
        void handleMoveRequest();
        void handleSizeRequest();

        int numButtons() const;
        void setForceInactive(bool);

        /**
         * For left,right,bottom we use the native Windows border, but for top it's required we add
         * the margin ourselves.
         */
        bool isTopResizeArea(const QPoint& globalPos) const;
        /**
          * These will only return true ever for macOS.
          */
        bool isLeftResizeArea(const QPoint& globalPos) const;
        bool isRightResizeArea(const QPoint& globalPos) const;

        /**
         * The title rect width minus the buttons rect.
         * In local coords.
         */
        QRect draggableRect() const;

        /**
        * Expose the title using a QT property so that test automation can read it
        */
        Q_PROPERTY(QString title READ title)

    Q_SIGNALS:
        void undockAction();

    protected:
        void paintEvent(QPaintEvent* event) override;
        void mousePressEvent(QMouseEvent* ev) override;
        void mouseReleaseEvent(QMouseEvent* ev) override;
        void mouseMoveEvent(QMouseEvent* ev) override;
        void mouseDoubleClickEvent(QMouseEvent *ev) override;
        void timerEvent(QTimerEvent* ev) override;
        void contextMenuEvent(QContextMenuEvent* ev) override;

    protected Q_SLOTS:
        void handleButtonClicked(const DockBarButton::WindowDecorationButton type);

    private:
        bool usesCustomTopBorderResizing() const;
        void checkEnableMouseTracking();
        QWidget* dockWidget() const;
        bool isInDockWidget() const;
        bool isInFloatingDockWidget() const;
        bool isInDockWidgetWindowGroup() const;
        void updateStandardContextMenu();
        void updateDockedContextMenu();
        void fixEnabled();
        bool isMaximized() const;
        QString title() const;
        void setupButtons();
        bool isDragging() const;
        bool isLeftButtonDown() const;
        bool canDragWindow() const;
        bool isResizingWindow() const;
        bool isDraggingWindow() const;
        void resizeWindow(const QPoint& globalPos);
        void dragWindow(const QPoint& globalPos);

        DockBar* m_dockBar;
        QWidget* m_firstButton = nullptr;
        QString m_titleOverride;
        bool m_drawSideBorders = true;
        bool m_drawSimple = false;
        bool m_dragEnabled = false;
        bool m_tearEnabled = false;
        QPoint m_dragPos;
        WindowDecorationButtons m_buttons;
        bool m_forceInactive = false; // So we can show it inactive in the gallery, for demo purposes
        bool m_autoButtons = false;
        bool m_pendingRepositioning = false;
        bool m_resizingTop = false;
        bool m_resizingRight = false;
        bool m_resizingLeft = false;
        qreal m_relativeDragPos = 0.0;
        qreal m_lastLocalPosX = 0.0;
        QMenu* m_contextMenu = nullptr;
        QAction* m_restoreMenuAction = nullptr;
        QAction* m_sizeMenuAction = nullptr;
        QAction* m_moveMenuAction = nullptr;
        QAction* m_minimizeMenuAction = nullptr;
        QAction* m_maximizeMenuAction = nullptr;
        QAction* m_closeMenuAction = nullptr;
        QAction* m_closeGroupMenuAction = nullptr;
        QAction* m_undockMenuAction = nullptr;
        QAction* m_undockGroupMenuAction = nullptr;

        QWindow* topLevelWindow() const;
        void updateMouseCursor(const QPoint& globalPos);
        bool canResize() const;

        Qt::CursorShape m_originalCursor = Qt::ArrowCursor;
        QTimer m_enableMouseTrackingTimer;

        QPointer<InteractiveWindowGeometryChanger> m_interactiveWindowGeometryChanger;
    };
} // namespace AzQtComponents

