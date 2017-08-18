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

#include "StdAfx.h"
#include <ImporterRootDisplay.h>
#include <ui_ImporterRootDisplay.h>
#include <TraceMessageAggregator.h>

#include <IEditor.h>
#include <SceneAPI/SceneCore/Containers/Scene.h>
#include <SceneAPI/SceneUI/CommonWidgets/ReportWidget.h>
#include <SceneAPI/SceneUI/CommonWidgets/OverlayWidget.h>
#include <SceneAPI/SceneUI/SceneWidgets/ManifestWidget.h>
#include <AzCore/Serialization/SerializeContext.h>

ImporterRootDisplay::ImporterRootDisplay(AZ::SerializeContext* serializeContext, QWidget* parent)
    : QWidget(parent)
    , ui(new Ui::ImporterRootDisplay())
    , m_manifestWidget(new AZ::SceneAPI::UI::ManifestWidget(serializeContext))
    , m_hasUnsavedChanges(false)
{
    ui->setupUi(this);
    ui->m_manifestWidgetAreaLayout->addWidget(m_manifestWidget.data());

    ui->m_updateButton->setEnabled(false);
    ui->m_updateButton->setProperty("class", "Primary");

    connect(ui->m_updateButton, &QPushButton::clicked, this, &ImporterRootDisplay::UpdateClicked);
}

ImporterRootDisplay::~ImporterRootDisplay()
{
    delete ui;
}

AZ::SceneAPI::UI::ManifestWidget* ImporterRootDisplay::GetManifestWidget()
{
    return m_manifestWidget.data();
}

void ImporterRootDisplay::SetSceneDisplay(const QString& headerText, const AZStd::shared_ptr<AZ::SceneAPI::Containers::Scene>& scene)
{
    if (!scene)
    {
        AZ_Assert(scene, "No scene provided to display.");
        return;
    }

    ui->m_filePathText->setText(headerText);

    m_manifestWidget->BuildFromScene(scene);

    ui->m_updateButton->setEnabled(false);
    m_hasUnsavedChanges = false;

    connect(m_manifestWidget.data(), &AZ::SceneAPI::UI::ManifestWidget::ManifestUpdated, this, &ImporterRootDisplay::SetToDirtyState);
}

void ImporterRootDisplay::HandleSceneWasReset(const AZStd::shared_ptr<AZ::SceneAPI::Containers::Scene>& scene)
{
    m_manifestWidget->BuildFromScene(scene);
}

void ImporterRootDisplay::HandleSaveWasSuccessful()
{
    ui->m_updateButton->setEnabled(false);
    m_hasUnsavedChanges = false;
}

bool ImporterRootDisplay::HasUnsavedChanges() const
{
    return m_hasUnsavedChanges;
}

void ImporterRootDisplay::SetToDirtyState()
{
    m_hasUnsavedChanges = true;
    ui->m_updateButton->setEnabled(true);
}

#include <ImporterRootDisplay.moc>