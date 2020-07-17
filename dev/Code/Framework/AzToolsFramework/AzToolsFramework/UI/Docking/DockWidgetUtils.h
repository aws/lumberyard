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

#ifndef DOCK_WIDGET_UTILS_H
#define DOCK_WIDGET_UTILS_H

// This is a collection of methods to change QDockWidget internals to workaround bugs present in
// 5.6.2, mainly related to restoring floating tabbed windows

#include <QList>

class QObject;
class QWidget;
class QMainWindow;
class QDataStream;
class QDockWidget;

namespace DockWidgetUtils
{

   /**
    * Returns true if w is a QDockWidgetGroupWindow.
    */
    bool isDockWidgetWindowGroup(QWidget* w);

    /**
     * Returns true if the dockwidget dw is inside QDockWidgetGroupWindow.
     */
    bool isInDockWidgetWindowGroup(QDockWidget* dw);

    /**
     * After calling QMainWindow::restoreDockWidget(dw) it can happen that
     * the dockwidget is inside an hidden QDockWidgetGroupWindow, which needs to be shown.
     */
    void correctVisibility(QDockWidget *dw);

    /**
     * Returns true if either obj or one of its children is a QDockWidget.
     * Useful to check if a QDockWidgetGroupWindow has any QDockWidgets.
     */
    bool containsDockWidget(QObject *obj);

    /**
     * Returns a list of QDockWidgetGroupWindow that are direct children of mainWindow.
     */
    QList<QWidget*> getDockWindowGroups(QMainWindow *mainWindow);

    /**
     * Deletes all QDockWidgetGroupWindow.
     * If onlyGhosts is true, then only the ones with no QDockWidget are deleted
     */
    void deleteWindowGroups(QMainWindow *mainWindow, bool onlyGhosts = false);

    /**
     * Prints a list of dock widgets and QDockWidgetGroupWindows to stderr.
     */
    void dumpDockWidgets(QMainWindow *mainWindow);

    /**
     * Calls dumpDockWidgets() every 5 seconds.
     */
    void startPeriodicDebugDump(QMainWindow *mainWindow);

    /**
     * This method is for debugging purposes.
     * Processes the bytearray that contains the saved docking layout, as outputed from QMainWindow::saveState().
     * Returns the dock names that would be restored. Eventually we can think of editing the saved data
     * to fix bugs.
     */
    bool processSavedState(const QByteArray &savedData, QStringList &dockNames);

    /**
     * Looks for non-floating dock widgets that aren't in any dock area because QMainWindow wasn't able to restore it.
     * Should only happen in case of a crash.
     */
    bool hasInvalidDockWidgets(QMainWindow *mainWindow);
}

#endif
