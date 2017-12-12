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

#include "StdAfx.h"
#include "Tools/DesignerTool.h"
#include "Tools/BaseTool.h"
#include "DesignerPanel.h"
#include "Material/MaterialManager.h"
#include "CubeEditorPanel.h"

#include <QLabel>
#include <QComboBox>
#include <QCheckBox>
#include <QGridLayout>
#include <QPushButton>

using namespace CD;

void CubeEditorPanel::Done()
{
    QSettings settings;
    for (auto g : QString(CD::sessionName).split('\\'))
        settings.beginGroup(g);
    settings.setValue("CubeEditor_CubeSize_Index", m_pBrushSizeComboBox->currentIndex());
    settings.setValue("CubeEditor_MergeSides", IsSidesMerged() ? 1 : 0);
    settings.setValue("CubeEditor_MaterialID", GetSubMatID() + 1);
    settings.setValue("CubeEditor_EditMode", (int)m_pCubeEditor->GetEditMode());
    CD::RemoveAttributeWidget(m_pCubeEditor);
}

CubeEditorPanel::CubeEditorPanel(CubeEditor* pCubeEditor)
    : m_pCubeEditor(pCubeEditor)
{
    QGridLayout* pGridLayout = new QGridLayout;

    m_pAddButton = new QPushButton("Add");
    m_pRemoveButton = new QPushButton("Remove");
    m_pPaintButton = new QPushButton("Paint");
    m_pBrushSizeComboBox = new QComboBox;
    m_pSubMatIdComboBox = new QComboBox;
    m_pMergeSideCheckBox = new QCheckBox("Merge Sides");

    QObject::connect(m_pAddButton, &QPushButton::clicked, this, [ = ] {SelectEditMode(eCubeEditorMode_Add);
        });
    QObject::connect(m_pRemoveButton, &QPushButton::clicked, this, [ = ] {SelectEditMode(eCubeEditorMode_Remove);
        });
    QObject::connect(m_pPaintButton, &QPushButton::clicked, this, [ = ] {SelectEditMode(eCubeEditorMode_Paint);
        });

    pGridLayout->addWidget(m_pAddButton, 0, 0, Qt::AlignTop);
    pGridLayout->addWidget(m_pRemoveButton, 0, 1, Qt::AlignTop);
    pGridLayout->addWidget(m_pPaintButton, 0, 2, Qt::AlignTop);
    pGridLayout->addWidget(new QLabel("Brush Size"), 1, 0, Qt::AlignTop);
    pGridLayout->addWidget(m_pBrushSizeComboBox, 1, 1, 1, 2, Qt::AlignTop);
    pGridLayout->addWidget(new QLabel("Sub Mat ID"), 2, 0, Qt::AlignTop);
    pGridLayout->addWidget(m_pSubMatIdComboBox, 2, 1, 1, 2, Qt::AlignTop);
    pGridLayout->addWidget(m_pMergeSideCheckBox, 3, 0, 1, 3, Qt::AlignTop);

    setLayout(pGridLayout);

    QSettings settings;
    for (auto g : QString(CD::sessionName).split('\\'))
        settings.beginGroup(g);

    int nCubeSizeIndex = settings.value("CubeEditor_CubeSize_Index", 3).toInt();
    m_pBrushSizeComboBox->addItem("0.125", QVariant(0.125));
    m_pBrushSizeComboBox->addItem("0.25", QVariant(0.25));
    m_pBrushSizeComboBox->addItem("0.5", QVariant(0.5));
    m_pBrushSizeComboBox->addItem("1.0", QVariant(1.0));
    m_pBrushSizeComboBox->addItem("2.0", QVariant(2.0));
    m_pBrushSizeComboBox->addItem("4.0", QVariant(4.0));
    m_pBrushSizeComboBox->addItem("8.0", QVariant(8.0));
    m_pBrushSizeComboBox->addItem("16.0", QVariant(16.0));
    m_pBrushSizeComboBox->addItem("32.0", QVariant(32.0));
    m_pBrushSizeComboBox->addItem("64.0", QVariant(64.0));
    m_pBrushSizeComboBox->setCurrentIndex(nCubeSizeIndex);

    int nMergeSides = settings.value("CubeEditor_MergeSides", 0).toInt();
    m_pMergeSideCheckBox->setCheckState(nMergeSides == 1 ? Qt::Checked : Qt::Unchecked);

    int nEditMode = settings.value("CubeEditor_EditMode", 0).toInt();
    SelectEditMode((ECubeEditorMode)nEditMode);

    UpdateSubMaterialComboBox();
    if (!SetMaterial(GetIEditor()->GetMaterialManager()->GetCurrentMaterial()))
    {
        int nMaterialID = settings.value("CubeEditor_MaterialID", 0).toInt();
        SetSubMatID(nMaterialID);
    }

    CD::SetAttributeWidget(m_pCubeEditor, this);
}

