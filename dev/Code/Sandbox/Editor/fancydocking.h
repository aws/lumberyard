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

#include <AzQtComponents/Components/DockTabWidget.h>
#include <AzQtComponents/Components/StyledDockWidget.h>
#include <AzQtComponents/Components/FancyDockingDropZoneWidget.h>

#include "Include/EditorCoreAPI.h"
#include <QtWidgets/QMainWindow>
#include <QColor>
#include <QMetaType>
#include <QPixmap>
#include <QString>
#include <QStringList>
#include <QtCore/QPointer>
#include <QtCore/QScopedPointer>
#include <QtCore/QMap>
#include <QtCore/QVariant>
#include <QScreen>
#include <QSize>
#include <QApplication>

class QDesktopWidget;
class QTimer;

class FloatingDropZone;

namespace AzQtComponents
{
    class FancyDockingGhostWidget;
    class FancyDockingDropZoneWidget;
}

/**
 * This class implements the Visual Sudio docking style for a QMainWindow.
 * To use is, just create an instance of this class and give the QMainWindow as
 * a parent.
 *
 * One shoud use saveState/restoreState/restoreDockWidget on the instance
 * if this class rather than directly on the QMainWindow.
 */
class EDITOR_CORE_API FancyDocking
    : public QWidget
{
    Q_OBJECT
public:
    FancyDocking(QMainWindow* mainWindow);
    ~FancyDocking();

    struct TabContainerType
    {
        QString floatingDockName;
        QStringList tabNames;
        int currentIndex;
    };

    // Grabbing a widget needs both a pixmap and a size, because the size of the pixmap
    // will not take the screen's scaleFactor into account
    struct WidgetGrab
    {
        QPixmap screenGrab;
        QSize size;
    };

    bool restoreState(const QByteArray& layout_data);
    QByteArray saveState();
    bool restoreDockWidget(QDockWidget* dock);
    AzQtComponents::DockTabWidget* tabifyDockWidget(QDockWidget* dropTarget, QDockWidget* dropped, QMainWindow* mainWindow, WidgetGrab* droppedGrab = nullptr);
    void setAbsoluteCornersForDockArea(QMainWindow* mainWindow, Qt::DockWidgetArea area);
    void makeDockWidgetFloating(QDockWidget* dock, const QRect& geometry);
    void splitDockWidget(QMainWindow* mainWindow, QDockWidget* target, QDockWidget* dropped, Qt::Orientation orientation);

    void disableAutoSaveLayout(QDockWidget* dock);
    void enableAutoSaveLayout(QDockWidget* dock);

protected:
    bool eventFilter(QObject* watched, QEvent* event) override;

private Q_SLOTS:
    void onTabIndexPressed(int index);
    void onTabCountChanged(int count);
    void onTabWidgetInserted(QWidget* widget);
    void onUndockTab(int index);
    void onUndockDockWidget();
    void updateDockingGeometry();
    void onDropZoneHoverFadeInUpdate();

    void updateFloatingPixmap();

private:
    typedef QMap<QString, QPair<QStringList, QByteArray> > SerializedMapType;
    typedef QMap<QString, TabContainerType> SerializedTabType;

    // Used to save/restore, history as follows:
    //   5001 - Initial version
    //   5002 - Added custom tab containers
    //   5003 - Extended tab restoration to floating windows
    //   5004 - Got rid of single floating dock widgets, all are now floating
    //          main windows so that they get the extra top bar for repositioning
    //          without engaging docking
    //   5005 - Reworked the m_restoreFloatings to handle restoring floating panes
    //          to their previous location properly since now even single floating
    //          panes are within a floating QMainWindow
    enum
    {
        VersionMarker = 5005
    };

    enum SnappedSide
    {
        SnapLeft = 0x1,
        SnapRight = 0x2,
        SnapTop = 0x4,
        SnapBottom = 0x8
    };

    bool dockMousePressEvent(QDockWidget* dock, QMouseEvent* event);
    bool dockMouseMoveEvent(QDockWidget* dock, QMouseEvent* event);
    bool dockMouseReleaseEvent(QDockWidget* dock, QMouseEvent* event);
    bool canDragDockWidget(QDockWidget* dock, QPoint mousePos);
    void startDraggingWidget(QDockWidget* dock, const QPoint& pressPos, int tabIndex = -1);
    Qt::DockWidgetArea dockAreaForPos(const QPoint& globalPos);
    QWidget* dropTargetForWidget(QWidget* widget, const QPoint& globalPos, QWidget* exclude) const;
    QWidget* dropWidgetUnderMouse(const QPoint& globalPos, QWidget* exclude) const;
    QRect getAbsoluteDropZone(QWidget* dock, Qt::DockWidgetArea& area, const QPoint& globalPos = QPoint());
    void setupDropZones(QWidget* dock, const QPoint& globalPos = QPoint());
    void raiseDockWidgets();
    void dropDockWidget(QDockWidget* dock, QWidget* onto, Qt::DockWidgetArea area);
    QMainWindow* createFloatingMainWindow(const QString& name, const QRect& geometry, bool skipTitleBarDrawing = false);
    AzQtComponents::DockTabWidget* createTabWidget(QMainWindow* mainWindow, QDockWidget* widgetToReplace, QString name = QString());
    QString getUniqueDockWidgetName(const char* prefix);
    void destroyIfUseless(QMainWindow* mw);
    void clearDraggingState();
    QDockWidget* getTabWidgetContainer(QObject* obj);
    void undockDockWidget(QDockWidget* dockWidget, QDockWidget* placeholder = nullptr);
    void SetDragOrDockOnFloatingMainWindow(QMainWindow* mainWindow);
    int NumVisibleDockWidgets(QMainWindow* mainWindow);

    void StartDropZone(QWidget* dropZoneContainer, const QPoint& globalPos);
    void StopDropZone();

    void RepaintFloatingIndicators();
    void SetFloatingPixmapClipping(QWidget* dropOnto, Qt::DockWidgetArea area);

    void AdjustForSnapping(QRect& rect, int cursorScreenIndex);
    bool AdjustForSnappingToScreenEdges(QRect& rect, int screenIndex);
    bool AdjustForSnappingToFloatingWindow(QRect& rect, const QRect& floatingRect);

    bool AnyDockWidgetsExist(QStringList names);

    QMainWindow* m_mainWindow;
    QDesktopWidget* m_desktopWidget;
    QList<QScreen*> m_desktopScreens;

    // An empty QWidget used as a placeholder when dragging a dock window
    // as opposed to creating a new one each time we start dragging a dock widget
    QWidget* m_emptyWidget;

    AzQtComponents::FancyDockingDropZoneState m_dropZoneState;

    // When a user hovers over a drop zone, we will fade it in using this timer
    QTimer* m_dropZoneHoverFadeInTimer;

    struct DragState
    {
        QPointer<QDockWidget> dock;
        QPoint pressPos;
        WidgetGrab dockWidgetScreenGrab;  // Save a screen grab of the widget we are dragging so we can paint it as we drag
        QPointer<AzQtComponents::DockTabWidget> tabWidget;
        QPointer<QWidget> draggedWidget;
        QPointer<QDockWidget> draggedDockWidget;  // This could be different from m_state.dock if the dock widget being dragged is tabbed
        QPointer<QDockWidget> floatingDockContainer;
        int tabIndex = -1;
        bool updateInProgress = false;
        int snappedSide = 0;

        // A setter, so you don't forget to initialize one of them
        void setPlaceholder(const QRect& rect, QScreen* screen)
        {
            m_placeholderScreen = screen;
            m_placeholder = rect;
        }

        // Overload
        void setPlaceholder(const QRect& rect, int screenIndex)
        {
            setPlaceholder(rect, screenIndex == -1 ? nullptr : qApp->screens().at(screenIndex));
        }

        QRect placeholder() const
        {
            return m_placeholder;
        }

        QScreen* placeholderScreen() const
        {
            return m_placeholderScreen;
        }

    private:
        QPointer<QScreen> m_placeholderScreen;
        QRect m_placeholder;
    } m_state;
    
    // map QDockWidget name with its last floating dock.
    QMap<QString, QString> m_placeholders;
    // map floating dock name with it's serialization and the geometry
    QMap<QString, QPair<QByteArray, QRect> > m_restoreFloatings;
    // Map of the last tab container a dock widget was tabbed in (if any)
    QMap<QString, QString> m_lastTabContainerForDockWidget;

    // Map of the last floating screen grab of a dock widget based on its name
    // so we can restore its previous floating size/placeholder image when
    // dragging it around
    QMap<QString, WidgetGrab> m_lastFloatingScreenGrab;

    QScopedPointer<AzQtComponents::FancyDockingGhostWidget> m_ghostWidget;
    QMap<QScreen*, AzQtComponents::FancyDockingDropZoneWidget*> m_dropZoneWidgets;
    QList<AzQtComponents::FancyDockingDropZoneWidget*> m_activeDropZoneWidgets;

    QList<QString> m_orderedFloatingDockWidgetNames;

    friend class FloatingDropZone;
};

Q_DECLARE_METATYPE(FancyDocking::TabContainerType);

