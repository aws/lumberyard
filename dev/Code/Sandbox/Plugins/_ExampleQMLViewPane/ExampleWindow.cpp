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
#include "ExampleWindow.h"
#include <QDockWidget>
// #include <QWebView>

#include "../EditorCommon/QPropertyTree/QPropertyTree.h"
#include "../EditorCommon/QViewport.h"
#include <QQuickWidget>

CExampleWindow::CExampleWindow()
{
    m_mainWidget = new QQuickWidget(GetIEditor()->GetQMLEngine(), this);
    m_mainWidget->setResizeMode(QQuickWidget::SizeRootObjectToView);
    setCentralWidget(m_mainWidget);

    // an example of an INTERNAL QML resource, which gets packed into the main executable.
    // on the positive side, you don't have to ship anything with your plugin - all its resources are packed into the dll.
    // on the negative, when this QML file is changed you have to rebuild the plugin which contains the resource (WAF will do this automatically if you choose to build)
    m_mainWidget->setSource(QUrl("qrc:/exampleqmlviewpane/data/main.qml"));

    // an example of an EXTERNAL qml.  This does not need a rebuild of the editor or engine
    // on the positive side, you don't have to recompile
    // on the negative side, your QMLs must be placed in this location relative to the root (or another known location)
    //m_mainWidget->setSource(QUrl("Editor/UI/qml/ExampleQMLViewPane/main.qml"));

    GetIEditor()->RegisterNotifyListener(this);
}

CExampleWindow::~CExampleWindow()
{
    delete m_mainWidget;

    GetIEditor()->UnregisterNotifyListener(this);
}

void CExampleWindow::OnEditorNotifyEvent(EEditorNotifyEvent ev)
{
    if (ev == eNotify_OnIdleUpdate)
    {
    }
}
