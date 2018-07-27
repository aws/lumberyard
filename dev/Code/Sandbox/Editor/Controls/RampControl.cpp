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
// Original file Copyright Crytek GMBH or its affiliates, used under license.

// Description : implementation file


#include "StdAfx.h"
#include "RampControl.h"
#include <algorithm>

#include <QPainter>
#include <QMouseEvent>
#include <QKeyEvent>
#include <QContextMenuEvent>
#include <QMenu>
#include <QAction>
#include <QApplication>

/*****************************************************************************************************/
CRampControl::CRampControl(QWidget* parent)
    : QWidget(parent)
{
    m_max = 5;
    m_ptLeftDown = NULL;
    m_menuPoint = QPoint(0, 0);
    m_movedHandles = false;
    m_canMoveSelected = false;
    m_mouseMoving = false;
}

CRampControl::~CRampControl()
{
}

QSize CRampControl::sizeHint() const
{
    return QSize(240, 50);
}

void CRampControl::paintEvent(QPaintEvent* event)
{
    QWidget::paintEvent(event);

    QPainter dc(this);
    DrawRamp(dc);
    const int handleCount = m_handles.size();
    for (int idx = 0; idx < handleCount; ++idx)
    {
        DrawHandle(dc, m_handles[idx]);
    }
}

void CRampControl::mouseDoubleClickEvent(QMouseEvent* event)
{
    float percent = DeterminePercentage(event->pos());
    if (!HitAnyHandles(percent))
    {
        if (AddHandle(percent))
        {
            emit handleAdded((int)percent);
        }
    }
    update();
}

void CRampControl::mousePressEvent(QMouseEvent* event)
{
    float percent = DeterminePercentage(event->pos());
    if (HitAnyHandles(percent) && !(event->modifiers() & Qt::ControlModifier))
    {
        ClearSelectedHandles();
        emit handleSelectionCleared();
    }

    m_ptLeftDown = new QPoint(event->pos());
    m_canMoveSelected = IsSelectedHandle(percent);
    update();
}

void CRampControl::mouseReleaseEvent(QMouseEvent* event)
{
    float percent = DeterminePercentage(event->pos());
    if (m_movedHandles == false)
    {
        if (!HitAnyHandles(percent) && !(event->modifiers() & Qt::ControlModifier))
        {
            ClearSelectedHandles();
            emit handleSelectionCleared();
        }
        else if (HitAnyHandles(percent))
        {
            if (IsSelectedHandle(percent))
            {
                DeSelectHandle(percent);
            }
            else
            {
                if (SelectHandle(percent))
                {
                    emit handleSelected(/*(int)percent*/);
                }
            }
        }
    }

    if (m_ptLeftDown)
    {
        delete m_ptLeftDown;
    }
    m_ptLeftDown = NULL;

    m_mouseMoving = false;
    m_canMoveSelected = false;
    m_movedHandles = false;
    setFocus();
    releaseMouse();
    update();
}

void CRampControl::mouseMoveEvent(QMouseEvent* event)
{
    float abspercent = DeterminePercentage(event->pos());
    SetHotItem(abspercent);
    update();

    const int handleCount = m_handles.size();
    if (!m_ptLeftDown || handleCount == 0)
    {
        return;
    }

    if (m_movedHandles == false && !m_mouseMoving && !m_canMoveSelected)
    {
        m_canMoveSelected = SelectHandle(abspercent);
    }

    m_mouseMoving = true;

    if (!m_canMoveSelected)
    {
        return;
    }

    grabMouse();

    bool modified = false;

    QPoint movement = event->pos() - *m_ptLeftDown;
    float percent = DeterminePercentage(movement);

    //early out if we hit the edge limits
    for (int idx = 0; idx < handleCount; ++idx)
    {
        if (m_handles[idx].flags & hfSelected && ((movement.x() < 0 && m_handles[idx].percentage <= 1) || (movement.x() > 0 && m_handles[idx].percentage >= 99)))
        {
            return;
        }
    }

    if (movement.x() > 0)
    {
        // move the hanldes clamping in direction where required
        for (int idx = handleCount - 1; idx >= 0; --idx)
        {
            if (m_handles[idx].flags & hfSelected)
            {
                float amountClamped = MoveHandle(m_handles[idx], percent);
                if (amountClamped != 0.0f)
                {
                    percent += amountClamped;
                }
                modified = true;
            }
        }
    }
    else if (movement.x() < 0)
    {
        // move the hanldes clamping in direction where required
        for (int idx = 0; idx < handleCount; ++idx)
        {
            if (m_handles[idx].flags & hfSelected)
            {
                float amountClamped = MoveHandle(m_handles[idx], percent);
                if (amountClamped != 0.0f)
                {
                    percent += amountClamped;
                }
                modified = true;
            }
        }
    }
    *m_ptLeftDown = event->pos();

    if (modified)
    {
        m_movedHandles = true;
        emit handleChanged();
        update();
        SortHandles();
    }
}

