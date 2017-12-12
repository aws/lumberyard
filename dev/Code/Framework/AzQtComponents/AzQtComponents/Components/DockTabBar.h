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

#include <QTabBar>

class QAction;
class QLabel;
class QMenu;
class QMouseEvent;
class QPropertyAnimation;
class QToolButton;

namespace AzQtComponents
{
    class DockBar;

    class AZ_QT_COMPONENTS_API DockTabBar
        : public QTabBar
    {
        Q_OBJECT
    public:
        explicit DockTabBar(QWidget* parent = nullptr);
        void mouseMoveEvent(QMouseEvent* event);
        void mouseReleaseEvent(QMouseEvent* event);
        void contextMenuEvent(QContextMenuEvent* event) override;
        void finishDrag();
        QSize sizeHint() const override;

    Q_SIGNALS:
        void closeTab(int index);
        void undockTab(int index);

    protected:
        void mousePressEvent(QMouseEvent* event);
        void paintEvent(QPaintEvent* event) override;
        QSize tabSizeHint(int index) const override;
        void tabLayoutChange() override;

    protected Q_SLOTS:
        void handleButtonClicked(const DockBarButton::WindowDecorationButton type);
        void currentIndexChanged();
        void updateCloseButtonPosition();
        void tabPressed(int index);
        void closeTabGroup();
        void dragTabAnimationFinished();
        void displacedTabAnimationFinished();

    private:
        int closeButtonOffsetForIndex(int index) const;
        void setImageForAnimatedLabel(QLabel* label, const QRect& area);
        DockBar* m_dockBar;
        DockBarButton* m_closeTabButton;
        QWidget* m_tabIndicatorUnderlay;
        QLabel* m_draggingTabImage;
        QLabel* m_displacedTabImage;
        QPropertyAnimation* m_dragTabFinishedAnimation;
        QPropertyAnimation* m_displacedTabAnimation;
        QToolButton* m_leftButton;
        QToolButton* m_rightButton;
        QMenu* m_contextMenu;
        QAction* m_closeTabMenuAction;
        QAction* m_closeTabGroupMenuAction;
        QAction* m_undockTabMenuAction;
        QAction* m_undockTabGroupMenuAction;
        int m_menuActionTabIndex;
        int m_tabIndexPressed;
        QPoint m_dragStartPosition;
        bool m_dragInProgress;
        int m_displacedTabIndex;
    };
} // namespace AzQtComponents