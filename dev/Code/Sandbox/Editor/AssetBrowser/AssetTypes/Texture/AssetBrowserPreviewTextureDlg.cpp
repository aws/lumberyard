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

// Description : Implementation of AssetBrowserPreviewTextureDlg.h

#include "StdAfx.h"
#include "AssetBrowser/AssetBrowserPreviewDlg.h"
#include "AssetBrowserPreviewTextureDlg.h"
#include "AssetTextureItem.h"

#include <AssetBrowser/AssetTypes/Texture/ui_AssetBrowserPreviewTextureDlg.h>
#include <QColorDialog>

const int kExpandedTexturePreviewPanelHeight = 111;
const int kCollapsedTexturePreviewPanelHeight = 25;

CAssetBrowserPreviewTextureDlg::CAssetBrowserPreviewTextureDlg(QWidget* pParent /*=NULL*/)
    : QWidget(pParent)
    , m_ui(new Ui::AssetBrowserPreviewTextureDlg)
{
    m_ui->setupUi(this);
    m_ui->m_cbMips->setVisible(false);


    m_pTexture = NULL;
    m_customBackColor = QColor(255, 0, 255);

    void (QComboBox::* currentIndexChanged)(int) = &QComboBox::currentIndexChanged;

    connect(m_ui->m_radioShowAlpha, &QPushButton::clicked, this, &CAssetBrowserPreviewTextureDlg::OnBnClickedButtonShowAlpha);
    connect(m_ui->m_radioShowRgb, &QPushButton::clicked, this, &CAssetBrowserPreviewTextureDlg::OnBnClickedButtonShowRgb);
    connect(m_ui->m_radioShowRgba, &QPushButton::clicked, this, &CAssetBrowserPreviewTextureDlg::OnBnClickedButtonShowRgba);
    connect(m_ui->m_buttonZoom1To1, &QPushButton::clicked, this, &CAssetBrowserPreviewTextureDlg::OnBnClickedButtonZoom1to1);
    connect(m_ui->m_checkSmoothTexture, &QCheckBox::clicked, this, &CAssetBrowserPreviewTextureDlg::OnBnClickedCheckSmoothTexture);
    connect(m_ui->m_cbMips, currentIndexChanged, this, &CAssetBrowserPreviewTextureDlg::OnCbnSelchangeComboTextureMipLevel);
    connect(m_ui->m_cbBackColor, currentIndexChanged, this, &CAssetBrowserPreviewTextureDlg::OnCbnSelendokComboTexturePreviewBackcolor);
    connect(m_ui->m_checkMoreInfo, &QCheckBox::clicked, this, &CAssetBrowserPreviewTextureDlg::OnBnClickedCheckMoreTextureInfo);

    OnInitDialog();
}

CAssetBrowserPreviewTextureDlg::~CAssetBrowserPreviewTextureDlg()
{
}

// CAssetBrowserPreviewTextureDlg message handlers

void CAssetBrowserPreviewTextureDlg::OnBnClickedButtonShowAlpha()
{
    m_ui->m_checkSmoothTexture->setEnabled(true);
    m_pTexture->m_previewDrawingMode = CAssetTextureItem::eAssetTextureDrawing_Alpha;
    window()->findChild<QWidget*>("m_wndAssetRender")->update();
}

void CAssetBrowserPreviewTextureDlg::OnBnClickedButtonShowRgb()
{
    m_ui->m_checkSmoothTexture->setEnabled(true);
    m_pTexture->m_previewDrawingMode = CAssetTextureItem::eAssetTextureDrawing_RGB;
    window()->findChild<QWidget*>("m_wndAssetRender")->update();
}

void CAssetBrowserPreviewTextureDlg::OnBnClickedButtonShowRgba()
{
    m_ui->m_checkSmoothTexture->setEnabled(false);
    m_pTexture->m_previewDrawingMode = CAssetTextureItem::eAssetTextureDrawing_RGBA;
    window()->findChild<QWidget*>("m_wndAssetRender")->update();
}

