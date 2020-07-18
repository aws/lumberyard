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
#include "EditorUI_QT_Precompiled.h"
#include "DockWidgetTitleBar.h"
#include <ui_DockWidgetTitleBar.h>
#include <DockWidgetTitleBar.moc>
#include <QHBoxLayout>
#include <QLabel>
#include <QTabBar>
#include <QMouseEvent>
#include "ContextMenu.h"
#include <QPainter>
#include "./FluidTabBar.h"

DockWidgetTitleBar::DockWidgetTitleBar(QWidget* parent)
    : QWidget(parent)
    , ui(new Ui::DockWidgetTitleBar)
    , m_isDraggingDockWidget(false)
    , m_mainControl(nullptr)
{
    ui->setupUi(this);

    connect(ui->btnMenu, &QPushButton::clicked, this, &DockWidgetTitleBar::ShowMenuButtonContextMenu);
    ui->btnMenu->setProperty("iconButton", true);
    ui->btnMenu->setObjectName("DockWidgetContextMenu");

    setMaximumHeight(30);
    setMinimumHeight(30);
    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);

    ui->btnMenu->setVisible(false);
    setContextMenuPolicy(Qt::ContextMenuPolicy::CustomContextMenu);
}

void DockWidgetTitleBar::SetupLabel(const QString& text)
{
    if (m_mainControl != nullptr)
    {
        qWarning("Warning Setting up DockWidgetTitleBar main control when it's already present.");
    }
    QHBoxLayout* l = qobject_cast<QHBoxLayout*>(layout());
    if (l)
    {
        QLabel* lbl = new QLabel(this);
        lbl->setText(text);
        l->insertWidget(1, lbl);
        m_mainControl = lbl;
    }
    else
    {
        qCritical("Error: DockWidgetTitleBar layout is not a QHBoxLayout. Can not setup label.");
    }
}

FluidTabBar* DockWidgetTitleBar::SetupFluidTabBar()
{
    if (m_mainControl != nullptr)
    {
        qWarning("Warning Setting up DockWidgetTitleBar main control when it's already present.");
    }
    QHBoxLayout* l = qobject_cast<QHBoxLayout*>(layout());
    if (l)
    {
        //Do some magic to make the tab bar bottom border join with the titlebar bottom border
        int left, right, top, bottom;
        l->getContentsMargins(&left, &top, &right, &bottom);
        l->setContentsMargins(left, top, right, 0);
        FluidTabBar* tabBar = new FluidTabBar(this);

        QPalette pal = tabBar->palette();
        for (int i = 0; i < 17; i++)
        {
            pal.setColor((QPalette::ColorRole)i, QColor(255, 0, 0, 0));
        }
        tabBar->setPalette(pal);

        tabBar->setObjectName("DockWidgetTitleBarTabs");
        tabBar->setTabsClosable(true);
        tabBar->setMovable(false);
        tabBar->setUsesScrollButtons(true);

        tabBar->installEventFilter(this);

        l->insertWidget(1, tabBar);
        m_mainControl = tabBar;
        return tabBar;
    }
    else
    {
        qCritical("Error: DockWidgetTitleBar layout is not a QHBoxLayout. Can not setup tab bar.");
    }
    return nullptr;
}

DockWidgetTitleBar::~DockWidgetTitleBar()
{
    delete ui;
}


bool DockWidgetTitleBar::eventFilter(QObject* watched, QEvent* event)
{
    //Handle dragging on empty tabbar space
    FluidTabBar* tabBar = qobject_cast<FluidTabBar*>(watched);
    if (tabBar)
    {
        if (event->type() == QEvent::MouseButtonDblClick)
        {
            QMouseEvent* mouseEvent = (QMouseEvent*)event;
            int tab = tabBar->tabAt(mouseEvent->pos());
            if (tab == -1)
            {
                mouseEvent->ignore();
                return true;
            }
        }
        if (event->type() == QEvent::MouseButtonPress)
        {
            QMouseEvent* mouseEvent = (QMouseEvent*)event;
            if (mouseEvent->button() == Qt::LeftButton)
            {
                int tab = tabBar->tabAt(mouseEvent->pos());
                m_isDraggingDockWidget = tab == -1;
                if (m_isDraggingDockWidget)
                {
                    mouseEvent->ignore();
                    return true;
                }
            }
        }
        else if (event->type() == QEvent::MouseButtonRelease)
        {
            QMouseEvent* mouseEvent = (QMouseEvent*)event;
            if (mouseEvent->button() == Qt::LeftButton)
            {
                if (m_isDraggingDockWidget)
                {
                    QMouseEvent* mouseEvent = (QMouseEvent*)event;
                    m_isDraggingDockWidget = false;
                    mouseEvent->ignore();
                    return true;
                }
            }
        }
        else if (event->type() == QEvent::MouseMove && m_isDraggingDockWidget)
        {
            QMouseEvent* mouseEvent = (QMouseEvent*)event;
            mouseEvent->ignore();
            return true;
        }
    }
    return QWidget::eventFilter(watched, event);
}

FluidTabBar* DockWidgetTitleBar::GetFluidTabBar()
{
    return qobject_cast<FluidTabBar*>(m_mainControl);
}
QLabel* DockWidgetTitleBar::GetLabel()
{
    return qobject_cast<QLabel*>(m_mainControl);
}

void DockWidgetTitleBar::SetShowMenuContextMenuCallback(AZStd::function<QMenu*(void)> callback)
{
    m_showMenuButtonContextMenuCallback = callback;
    ui->btnMenu->setVisible(true);
}

void DockWidgetTitleBar::ShowMenuButtonContextMenu()
{
    QMenu* m = m_showMenuButtonContextMenuCallback();
    //Show at the bottom left of the pressed button
    m->exec(ui->btnMenu->parentWidget()->mapToGlobal(ui->btnMenu->pos() + QPoint(0, ui->btnMenu->geometry().height())));
}

void DockWidgetTitleBar::SetTitle(const QString& text)
{
    QLabel* lbl = qobject_cast<QLabel*>(m_mainControl);
    if (lbl)
    {
        lbl->setText(text);
    }
}

void DockWidgetTitleBar::paintEvent(QPaintEvent*)
{
    QStyleOption opt;
    opt.init(this);
    QPainter p(this);
    style()->drawPrimitive(QStyle::PE_Widget, &opt, &p, this);
}

QSize DockWidgetTitleBar::sizeHint() const
{
    return QSize(QWidget::sizeHint().width(), minimumHeight());
}
QSize DockWidgetTitleBar::minimumSizeHint() const
{
    return QSize(QWidget::minimumSizeHint().width(), minimumHeight());
}

void DockWidgetTitleBar::mouseReleaseEvent(QMouseEvent* mouseEvent)
{
    if (mouseEvent->button() == Qt::LeftButton)
    {
        emit SignalLeftClicked(mouseEvent->globalPos());
    }
    else if (mouseEvent->button() == Qt::RightButton)
    {
        emit SignalRightClicked(mouseEvent->globalPos());
    }
    return QWidget::mouseReleaseEvent(mouseEvent);
}
