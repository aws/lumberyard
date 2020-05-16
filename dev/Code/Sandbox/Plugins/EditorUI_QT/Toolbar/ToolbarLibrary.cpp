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
#include "ToolbarLibrary.h"

#include <QtWidgets/QToolBar>
#include <QtWidgets/QComboBox>

#include <QtCore/QCoreApplication>
#include "IBaseLibraryManager.h"

CToolbarLibrary::CToolbarLibrary(QMainWindow* mainWindow)
    : IToolbar(mainWindow, TO_CSTR(CToolbarLibrary))
{
    setWindowTitle("Library");
    m_ignoreIndexChange = false;
}

CToolbarLibrary::~CToolbarLibrary()
{
}

void CToolbarLibrary::init()
{
    createAction("actionLibraryLoad",   tr("Load"),     tr("Load library"), "libraryLoad.png");
    createAction("actionLibrarySave",   tr("Save"),     tr("Save modified library"), "librarySave.png");
    createAction("actionLibraryAdd",    tr("Add"),      tr("Add library"), "libraryAdd.png");
    createAction("actionLibraryRemove", tr("Remove"), tr("Remove library"), "libraryRemove.png");

    m_list = new QComboBox(this);
    m_list->setFixedWidth(128);
    m_list->setLayoutDirection(Qt::LayoutDirection::LayoutDirectionAuto);
    m_list->addItem("<loading..>");
    addWidget(m_list);

    createAction("actionLibraryReload", tr("Reload"), tr("Reload library"), "libraryReload.png");
}



void CToolbarLibrary::refresh(IBaseLibraryManager* m_pItemManager)
{
    if (!m_pItemManager)
    {
        return;
    }

    // ignore index change event
    m_ignoreIndexChange = true;

    // store old selection
    QString currentSelected;
    if (m_list->currentIndex() != -1)
    {
        currentSelected = m_list->itemText(m_list->currentIndex());
    }

    // erase current list
    m_list->clear();

    for (int i = 0; i < m_pItemManager->GetLibraryCount(); i++)
    {
        // add library
        QString library = m_pItemManager->GetLibrary(i)->GetName();
        m_list->addItem(library);

        // restore old selection
        if (currentSelected == library)
        {
            m_list->setCurrentIndex(m_list->count() - 1);
        }
    }

    m_ignoreIndexChange = false;
}

void CToolbarLibrary::setSelectedLibrary(const char* ident)
{
    m_ignoreIndexChange = true;
    for (int i = 0; i < m_list->count(); i++)
    {
        if (m_list->itemText(i) == ident)
        {
            m_list->setCurrentIndex(i);
        }
    }
    m_ignoreIndexChange = false;
}

QObject* CToolbarLibrary::getObject(const char* identifier)
{
    if (strcmp(identifier, "library_bar") == 0)
    {
        return m_list;
    }
    return nullptr;
}