void CRampControl::keyPressEvent(QKeyEvent* event)
{
    switch (event->key())
    {
    case Qt::Key_Delete:
        RemoveSelectedHandles();
        emit handleDeleted();
        break;
    case Qt::Key_Left:
        SelectPreviousHandle();
        emit handleSelected();
        break;
    case Qt::Key_Right:
        SelectNextHandle();
        emit handleSelected();
        break;
    default:
        break;
    }

    update();
}

void CRampControl::contextMenuEvent(QContextMenuEvent* event)
{
    m_menuPoint = event->pos();
    float percent = DeterminePercentage(m_menuPoint);
    bool hit = HitAnyHandles(percent);
    bool maxed = GetHandleCount() == GetMaxHandles();

    QMenu menu;
    QAction* action = menu.addAction("&Add");
    action->setData(ID_MENU_ADD);
    action->setEnabled(!maxed);
    menu.addSeparator();
    action = menu.addAction("&Delete");
    action->setData(ID_MENU_DELETE);
    action->setEnabled(hit);
    action = menu.addAction("&Select");
    action->setData(ID_MENU_SELECT);
    action->setEnabled(hit);
    menu.addSeparator();
    menu.addAction("Select &All")->setData(ID_MENU_SELECTALL);
    menu.addAction("Select &None")->setData(ID_MENU_SELECTNONE);
    AddCustomMenuOptions(menu);

    action = menu.exec(QCursor::pos());
    int res = action ? action->data().toInt() : 0;
    switch (res)
    {
    case ID_MENU_ADD:
        OnMenuAdd();
        break;
    case ID_MENU_DELETE:
        OnMenuDelete();
        break;
    case ID_MENU_SELECT:
        OnMenuSelect();
        break;
    case ID_MENU_SELECTALL:
        OnMenuSelectAll();
        break;
    case ID_MENU_SELECTNONE:
        OnMenuSelectNone();
        break;
    default:
        OnMenuCustom(res);
        break;
    }
}

void CRampControl::OnMenuAdd()
{
    float percent = DeterminePercentage(m_menuPoint);
    if (AddHandle(percent))
    {
        emit handleAdded((int)percent);
    }
    update();
}

void CRampControl::OnMenuSelect()
{
    ClearSelectedHandles();
    float percent = DeterminePercentage(m_menuPoint);
    if (SelectHandle(percent))
    {
        emit handleSelected(/*(int)percent*/);
    }
    update();
}

void CRampControl::OnMenuDelete()
{
    ClearSelectedHandles();
    float percent = DeterminePercentage(m_menuPoint);
    SelectHandle(percent);
    RemoveSelectedHandles();
    emit handleDeleted();
    update();
}

void CRampControl::OnMenuSelectAll()
{
    SelectAllHandles();
    emit handleSelected();
    update();
}

void CRampControl::OnMenuSelectNone()
{
    ClearSelectedHandles();
    emit handleSelectionCleared();
    update();
}

/*****************************************************************************************************/
void CRampControl::DrawRamp(QPainter& dc)
{
    QSize rect(size().width() - 1, size().height() - 1);
    DrawBackground(dc);

    dc.setPen(Qt::black);
    QPoint points[2];
    points[0] = QPoint(0, rect.height() / 2);
    points[1] = QPoint(rect.width(), rect.height() / 2);
    dc.drawLine(points[0], points[1]);

    for (int idx = 0; idx < 100; ++idx)
    {
        points[0] = QPoint((int)((((float)(rect.width())) / 100.0f) * idx), rect.height() / 2 - 5);
        points[1] = QPoint(points[0].x(), points[0].y() + 5);

        if (idx % 5 == 0)
        {
            points[0].setY(points[0].y() - 2);
        }

        if (idx % 10 == 0)
        {
            points[0].setY(points[0].y() - 3);
        }

        dc.drawLine(points[0], points[1]);
    }

    DrawForeground(dc);

    if (this == QApplication::focusWidget())
    {
        dc.drawRect(QRect(0, 0, width() - 1, height() - 1));
    }
}

