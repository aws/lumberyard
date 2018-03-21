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

// Description : implementation file


#include "StdAfx.h"
#include "SkyDialog.h"
#include "CryEditDoc.h"
#include "Terrain/GenerationParam.h"
#include "Terrain/Clouds.h"
#include <ui_SkyDialog.h>

#include <QPainter>
#include <QDebug>

class CloudPreviewWidget
    : public QWidget
{
public:
    CloudPreviewWidget(QWidget* pParent = nullptr);
protected:
    const int kSize = 256;
    void paintEvent(QPaintEvent* event) override;
};

CloudPreviewWidget::CloudPreviewWidget(QWidget* pParent)
    : QWidget(pParent)
{
    setFixedSize(kSize, kSize);
}

void CloudPreviewWidget::paintEvent(QPaintEvent* event)
{
    QPainter painter(this);
    QRect rcDest(0, 0, kSize, kSize);
    GetIEditor()->GetDocument()->GetClouds()->DrawClouds(painter, rcDest);
}


/////////////////////////////////////////////////////////////////////////////
// CSkyDialog dialog


CSkyDialog::CSkyDialog(QWidget* pParent /*=NULL*/)
    : QDialog(pParent)
    , m_initialized(false)
    , ui(new Ui::CSkyDialog)
{
    ui->setupUi(this);
    ui->m_propWnd->Setup();

    m_preview = new CloudPreviewWidget(ui->m_previewFrame);

    m_PD = PDNorth;

    connect(ui->actionSkyClouds, &QAction::triggered, this, &CSkyDialog::OnSkyClouds);
    connect(ui->actionSkyCloudsMenu, &QAction::triggered, this, &CSkyDialog::OnSkyClouds);
    connect(ui->actionSkyNorth, &QAction::triggered, this, &CSkyDialog::OnSkyNorth);
    connect(ui->actionSkySouth, &QAction::triggered, this, &CSkyDialog::OnSkySouth);
    connect(ui->actionSkyWest, &QAction::triggered, this, &CSkyDialog::OnSkyWest);
    connect(ui->actionSkyEast, &QAction::triggered, this, &CSkyDialog::OnSkyEast);
    connect(ui->buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
}

CSkyDialog::~CSkyDialog()
{
}

void CSkyDialog::OnInitDialog()
{
    XmlNodeRef& templ = GetIEditor()->GetDocument()->GetFogTemplate();
    if (templ)
    {
        XmlNodeRef rootNode = GetIEditor()->GetDocument()->GetEnvironmentTemplate();

        ui->m_propWnd->CreateItems(rootNode);
        ui->m_propWnd->ExpandAll();
    }
}

void CSkyDialog::OnSkyClouds()
{
    ////////////////////////////////////////////////////////////////////////
    // Generate new clouds
    ////////////////////////////////////////////////////////////////////////

    SNoiseParams sParams;
    CGenerationParam cDialog;

    if (GetIEditor()->GetDocument()->GetClouds()->GetLastParam()->bValid)
    {
        // Use last parameters
        cDialog.LoadParam(*GetIEditor()->GetDocument()->GetClouds()->GetLastParam());
    }
    else
    {
        // Set default parameters for the dialog
        SNoiseParams sDefaultParams;
        sDefaultParams.iPasses = 8;
        sDefaultParams.fFrequency = 3.0f;
        sDefaultParams.fFrequencyStep = 2.0f;
        sDefaultParams.fFade = 0.8f;
        sDefaultParams.iCover = 50;
        sDefaultParams.iRandom = 1;
        sDefaultParams.iSharpness = 0.999f;
        sDefaultParams.iSmoothness = 0;

        cDialog.LoadParam(sDefaultParams);
    }

    // Show the generation parameter dialog
    if (cDialog.exec() == QDialog::Rejected)
    {
        return;
    }

    CLogFile::WriteLine("Generating cloud layer...");

    // Fill the parameter structure for the cloud generation
    cDialog.FillParam(sParams);
    sParams.iWidth = 512;
    sParams.iHeight = 512;
    sParams.bBlueSky = true;

    QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

    // Call the generator function
    GetIEditor()->GetDocument()->GetClouds()->GenerateClouds(&sParams, ui->m_genStatus);

    // Update the window
    m_preview->update();

    // Remove the status indicator
    ui->m_genStatus->setText(QString());

    // We modified the document
    GetIEditor()->GetDocument()->SetModifiedFlag(TRUE);

    QApplication::restoreOverrideCursor();

    ui->m_propWnd->RebuildCtrl();
}


/////////////////////////////////////////////////////////////////////////////
// CSkyDialog direction buttons

void CSkyDialog::OnSkyNorth()
{
    m_PD = PDNorth;
}

void CSkyDialog::OnSkySouth()
{
    m_PD = PDSouth;
}

void CSkyDialog::OnSkyWest()
{
    m_PD = PDWest;
}

void CSkyDialog::OnSkyEast()
{
    m_PD = PDEast;
}

void CSkyDialog::showEvent(QShowEvent* event)
{
    if (!m_initialized)
    {
        OnInitDialog();
        m_initialized = true;
    }

    QDialog::showEvent(event);
}

#include <SkyDialog.moc>
