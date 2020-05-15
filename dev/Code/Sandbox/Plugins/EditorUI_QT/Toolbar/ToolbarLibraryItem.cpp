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
#include "ToolbarLibraryItem.h"

#include <QtWidgets/QToolBar>
#include <QtCore/QCoreApplication>

CToolbarLibraryItem::CToolbarLibraryItem(QMainWindow* mainWindow)
    : IToolbar(mainWindow, TO_CSTR(CToolbarLibraryItem))
{
    setWindowTitle("Item");
}

CToolbarLibraryItem::~CToolbarLibraryItem()
{
}

void CToolbarLibraryItem::init()
{
    QAction* action = nullptr;

    action = createAction("actionItemAdd",              "Add",              "Add new item",                     "itemAdd.png");
    action = createAction("actionItemClone",            "Clone",            "Clone library item",               "itemClone.png");
    action = createAction("actionItemRemove",           "Remove",           "Remove item",                      "itemRemove.png");
    action = createAction("actionItemAssign",           "Assign",           "Assign item to selected objects",  "itemAssign.png");
    insertSeparator(action);
    action = createAction("actionItemGetProperties",    "Get Properties",   "Get properties from selection",    "itemGetProperties.png");
    action = createAction("actionItemReload",           "Reload",           "Reload item",                      "itemReload.png");
}