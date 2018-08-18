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
#ifndef DOCKWIDGETTITLEBAR_H
#define DOCKWIDGETTITLEBAR_H
#pragma once

#include "api.h"
#include <QWidget>
#include <AzCore/std/functional.h>

namespace Ui {
    class DockWidgetTitleBar;
}

class QTabBar;
class QLabel;
class QMenu;
class FluidTabBar;

class EDITOR_QT_UI_API DockWidgetTitleBar
    : public QWidget
{
    Q_OBJECT

public:
    explicit DockWidgetTitleBar(QWidget* parent = 0);
    ~DockWidgetTitleBar();
    void SetupLabel(const QString& text);
    FluidTabBar* SetupFluidTabBar();
    virtual bool eventFilter(QObject* watched, QEvent* event) override;
    QWidget* GetMainControl(){return m_mainControl; }
    FluidTabBar* GetFluidTabBar();
    QLabel* GetLabel();

    void SetShowMenuContextMenuCallback(AZStd::function<QMenu*(void)> callback);
    void ShowMenuButtonContextMenu();

signals:
    void SignalRightClicked(const QPoint& pos);
    void SignalLeftClicked(const QPoint& pos);

public slots:
    void SetTitle(const QString& text); //Only works if m_mainControl is a QLabel

private:
    virtual void paintEvent(QPaintEvent*) override;
    virtual QSize sizeHint() const override;
    virtual QSize minimumSizeHint() const override;

    virtual void mouseReleaseEvent(QMouseEvent* mouseEvent) override;


private:
    Ui::DockWidgetTitleBar* ui;
    bool m_isDraggingDockWidget;
    QWidget* m_mainControl; //Tabbar or label

    AZStd::function<QMenu*(void)> m_showMenuButtonContextMenuCallback;
};

#endif // DOCKWIDGETTITLEBARR_H