void CRampControl::DrawHandle(QPainter& dc, const HandleData& data)
{
    QSize rect(size().width() - 1, size().height() - 1);
    const float pixelsPercent = (float)rect.width() / 100.0f;

    QPoint handlePoints[2];
    handlePoints[0] = QPoint((int)(pixelsPercent * data.percentage),
            data.flags & hfSelected && m_drawPercentage ? 14 : 0);
    handlePoints[1] = QPoint(handlePoints[0].x(), rect.height());

    QPen linebrush;
    if (data.flags & hfHotitem)
    {
        linebrush.setColor(QColor(0, 0, 0xFF));
    }
    else
    {
        linebrush.setColor(Qt::black);
    }

    dc.setPen(linebrush);
    dc.drawLine(handlePoints[0], handlePoints[1]);

    handlePoints[0].setX(handlePoints[0].x() - 3);
    handlePoints[1].setX(handlePoints[1].x() + 4);
    handlePoints[1].setY(handlePoints[1].y());
    handlePoints[0].setY(handlePoints[1].y() - 7);

    dc.setBrush(Qt::white);
    QRect r(handlePoints[0].x(), handlePoints[0].y(), handlePoints[1].x() - handlePoints[0].x(), handlePoints[1].y() - handlePoints[0].y());
    dc.fillRect(r, Qt::SolidPattern);

    if (data.flags & hfSelected)
    {
        dc.setBrush(Qt::red);
        dc.fillRect(r, Qt::SolidPattern);

        if (m_drawPercentage)
        {
            QString str = QString::number((int)data.percentage);

            handlePoints[0].setX((int)(pixelsPercent * data.percentage) - 7);
            handlePoints[1].setX(handlePoints[1].x() + 5);
            handlePoints[1].setY(0);
            handlePoints[0].setY(10);

            dc.drawText(QRect(handlePoints[0].x(), handlePoints[0].y(), handlePoints[1].x() - handlePoints[0].x(), handlePoints[1].y() - handlePoints[0].y()), str);
        }
    }
}

void CRampControl::DrawBackground(QPainter& dc)
{
}

void CRampControl::DrawForeground(QPainter& dc)
{
}

/*****************************************************************************************************/

void CRampControl::AddCustomMenuOptions(QMenu& menu)
{
}

void CRampControl::OnMenuCustom(UINT nID)
{
}

/*****************************************************************************************************/
float CRampControl::DeterminePercentage(const QPoint& point)
{
    QSize sz = size();
    float percent = ((float)(point.x()) / (float)sz.width()) * 100.0f;
    return percent;
}

bool CRampControl::AddHandle(const float percent)
{
    if (m_handles.size() >= m_max)
    {
        return false;
    }

    HandleData data;
    data.percentage = percent;
    data.flags = 0;
    m_handles.push_back(data);

    SortHandles();
    return true;
}

void CRampControl::SortHandles()
{
    std::sort(m_handles.begin(), m_handles.end(), SortHandle);
}

bool CRampControl::SortHandle(const CRampControl::HandleData& dataA, const CRampControl::HandleData& dataB)
{
    return dataA.percentage < dataB.percentage;
}

void CRampControl::ClearSelectedHandles()
{
    const int handleCount = m_handles.size();
    for (int idx = 0; idx < handleCount; ++idx)
    {
        m_handles[idx].flags &= ~hfSelected;
    }
}

void CRampControl::Reset()
{
    m_handles.clear();
}

bool CRampControl::SelectHandle(const float percent)
{
    const int handleCount = m_handles.size();
    for (int idx = 0; idx < handleCount; ++idx)
    {
        if (HitTestHandle(m_handles[idx], percent) && !(m_handles[idx].flags & hfSelected))
        {
            m_handles[idx].flags |= hfSelected;
            return true;
        }
    }
    return false;
}

