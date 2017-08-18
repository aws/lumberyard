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
#include "IEditor.h"

#include <PermissionsAndDeployments.h>

#include <QScrollArea>

#include <PermissionsAndDeployments.moc>
#include <LmbrAWS/ILmbrAWS.h>
#include <LmbrAWS/IAWSClientManager.h>


PermissionsAndDeployments::PermissionsAndDeployments(QWidget* parent)
    : QMainWindow(parent)
    , m_clientManager(gEnv->pLmbrAWS->GetClientManager())
    , m_mainWidget()
    , m_mainVerticalLayout()
    , m_buttonLayout()
    , m_closeButton("Close")
{
    // These two bailouts are to prevent a bad case of the window opening due to saved registry values before the system is fully initialized.
    // We haven't even initialized python yet and this can lead to bad things happening.
    // This will be fixed in the near future
    if (!GetIEditor())
    {
        return;
    }
    if (!GetIEditor()->IsInitialized())
    {
        CloseWindow();
        return;
    }

    m_tabWidget.addTab(&m_credentialsEditorTab, "Authentication");
    m_tabWidget.addTab(&m_activeDeploymentTab, "Deployments");

    m_mainVerticalLayout.addWidget(&m_tabWidget);

    m_mainVerticalLayout.addWidget(&m_closeButton, 0, Qt::AlignRight);
    m_mainVerticalLayout.setSizeConstraint(QLayout::SetFixedSize);

    QWidget::connect(&m_closeButton, &QPushButton::clicked, this, &PermissionsAndDeployments::CloseWindow);
    QWidget::connect(&m_credentialsEditorTab, &QAWSCredentialsEditor::LayoutChanged, this, &PermissionsAndDeployments::HandleResize);

    m_mainWidget.setLayout(&m_mainVerticalLayout);

    QScrollArea* scrollArea = new QScrollArea();
    scrollArea->setWidget(&m_mainWidget);
    scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    setCentralWidget(scrollArea);
}

void PermissionsAndDeployments::CloseWindow()
{
    close();
    GetIEditor()->CloseView(GetPaneName());
}

void PermissionsAndDeployments::HandleResize()
{
    // Handle resizing more elegantly in the future
}