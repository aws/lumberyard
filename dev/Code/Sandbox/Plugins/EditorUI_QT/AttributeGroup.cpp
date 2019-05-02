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
#include "AttributeGroup.h"
#include "AttributeView.h"
#include "AttributeListView.h"
#include <AttributeGroup.moc>

#include "AttributeView.h"

// EditorCore
#include <Util/Variable.h>
#include <Util/VariablePropertyType.h>

// QT
#include <QLabel>
#include <QLayout>

#include "Utils.h"
#include "AttributeItem.h"

CAttributeGroup::CAttributeGroup(QWidget* parent, CAttributeItem* content)
    : QWidget(parent)
{
    Init();
    SetContent(content);
}

CAttributeGroup::CAttributeGroup(QWidget* parent)
    : QWidget(parent)
{
    Init();
}

void CAttributeGroup::Init()
{
    m_layout = new QVBoxLayout(this);
    setLayout(m_layout);
    setContentsMargins(QMargins(1, 1, 1, 1));
    m_layout->setMargin(0);
    m_layout->setContentsMargins(0, 0, 0, 0);
    setMouseTracking(true);
    setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Maximum);
    m_attributeConfigItem = nullptr;
    m_attributeConfigGroup = nullptr;
}


CAttributeGroup::~CAttributeGroup()
{
}

void CAttributeGroup::SetContent(CAttributeItem* content)
{
    m_layout->addWidget(content);
    m_content = content;
    content->setParent(this);
}

void CAttributeGroup::SetConfigGroup(const CAttributeViewConfig::config::group* config)
{
    m_attributeConfigGroup = config;
}

void CAttributeGroup::SetConfigItem(const CAttributeViewConfig::config::item* item)
{
    m_attributeConfigItem = item;
}


void CAttributeGroup::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton)
    {
        m_dragStartPoint = event->pos();
        CAttributeItem* previousattribute = (static_cast<CAttributeItem*>(this->parent()->parent()));
        CAttributeView* attributeView = previousattribute->getAttributeView();
        //Set style for multi-selection
        attributeView->AddSelectedAttribute(this);
    }
    QWidget::mousePressEvent(event);
}

void CAttributeGroup::mouseMoveEvent(QMouseEvent* event)
{
    if (!(event->buttons() & Qt::LeftButton))
    {
        return;
    }
    if ((event->pos() - m_dragStartPoint).manhattanLength()
        < QApplication::startDragDistance())
    {
        return;
    }

    CAttributeItem* previousattribute = (static_cast<CAttributeItem*>(this->parent()->parent()));
    CAttributeView* attributeView = previousattribute->getAttributeView();

    attributeView->AddDragAttribute(this);

    QVector<CAttributeGroup*> selectedGroups = attributeView->GetSelectedGroups();

    QDrag* drag = new QDrag(this);
    QMimeData* mimeData = new QMimeData;
    mimeData->setData("AttributeGroup", "");

    int pixHeight = 0;
    for (int i = 0; i < selectedGroups.size(); i++)
    {
        CAttributeGroup* group = selectedGroups[i];
        pixHeight += group->height();
    }

    // Create new picture for transparent
    QPixmap transparent(width(), pixHeight);
    // Do transparency
    transparent.fill(Qt::transparent);
    QPainter p;
    p.begin(&transparent);
    p.setCompositionMode(QPainter::CompositionMode_Source);

    int drawHeight = 0;
    for (int i = 0; i < selectedGroups.size(); i++)
    {
        CAttributeGroup* group = selectedGroups[i];
        p.drawPixmap(0, drawHeight, group->grab(QRect(QPoint(0, 0), QPoint(group->width(), group->height()))));
        drawHeight += group->height();
    }
    p.setCompositionMode(QPainter::CompositionMode_DestinationIn);

    // Set transparency level to 150 (possible values are 0-255)
    // The alpha channel of a color specifies the transparency effect,
    // 0 represents a fully transparent color, while 255 represents
    // a fully opaque color.
    p.fillRect(transparent.rect(), QColor(0, 0, 0, 150));
    p.end();

    drag->setPixmap(transparent);
    drag->setMimeData(mimeData);

    Qt::DropAction dropAction = drag->exec(Qt::CopyAction);

    attributeView->ClearAttributeSelection();
}

// Override the paintEvent so we could use setProperty for widget style
void CAttributeGroup::paintEvent(QPaintEvent*)
{
    QStyleOption opt;
    opt.init(this);
    QPainter p(this);
    style()->drawPrimitive(QStyle::PE_Widget, &opt, &p, this);
}

bool CAttributeGroup::DropAGroup(CAttributeItem* dropAtAttributeItem, QWidget* insertAtGroup, int positionIndex)
{
    CAttributeItem* previousattribute = (static_cast<CAttributeItem*>(parent()->parent()));
    CAttributeListView* previousdockPanel = static_cast<CAttributeListView*>(previousattribute->parentWidget());
    if (dropAtAttributeItem == previousattribute) // the same panel, start re-ordering
    {
        if (previousdockPanel->isCustomPanel()) // Only re-ordering on custom panel
        {
            previousattribute->RemoveDragableGroup(this);
            dropAtAttributeItem->InsertDragableGroup(GetConfigItem(), GetConfigGroup(), insertAtGroup, positionIndex);
            if (previousattribute->getChildren().size() > 0)
            {
                previousattribute->HideDefault();
            }
            else
            {
                previousattribute->ShowDefault();
            }
        }
    }
    else
    {
        if (dropAtAttributeItem->InsertDragableGroup(GetConfigItem(), GetConfigGroup(), insertAtGroup, positionIndex)) // check if insertable
        {
            if (dropAtAttributeItem->getChildren().size() > 0)
            {
                dropAtAttributeItem->HideDefault();
            }
            else
            {
                dropAtAttributeItem->ShowDefault();
            }
            // Remove Item from previous panel. The Item only get removed from custom panel

            if (previousdockPanel->isCustomPanel())
            {
                previousattribute->RemoveDragableGroup(this);
                if (previousattribute->getChildren().size() > 0)
                {
                    previousattribute->HideDefault();
                }
                else
                {
                    previousattribute->ShowDefault();
                }
            }
        }
        else
        {
            QString name = "";
            // Pop error dialog
            if (m_attributeConfigGroup)
            {
                name = m_attributeConfigGroup->name;
            }
            else if (m_attributeConfigItem)
            {
                name = m_attributeConfigItem->name;
            }

            QString text = "Attribute \"" + name + "\" already exists in the panel\n";
            QMessageBox::warning(this, "Warning", text, "Ok");

            //  Clear selected style after drop
            SetDragStyle("None");
            return false;
        }
    }
    //  Clear selected style after drop
    SetDragStyle("None");

    return true;
}

void CAttributeGroup::SetDragStyle(QString DragStatus)
{
    setProperty("DragStatus", DragStatus);
    style()->unpolish(this);
    style()->polish(this);
    update();
}