BrushFloat CubeEditorPanel::GetCubeSize() const
{
    int nCurSel = m_pBrushSizeComboBox->currentIndex();
    if (nCurSel != -1)
    {
        return (BrushFloat)(m_pBrushSizeComboBox->itemData(nCurSel).toDouble());
    }
    return (BrushFloat)1.0;
}

bool CubeEditorPanel::IsSidesMerged() const
{
    return m_pMergeSideCheckBox->isChecked();
}

bool CubeEditorPanel::IsAddButtonChecked() const
{
    return m_CheckedButton == eCubeEditorMode_Add;
}

bool CubeEditorPanel::IsRemoveButtonChecked() const
{
    return m_CheckedButton == eCubeEditorMode_Remove;
}

bool CubeEditorPanel::IsPaintButtonChecked() const
{
    return m_CheckedButton == eCubeEditorMode_Paint;
}

int CubeEditorPanel::GetSubMatID() const
{
    return CD::GetSubMatID(m_pSubMatIdComboBox);
}

void CubeEditorPanel::SetSubMatID(int nID) const
{
    CD::SetSubMatID(m_pCubeEditor->GetBaseObject(), m_pSubMatIdComboBox, nID);
}

bool CubeEditorPanel::SetMaterial(CMaterial* pMaterial)
{
    return CD::SelectMaterial(m_pCubeEditor->GetBaseObject(), m_pSubMatIdComboBox, pMaterial);
}

void CubeEditorPanel::SelectPrevBrush()
{
    int nCurSel = m_pBrushSizeComboBox->currentIndex();
    if (nCurSel < m_pBrushSizeComboBox->count() - 1)
    {
        m_pBrushSizeComboBox->setCurrentIndex(nCurSel + 1);
    }
}

void CubeEditorPanel::SelectNextBrush()
{
    int nCurSel = m_pBrushSizeComboBox->currentIndex();
    if (nCurSel > 0)
    {
        m_pBrushSizeComboBox->setCurrentIndex(nCurSel - 1);
    }
}

void CubeEditorPanel::UpdateSubMaterialComboBox()
{
    int nCurSel = m_pSubMatIdComboBox->currentIndex();
    m_pSubMatIdComboBox->clear();
    if (m_pCubeEditor->GetBaseObject() == NULL)
    {
        return;
    }
    FillComboBoxWithSubMaterial(m_pSubMatIdComboBox, m_pCubeEditor->GetBaseObject());
    if (nCurSel != -1)
    {
        SetSubMatID(nCurSel + 1);
    }
    else
    {
        SetSubMatID(1);
    }
}

void CubeEditorPanel::SelectEditMode(ECubeEditorMode editMode)
{
    m_pAddButton->setStyleSheet(GetDefaultButtonStyle());
    m_pRemoveButton->setStyleSheet(GetDefaultButtonStyle());
    m_pPaintButton->setStyleSheet(GetDefaultButtonStyle());

    if (editMode == eCubeEditorMode_Remove)
    {
        m_pRemoveButton->setStyleSheet(GetSelectButtonStyle(this));
        m_CheckedButton = eCubeEditorMode_Remove;
    }
    else if (editMode == eCubeEditorMode_Paint)
    {
        m_pPaintButton->setStyleSheet(GetSelectButtonStyle(this));
        m_CheckedButton = eCubeEditorMode_Paint;
    }
    else
    {
        m_pAddButton->setStyleSheet(GetSelectButtonStyle(this));
        m_CheckedButton = eCubeEditorMode_Add;
    }
}