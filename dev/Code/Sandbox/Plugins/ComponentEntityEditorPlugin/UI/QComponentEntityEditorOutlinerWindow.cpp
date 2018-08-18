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

#include "stdafx.h"
#include "CryEdit.h"
#include "UI/QComponentEntityEditorOutlinerWindow.h"
#include "UI/Outliner/OutlinerWidget.hxx"

#include <AzCore/Component/Entity.h>
#include <AzCore/Component/ComponentApplicationBus.h>

#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <AzToolsFramework/UI/PropertyEditor/ReflectedPropertyEditor.hxx>
#include <AzToolsFramework/UI/PropertyEditor/EntityPropertyEditor.hxx>

QComponentEntityEditorOutlinerWindow::QComponentEntityEditorOutlinerWindow(QWidget* parent)
    : QMainWindow(parent)
    , m_outlinerWidget(nullptr)
{
    gEnv->pSystem->GetISystemEventDispatcher()->RegisterListener(this);

    Init();
}

QComponentEntityEditorOutlinerWindow::~QComponentEntityEditorOutlinerWindow()
{
    gEnv->pSystem->GetISystemEventDispatcher()->RemoveListener(this);
}

void QComponentEntityEditorOutlinerWindow::OnSystemEvent(ESystemEvent event, UINT_PTR wparam, UINT_PTR lparam)
{
}

void QComponentEntityEditorOutlinerWindow::Init()
{
    QVBoxLayout* layout = new QVBoxLayout();

    m_outlinerWidget = new OutlinerWidget(nullptr);
    layout->addWidget(m_outlinerWidget);

    QWidget* window = new QWidget();
    window->setLayout(layout);
    setCentralWidget(window);
}

///////////////////////////////////////////////////////////////////////////////
// End of context menu handling
///////////////////////////////////////////////////////////////////////////////

#include <UI/QComponentEntityEditorOutlinerWindow.moc>