bool CRampControl::DeSelectHandle(const float percent)
{
    const int handleCount = m_handles.size();
    for (int idx = 0; idx < handleCount; ++idx)
    {
        if (HitTestHandle(m_handles[idx], percent) && (m_handles[idx].flags & hfSelected))
        {
            m_handles[idx].flags &= ~hfSelected;
            return true;
        }
    }
    return false;
}

bool CRampControl::IsSelectedHandle(const float percent)
{
    const int handleCount = m_handles.size();
    for (int idx = 0; idx < handleCount; ++idx)
    {
        if (HitTestHandle(m_handles[idx], percent))
        {
            if (m_handles[idx].flags & hfSelected)
            {
                return true;
            }
        }
    }
    return false;
}

void CRampControl::SelectAllHandles()
{
    const int handleCount = m_handles.size();
    for (int idx = 0; idx < handleCount; ++idx)
    {
        m_handles[idx].flags |= hfSelected;
    }
}

float CRampControl::MoveHandle(HandleData& data, const float percent)
{
    float clampedby = 0.0f;
    data.percentage += percent;
    if (data.percentage < 0.0f)
    {
        clampedby = -data.percentage;
        data.percentage = 0.0f;
    }

    if (data.percentage > 99.99f)
    {
        clampedby = -(data.percentage - 99.0f);
        data.percentage = 99.0f;
    }
    return clampedby;
}

bool CRampControl::HitAnyHandles(const float percent)
{
    const int handleCount = m_handles.size();
    for (int idx = 0; idx < handleCount; ++idx)
    {
        if (HitTestHandle(m_handles[idx], percent))
        {
            return true;
        }
    }
    return false;
}

bool CRampControl::HitTestHandle(const HandleData& data, const float percent)
{
    if (percent > data.percentage - 1 && percent < data.percentage + 1)
    {
        return true;
    }
    return false;
}

bool CRampControl::CheckSelected(const CRampControl::HandleData& data)
{
    return data.flags & CRampControl::hfSelected;
}

void CRampControl::RemoveSelectedHandles()
{
    m_handles.erase(remove_if(m_handles.begin(), m_handles.end(), CRampControl::CheckSelected), m_handles.end());
    SortHandles();
}

void CRampControl::SelectNextHandle()
{
    bool selected = false;
    const int count = m_handles.size();
    for (int idx = count - 1; idx >= 0; --idx)
    {
        if ((m_handles[idx].flags & hfSelected) && (idx + 1 < count))
        {
            m_handles[idx + 1].flags |= hfSelected;
            selected = true;
        }
        m_handles[idx].flags &= ~hfSelected;
    }

    if (selected == false && count > 0)
    {
        m_handles[0].flags |= hfSelected;
    }
}

void CRampControl::SelectPreviousHandle()
{
    bool selected = false;
    const int count = m_handles.size();
    for (int idx = 0; idx < count; ++idx)
    {
        if ((m_handles[idx].flags & hfSelected) && (idx - 1 >= 0))
        {
            m_handles[idx - 1].flags |= hfSelected;
            selected = true;
        }
        m_handles[idx].flags &= ~hfSelected;
    }

    if (selected == false && count > 0)
    {
        m_handles[count - 1].flags |= hfSelected;
    }
}

/*************************************************************************************/
const bool CRampControl::GetSelectedData(CRampControl::HandleData& data)
{
    const int handleCount = m_handles.size();
    for (int idx = 0; idx < handleCount; ++idx)
    {
        if (m_handles[idx].flags & hfSelected)
        {
            data = m_handles[idx];
            return true;
        }
    }
    return false;
}

const int CRampControl::GetHandleCount()
{
    return m_handles.size();
}

const CRampControl::HandleData CRampControl::GetHandleData(const int idx)
{
    return m_handles[idx];
}

const int CRampControl::GetHotHandleIndex()
{
    const int count = m_handles.size();
    for (int idx = 0; idx < count; ++idx)
    {
        if (m_handles[idx].flags & hfHotitem)
        {
            return idx;
        }
    }
    return -1;
}

void CRampControl::SetHotItem(const float percentage)
{
    const int count = m_handles.size();
    for (int idx = 0; idx < count; ++idx)
    {
        if (HitTestHandle(m_handles[idx], percentage))
        {
            m_handles[idx].flags |= hfHotitem;
        }
        else
        {
            m_handles[idx].flags &= ~hfHotitem;
        }
    }
}

#include <Controls/RampControl.moc>