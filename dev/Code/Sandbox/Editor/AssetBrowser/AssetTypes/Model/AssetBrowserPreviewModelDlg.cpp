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

// Description : Implementation of AssetBrowserPreviewModelDlg.h

#include "StdAfx.h"
#include "AssetBrowserPreviewModelDlg.h"
#include "AssetModelItem.h"
#include "AssetBrowser/AssetBrowserDialog.h"
#include "AssetBrowserPreviewModelDlgFooter.h"

#include <AssetBrowser/AssetTypes/Character/ui_AssetBrowserPreviewCharacterDlg.h>
#include "ITimeOfDay.h"

#include <QDockWidget>

CAssetBrowserPreviewModelDlg::CAssetBrowserPreviewModelDlg(QWidget* pParent /*=NULL*/)
    : QWidget(pParent)
    , m_ui(new Ui::AssetBrowserPreviewCharacterDlg)
{
    m_ui->setupUi(this);
    m_ui->m_labelLighting->setVisible(false);
    m_ui->m_lightingCB->setVisible(false);

    m_pModel = NULL;
    m_pFooter = NULL;

    void (QComboBox::* currentIndexChanged)(int) = &QComboBox::currentIndexChanged;

    connect(m_ui->m_checkWireframe, &QCheckBox::clicked, this, &CAssetBrowserPreviewModelDlg::OnBnClickedButtonWireframe);
    connect(m_ui->m_checkPhysics, &QCheckBox::clicked, this, &CAssetBrowserPreviewModelDlg::OnBnClickedButtonPhysics);
    connect(m_ui->m_checkNormals, &QCheckBox::clicked, this, &CAssetBrowserPreviewModelDlg::OnBnClickedButtonNormals);
    connect(m_ui->m_buttonResetView, &QPushButton::clicked, this, &CAssetBrowserPreviewModelDlg::OnBnClickedButtonResetView);
    connect(m_ui->m_fullscreenBtn, &QPushButton::clicked, this, &CAssetBrowserPreviewModelDlg::OnBnClickedButtonFullscreen);
    connect(m_ui->m_lightingCB, currentIndexChanged, this, &CAssetBrowserPreviewModelDlg::OnLightingChanged);
    connect(m_ui->m_buttonSaveThumbAngle, &QPushButton::clicked, this, &CAssetBrowserPreviewModelDlg::OnBnClickedButtonSaveThumbAngle);

    OnInitDialog();
}

CAssetBrowserPreviewModelDlg::~CAssetBrowserPreviewModelDlg()
{
    m_pModel = NULL;
    m_pFooter = NULL;
}

void CAssetBrowserPreviewModelDlg::OnInitDialog()
{
    XmlNodeRef lightingNodeDay = GetISystem()->LoadXmlFromFile("Editor/asset_model_preview_day.tod");
    XmlNodeRef lightingNodeEvening = GetISystem()->LoadXmlFromFile("Editor/asset_model_preview_evening.tod");
    XmlNodeRef lightingNodeNight = GetISystem()->LoadXmlFromFile("Editor/asset_model_preview_night.tod");

    if (lightingNodeDay)
    {
        m_lightingMap.insert(std::pair<QString, XmlNodeRef>("Day", lightingNodeDay));
    }

    if (lightingNodeEvening)
    {
        m_lightingMap.insert(std::pair<QString, XmlNodeRef>("Evening", lightingNodeEvening));
    }

    if (lightingNodeNight)
    {
        m_lightingMap.insert(std::pair<QString, XmlNodeRef>("Night", lightingNodeNight));
    }

    m_oldTod = GetISystem()->CreateXmlNode();
    ITimeOfDay* pTimeOfDay = gEnv->p3DEngine->GetTimeOfDay();

    if (pTimeOfDay)
    {
        pTimeOfDay->Serialize(m_oldTod, false);
    }

    for (auto item = m_lightingMap.begin(), end = m_lightingMap.end(); item != end; ++item)
    {
        m_ui->m_lightingCB->addItem(item->first);
    }

    m_ui->m_lightingCB->setCurrentIndex(0);
    OnLightingChanged();
}

// CAssetBrowserPreviewModelDlg message handlers
void CAssetBrowserPreviewModelDlg::Init()
{
    m_ui->m_checkWireframe->setChecked(CAssetModelItem::s_bWireframe);
    m_ui->m_checkPhysics->setChecked(CAssetModelItem::s_bPhysics);
    m_ui->m_checkNormals->setChecked(CAssetModelItem::s_bNormals);
}

void CAssetBrowserPreviewModelDlg::OnBnClickedButtonWireframe()
{
    CAssetModelItem::s_bWireframe = m_ui->m_checkWireframe->isChecked();

    window()->findChild<QWidget*>("m_wndAssetRender")->update();
}

void CAssetBrowserPreviewModelDlg::OnBnClickedButtonPhysics()
{
    CAssetModelItem::s_bPhysics = m_ui->m_checkPhysics->isChecked();

    window()->findChild<QWidget*>("m_wndAssetRender")->update();
}

void CAssetBrowserPreviewModelDlg::OnBnClickedButtonNormals()
{
    CAssetModelItem::s_bNormals = m_ui->m_checkNormals->isChecked();

    window()->findChild<QWidget*>("m_wndAssetRender")->update();
}

void CAssetBrowserPreviewModelDlg::OnBnClickedButtonResetView()
{
    if (m_pModel)
    {
        m_ui->m_checkWireframe->setChecked((CAssetModelItem::s_bWireframe = false));
        m_ui->m_checkPhysics->setChecked((CAssetModelItem::s_bPhysics = false));
        m_ui->m_checkNormals->setChecked((CAssetModelItem::s_bNormals = false));
        DockViewPane();
        m_pModel->ResetView();

        if (m_pFooter)
        {
            m_pFooter->Reset();
        }

        m_ui->m_lightingCB->setCurrentIndex(0);
        OnLightingChanged();
    }

    window()->findChild<QWidget*>("m_wndAssetRender")->update();
}

void CAssetBrowserPreviewModelDlg::OnLightingChanged()
{
    const QString combotext = m_ui->m_lightingCB->currentText();

    auto item = m_lightingMap.find(combotext);

    if (item != m_lightingMap.end())
    {
        ITimeOfDay* pTimeOfDay = gEnv->p3DEngine->GetTimeOfDay();

        if (pTimeOfDay && item->second)
        {
            pTimeOfDay->Serialize(item->second, true);
        }
    }
}

void CAssetBrowserPreviewModelDlg::OnBnClickedButtonFullscreen()
{
    QWidget* parent = parentWidget();
    while (parent != nullptr)
    {
        if (QDockWidget* dw = qobject_cast<QDockWidget*>(parent))
        {
            dw->setFloating(!dw->isFloating());
        }
        parent = parent->parentWidget();
    }
}

void CAssetBrowserPreviewModelDlg::DockViewPane()
{
    QWidget* parent = parentWidget();
    while (parent != nullptr)
    {
        if (QDockWidget* dw = qobject_cast<QDockWidget*>(parent))
        {
            dw->setFloating(false);
        }
        parent = parent->parentWidget();
    }
}

void CAssetBrowserPreviewModelDlg::OnBnClickedButtonSaveThumbAngle()
{
    m_pModel->CacheCurrentThumbAngle();
    m_pModel->UnloadThumbnail();
    m_pModel->LoadThumbnail();
    CAssetBrowserDialog::Instance()->GetAssetViewer().SelectAsset(m_pModel);
}

#include <AssetBrowser/AssetTypes/Model/AssetBrowserPreviewModelDlg.moc>