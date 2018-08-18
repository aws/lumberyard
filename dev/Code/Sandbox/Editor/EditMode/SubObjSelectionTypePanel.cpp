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

#include "StdAfx.h"

#include <QIcon>
#include <QListWidgetItem>

#include "SubObjSelectionTypePanel.h"
#include "Objects/SubObjSelection.h"

#include <EditMode/ui_SubObjSelectionTypePanel.h>

//////////////////////////////////////////////////////////////////////////
/// CXTPTaskPanelOffice2003ThemePlain

const int SELECTION_TYPE_ITEM_HEIGHT = 18;

// CSubObjSelectionTypePanel dialog

static QString SubObjElementTypeToString(ESubObjElementType type)
{
    switch (type)
    {
    case SO_ELEM_VERTEX:
    {
        return "vertex";
    }
    case SO_ELEM_EDGE:
    {
        return "edge";
    }
    case SO_ELEM_FACE:
    {
        return "face";
    }
    case SO_ELEM_POLYGON:
    {
        return "polygon";
    }
    default:
    {
        return QString();
    }
    }
}

static QStringList SelectionTypeList()
{
    QStringList selectionTypeNames;
    for (int i = SO_ELEM_NONE; i != SO_ELEM_UV; ++i)
    {
        const QString typeName = SubObjElementTypeToString((ESubObjElementType)i);
        if (!typeName.isEmpty())
        {
            selectionTypeNames << typeName;
        }
    }
    return selectionTypeNames;
}

CSubObjSelectionTypePanel::CSubObjSelectionTypePanel(QWidget* pParent /*=nullptr*/)
    : QWidget(pParent)
    , ui(new Ui::CSubObjSelectionTypePanel)
{
    ui->setupUi(this);

    // Filling the list with all the selection types and their icons
    const QString iconPrefix = ":/subObjSelectionType/";
    const QStringList typeNamesList = SelectionTypeList();
    for (int i = 0; i < typeNamesList.size(); ++i)
    {
        QString iconAlias = iconPrefix + typeNamesList.at(i);
        typeNamesList.at(i)[0].toUpper();
        QListWidgetItem* item = new QListWidgetItem(QIcon(iconAlias), typeNamesList.at(i));
        ui->SELECTION_TYPE_LIST_WIDGET->addItem(item);
    }

    ui->SELECTION_TYPE_LIST_WIDGET->setCurrentItem(ElementTypeToListItem(SO_ELEM_VERTEX));
    ui->SELECTION_TYPE_LIST_WIDGET->setMaximumHeight(ui->SELECTION_TYPE_LIST_WIDGET->count() * SELECTION_TYPE_ITEM_HEIGHT);

    connect(ui->SELECTION_TYPE_LIST_WIDGET, &QListWidget::currentItemChanged, this, &CSubObjSelectionTypePanel::ChangeSelectionType);
}

CSubObjSelectionTypePanel::~CSubObjSelectionTypePanel()
{
}

//////////////////////////////////////////////////////////////////////////
void CSubObjSelectionTypePanel::SelectElemtType(int nIndex)
{
    QListWidgetItem* item = ElementTypeToListItem((ESubObjElementType)nIndex);
    if (item != nullptr)
    {
        ui->SELECTION_TYPE_LIST_WIDGET->setCurrentItem(item);
    }
}

void CSubObjSelectionTypePanel::ChangeSelectionType(QListWidgetItem* current, QListWidgetItem* previous)
{
    // offset of 1 to be able to compare with the enum, starting with SO_ELEM_NONE at 0
    ESubObjElementType elementTypeEnum = ListItemToElementType(current);

    switch (elementTypeEnum)
    {
    case SO_ELEM_VERTEX:
    {
        CCryEditApp::SubObjectModeVertex();
    }
    break;

    case SO_ELEM_EDGE:
    {
        CCryEditApp::SubObjectModeEdge();
    }
    break;

    case SO_ELEM_FACE:
    {
        CCryEditApp::SubObjectModeFace();
    }
    break;

    case SO_ELEM_POLYGON:
    {
        CCryEditApp::SubObjectModePivot();
    }
    break;
    }
}

ESubObjElementType CSubObjSelectionTypePanel::ListItemToElementType(QListWidgetItem* current)
{
    for (int i = SO_ELEM_NONE; i != SO_ELEM_UV; ++i)
    {
        if (current->data(Qt::DisplayRole).toString() == SubObjElementTypeToString((ESubObjElementType)i))
        {
            return (ESubObjElementType)i;
        }
    }

    return SO_ELEM_NONE;
}

QListWidgetItem* CSubObjSelectionTypePanel::ElementTypeToListItem(ESubObjElementType elementType)
{
    for (int i = 0; i < ui->SELECTION_TYPE_LIST_WIDGET->count(); ++i)
    {
        QListWidgetItem* item = ui->SELECTION_TYPE_LIST_WIDGET->item(i);
        if (item->data(Qt::DisplayRole).toString() == SubObjElementTypeToString(elementType))
        {
            return item;
        }
    }
    return nullptr;
}
#include <EditMode/SubObjSelectionTypePanel.moc>