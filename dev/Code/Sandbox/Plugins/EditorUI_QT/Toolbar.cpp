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

#include "Toolbar.h"
#include <Toolbar.moc>

#include "Toolbar/ToolbarLibrary.h"
#include "Toolbar/ToolbarLibraryItem.h"
#include "Toolbar/ToolbarStandard.h"
#include "Toolbar/ToolbarParticleSpecific.h"

#include "../Editor/Resource.h"

IToolbar* IToolbar::createToolBar(QMainWindow* mainWindow, int toolbarID)
{
    IToolbar* toolbar = NULL;

    switch (toolbarID)
    {
    case IDR_DB_LIBRARY_BAR:
        toolbar = new CToolbarLibrary(mainWindow);
        break;
    case IDR_DB_LIBRARY_ITEM_BAR:
        toolbar = new CToolbarLibraryItem(mainWindow);
        break;
    case IDR_DB_STANDART:
        toolbar = new CToolbarStandard(mainWindow);
        break;
    case IDR_DB_PARTICLES_BAR:
        toolbar = new CToolbarParticleSpecific(mainWindow);
        break;
    default:
        break;
    }

    assert(toolbar);

    if (toolbar)
    {
        toolbar->init();
    }

    return toolbar;
}

IToolbar::IToolbar(QMainWindow* mainMindow, const char* objectName)
    : QToolBar(mainMindow)
    , m_MainWindow(mainMindow)
{
    setObjectName(objectName);
    setAutoFillBackground(true);
    setFloatable(false);
    mainMindow->addToolBar(this);
    setIconSize(QSize(30, 30));
}

IToolbar::~IToolbar()
{
}

QAction* IToolbar::getAction(const QString& name)
{
    ActionMap::iterator i = m_Actions.find(name);

    assert(i != m_Actions.end());

    return *i;
}

QAction* IToolbar::createAction(const QString& name, const QString& text, const QString& toolTip, const char* iconFile)
{
    QAction* action = new QAction(text, this);

    if (iconFile)
    {
        // Translate to correct folder
        char iconPath[MAX_PATH];
        sprintf_s(iconPath, "Editor/UI/Icons/toolbar/%s", iconFile);

        // Set icon
        action->setIcon(QIcon(iconPath));
        action->setIconVisibleInMenu(true);
    }

    action->setObjectName(name);
    action->setToolTip(toolTip);
    action->setCheckable(false);

    addAction(action);

    m_Actions.insert(name, action);

    return action;
}
