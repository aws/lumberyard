/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates, or 
* a third party where indicated.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,  
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  
*
*/
#include "stdafx.h"
#include "AttributeListView.h"
#include "AttributeItem.h"
#include "AttributeView.h"
#include "AttributeGroup.h"
#include "PanelWidget/paneltitlebar.h"
#include <AttributeListView.moc>

#include <QDebug>
#include <QEvent>

CAttributeListView::CAttributeListView(bool isCustom, QString groupvisibility)
    : QDockWidget()
    , m_isCustom(isCustom)
    , m_previousGroup(nullptr)
    , positionIndex(INSERT_ABOVE)
    , m_GroupVisibility(groupvisibility)
{
    setAcceptDrops(isCustom);
}

CAttributeListView::CAttributeListView(QWidget* parent, bool isCustom, QString groupvisibility)
    : QDockWidget(parent)
    , m_isCustom(isCustom)
    , m_previousGroup(nullptr)
    , positionIndex(INSERT_ABOVE)
    , m_GroupVisibility(groupvisibility)
{
    setAcceptDrops(isCustom);
}

bool CAttributeListView::isShowInGroup()
{
    return m_GroupVisibility == "group" || m_GroupVisibility == "both";
}

bool CAttributeListView::isHideInEmitter()
{
    return m_GroupVisibility == "group";
}

QString CAttributeListView::getGroupVisibility()
{
    return m_GroupVisibility;
}


void CAttributeListView::dragEnterEvent(QDragEnterEvent* event)
{
    if (event->mimeData()->hasFormat("AttributeGroup"))
    {
        event->acceptProposedAction();
    }
}

void CAttributeListView::dragMoveEvent(QDragMoveEvent* event)
{
    PanelTitleBar* bar = static_cast<PanelTitleBar*>(titleBarWidget());
    if (bar->getCollapsed())
    {
        m_previousGroup = nullptr;
        return;
    }
    CAttributeItem* attribute = (static_cast<CAttributeItem*>(this->widget()));
    CRY_ASSERT(attribute);


    QPoint posAttribute = attribute->mapFromParent(event->pos());
    QPoint pos = attribute->GetWidget()->mapFromParent(posAttribute);

    QWidget* widget = attribute->GetWidget()->childAt(pos);
    CAttributeGroup* insertGroup = static_cast<CAttributeGroup*>(widget);

    if (insertGroup)
    {
        // Select the same attribute group as previous group
        if (insertGroup == m_previousGroup)
        {
            return;
        }
        // Reset the style for the previous group
        if (m_previousGroup)
        {
            SetDragStyle(m_previousGroup, "None");
        }


        QVector<CAttributeGroup*> m_selectedGroup = attribute->getAttributeView()->GetSelectedGroups();
        for (CAttributeGroup* grp : m_selectedGroup)
        {
            if (grp == insertGroup) // if insertGroup is a draging group, do not update style
            {
                return;
            }
        }


        // update style for the current insert position.
        int widgetY = insertGroup->pos().y();
        int widgetHeight = insertGroup->height();
        if (pos.y() > widgetY + widgetHeight / 2)
        {
            insertGroup->SetDragStyle("InsertBelow");
            m_previousGroup = insertGroup;
            positionIndex = INSERT_BELOW;
            event->accept();
        }
        else
        {
            insertGroup->SetDragStyle("InsertUp");
            m_previousGroup = insertGroup;
            positionIndex = INSERT_ABOVE;
            event->accept();
        }
    }
    else
    {
        if (m_previousGroup)
        {
            SetDragStyle(m_previousGroup, "None");
        }
        m_previousGroup = nullptr;
        positionIndex = INSERT_ABOVE;
    }
}

void CAttributeListView::dropEvent(QDropEvent* event)
{
    PanelTitleBar* bar = static_cast<PanelTitleBar*>(titleBarWidget());
    if (bar->getCollapsed())
    {
        return;
    }
    event->mimeData();
    CAttributeGroup* varGroup = static_cast<CAttributeGroup*>(event->source());
    CAttributeItem* attribute = (static_cast<CAttributeItem*>(this->widget()));
    QWidget* insertGroup = nullptr;
    if (m_previousGroup)
    {
        CAttributeGroup* groupwidget = static_cast<CAttributeGroup*>(m_previousGroup);
        if (groupwidget)
        {
            insertGroup = groupwidget->GetContent();
        }
    }

    QVector<CAttributeGroup*> selectedGroups = attribute->getAttributeView()->GetSelectedGroups();



    if (positionIndex == INSERT_BELOW) //Insert below
    {
        for (int groupIndex = selectedGroups.size() - 1; groupIndex >= 0; groupIndex--)
        {
            selectedGroups[groupIndex]->DropAGroup(attribute, insertGroup, INSERT_BELOW);
        }
    }
    else// Insert above
    {
        for (int groupIndex = 0; groupIndex < selectedGroups.size(); groupIndex++)
        {
            selectedGroups[groupIndex]->DropAGroup(attribute, insertGroup, INSERT_ABOVE);
        }
    }
    // Clear Selection
    //attribute->getAttributeView()->ClearAttributeSelection();

    if (m_previousGroup)
    {
        SetDragStyle(m_previousGroup, "None");
        m_previousGroup = nullptr;
    }
}

void CAttributeListView::setCustomPanel(bool isCustom)
{
    m_isCustom = isCustom;
    setAcceptDrops(isCustom);
}

bool CAttributeListView::isCustomPanel()
{
    return m_isCustom;
}


// The function is used to set style for QWidget, in this specific case, for
// m_previousGroup. Since m_previousGroup is not a CAttributeGroup and has no
// SetDragStyle function.
void CAttributeListView::SetDragStyle(QWidget* widget, QString dragStatus)
{
    widget->setProperty("DragStatus", dragStatus);
    style()->unpolish(widget);
    style()->polish(widget);
    widget->update();
}