void CAssetBrowserPreviewTextureDlg::OnBnClickedButtonZoom1to1()
{
    const QRect rc = window()->findChild<QWidget*>("m_wndAssetRender")->rect();
    m_pTexture->PreviewCenterAndScaleToFit(rc);
    window()->findChild<QWidget*>("m_wndAssetRender")->update();
}

void CAssetBrowserPreviewTextureDlg::OnBnClickedCheckSmoothTexture()
{
    m_pTexture->m_bPreviewSmooth = m_ui->m_checkSmoothTexture->isChecked();
    window()->findChild<QWidget*>("m_wndAssetRender")->update();
}

void CAssetBrowserPreviewTextureDlg::OnCbnSelchangeComboTextureMipLevel()
{
}

void CAssetBrowserPreviewTextureDlg::Init()
{
    if (m_pTexture)
    {
        m_ui->m_cbMips->clear();

        for (size_t i = 0; i < m_pTexture->m_nMips; ++i)
        {
            m_ui->m_cbMips->addItem(tr("Mip%1").arg(i));
        }

        if (m_ui->m_checkMoreInfo->isChecked())
        {
            ComputeHistogram();
        }

        if (m_ui->m_radioShowAlpha->isChecked())
        {
            m_pTexture->m_previewDrawingMode = CAssetTextureItem::eAssetTextureDrawing_Alpha;
        }
        else if (m_ui->m_radioShowRgb->isChecked())
        {
            m_pTexture->m_previewDrawingMode = CAssetTextureItem::eAssetTextureDrawing_RGB;
        }
        else if (m_ui->m_radioShowRgba->isChecked())
        {
            m_pTexture->m_previewDrawingMode = CAssetTextureItem::eAssetTextureDrawing_RGBA;
        }
    }

    m_ui->m_checkSmoothTexture->setEnabled(true);
    m_ui->m_checkSmoothTexture->setChecked(true);
}

void CAssetBrowserPreviewTextureDlg::ComputeHistogram()
{
    CImageEx img;
    QString str = m_pTexture->GetRelativePath();

    str += m_pTexture->GetFilename();

    if (!CImageUtil::LoadImage(str, img))
    {
        return;
    }

    unsigned int* pData = img.GetData();

    if (!pData)
    {
        img.Release();
        return;
    }

    if (0 == img.GetWidth() || 0 == img.GetHeight())
    {
        img.Release();
        return;
    }

    if (pData)
    {
        m_ui->m_histogram->ComputeHistogram(img, CImageHistogram::eImageFormat_32BPP_BGRA);
    }

    img.Release();
}

void CAssetBrowserPreviewTextureDlg::OnInitDialog()
{
    m_ui->m_radioShowRgb->setChecked(true);
    Init();
    m_ui->m_histogram->setVisible(false);
}

void CAssetBrowserPreviewTextureDlg::OnCbnSelendokComboTexturePreviewBackcolor()
{
    switch (m_ui->m_cbBackColor->currentIndex())
    {
    case 0:
    {
        m_pTexture->m_previewBackColor = QColor(255, 255, 255);
        break;
    }

    case 1:
    {
        m_pTexture->m_previewBackColor = QColor(0, 0, 0);
        break;
    }

    case 2:
    {
        m_pTexture->m_previewBackColor = QColor(128, 128, 128);
        break;
    }

    case 3:
    {
        QColor color = QColorDialog::getColor(m_customBackColor);
        if (color.isValid())
        {
            m_pTexture->m_previewBackColor = m_customBackColor = color;
        }

        break;
    }
    }

    window()->findChild<QWidget*>("m_wndAssetRender")->update();
}

void CAssetBrowserPreviewTextureDlg::OnBnClickedCheckMoreTextureInfo()
{
    if (m_ui->m_checkMoreInfo->isChecked())
    {
        ComputeHistogram();
        m_ui->m_histogram->setVisible(true);
    }
    else
    {
        m_ui->m_histogram->setVisible(false);
    }

    OnBnClickedButtonZoom1to1();
}

#include <AssetBrowser/AssetTypes/Texture/AssetBrowserPreviewTextureDlg.moc>
