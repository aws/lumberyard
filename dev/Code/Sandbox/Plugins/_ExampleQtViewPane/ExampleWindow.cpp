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

#include "stdafx.h"

#include "ExampleWindow.h"
#include <QDockWidget>
// #include <QWebView>

#include "../EditorCommon/QPropertyTree/QPropertyTree.h"
#include "../EditorCommon/QViewport.h"
#include "Include/EditorCoreAPI.h"
#include "Style/StyledDockWidget.h"

CExampleWindow::CExampleWindow()
{
    m_viewport = new QViewport(gEnv, this);
    setCentralWidget(m_viewport);

    // if you need to add UI or QRC files, remember to add them to the .waf_files list too.
    // they will be automatically converted to ui_(filename).h as appropriate, during build.

    QPropertyTree* tree = new QPropertyTree(this);
    tree->attach(Serialization::SStruct(*m_viewport));

    QDockWidget* dock = new StyledDockWidget("Properties");
    dock->setWidget(tree);
    addDockWidget(Qt::RightDockWidgetArea, dock, Qt::Vertical);

    GetIEditor()->RegisterNotifyListener(this);
}

CExampleWindow::~CExampleWindow()
{
    GetIEditor()->UnregisterNotifyListener(this);
}

void CExampleWindow::OnEditorNotifyEvent(EEditorNotifyEvent ev)
{
    if (ev == eNotify_OnIdleUpdate)
    {
        m_viewport->Update();
    }
}