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
#include "Core/SmoothingGroupManager.h"
#include "DesignerSubPanel.h"
#include "Tools/BaseTool.h"

#include <QBoxLayout>
#include <QTabWidget>

namespace SubEditTool
{
    DesignerSubPanel* s_pSubBrushDesignerPanel = NULL;
    int s_nMFCWrapperPanelId = 0;
}

class DesignerSubPanelConstructor
{
public:
    static IDesignerSubPanel* Create()
    {
        if (!SubEditTool::s_pSubBrushDesignerPanel)
        {
            SubEditTool::s_pSubBrushDesignerPanel = new DesignerSubPanel;
        }
        if (!SubEditTool::s_nMFCWrapperPanelId)
        {
            std::vector<CD::SSelectedInfo> selections;
            CD::GetSelectedObjectList(selections);
            bool bExpandPanel = selections.size() > 1;

            SubEditTool::s_nMFCWrapperPanelId = GetIEditor()->AddRollUpPage(ROLLUP_OBJECTS, _T("Settings"), SubEditTool::s_pSubBrushDesignerPanel, -1, bExpandPanel);
        }

        return SubEditTool::s_pSubBrushDesignerPanel;
    }
};

void DesignerSubPanel::Done()
{
    gDesignerSettings.Save();

    if (SubEditTool::s_pSubBrushDesignerPanel)
    {
        GetIEditor()->RemoveRollUpPage(ROLLUP_OBJECTS, SubEditTool::s_nMFCWrapperPanelId);
        SubEditTool::s_nMFCWrapperPanelId = 0;
        SubEditTool::s_pSubBrushDesignerPanel = NULL;
    }
}

void DesignerSubPanel::SetDesignerTool(DesignerTool* pTool)
{
    m_pDesignerTool = pTool;
    DESIGNER_ASSERT(m_pDesignerTool && m_pDesignerTool->GetModel());
    if (m_pDesignerTool && m_pDesignerTool->GetModel())
    {
        gDesignerSettings.bDisplayBackFaces = m_pDesignerTool->GetModel()->CheckModeFlag(CD::eDesignerMode_DisplayBackFace);
    }
}

DesignerSubPanel::DesignerSubPanel(QWidget* parent)
    : QWidget(parent)
    , m_pDesignerTool(NULL)
{
    gDesignerSettings.Load();

    QTabWidget* pTabWidget = new QTabWidget;
    pTabWidget->setFixedHeight(288);
    pTabWidget->setTabPosition(QTabWidget::North);
    OrganizeSettingLayout(pTabWidget);
    OrganizeObjectFlagsLayout(pTabWidget);

    QBoxLayout* pMainLayout = new QBoxLayout(QBoxLayout::TopToBottom);
    pMainLayout->addWidget(pTabWidget, 0, Qt::AlignTop);
    setLayout(pMainLayout);

    UpdateBackFaceCheckBoxFromContext();
}

void DesignerSubPanel::OrganizeObjectFlagsLayout(QTabWidget* pTab)
{
    Serialization::SStructs ss;
    CSelectionGroup* pSelection = GetIEditor()->GetObjectManager()->GetSelection();
    int nSelectedDesignerObjCount = 0;
    for (int i = 0, iCount(pSelection->GetCount()); i < iCount; ++i)
    {
        CBaseObject* pObj = pSelection->GetObject(i);
        if (pObj->GetType() == OBJTYPE_SOLID)
        {
            ++nSelectedDesignerObjCount;
            ((DesignerObject*)pObj)->GetEngineFlags().Set();
            ss.push_back(Serialization::SStruct(((DesignerObject*)pObj)->GetEngineFlags()));
        }
    }

    if (ss.empty())
    {
        return;
    }

    QWidget* pPaper = pTab->widget(pTab->insertTab(pTab->count(), new QWidget(pTab), "Object"));
    QBoxLayout* pBoxLayout = new QBoxLayout(QBoxLayout::TopToBottom);
    pBoxLayout->setContentsMargins(0, 0, 0, 0);
    m_pObjectFlagsProperties = new QPropertyTree(pPaper);
    m_pObjectFlagsProperties->attach(ss);
    m_pObjectFlagsProperties->setPackCheckboxes(false);

    m_pObjectFlagsProperties->setCompact(true);
    pBoxLayout->addWidget(m_pObjectFlagsProperties);
    pPaper->setLayout(pBoxLayout);
    QObject::connect(m_pObjectFlagsProperties, &QPropertyTree::signalChanged, pPaper, [ = ] {UpdateEngineFlagsTab();
        });

    if (nSelectedDesignerObjCount >= 2)
    {
        pTab->setCurrentWidget(pPaper);
    }
}

void DesignerSubPanel::OrganizeSettingLayout(QTabWidget* pTab)
{
    QWidget* pPaper = pTab->widget(pTab->insertTab(pTab->count(), new QWidget(pTab), "CD"));
    QBoxLayout* pBoxLayout = new QBoxLayout(QBoxLayout::TopToBottom);
    pBoxLayout->setContentsMargins(0, 0, 0, 0);
    m_pSettingProperties = new QPropertyTree(pPaper);
    m_pSettingProperties->attach(Serialization::SStruct(gDesignerSettings));
    m_pSettingProperties->setCompact(true);
    m_pSettingProperties->setPackCheckboxes(false);
    pBoxLayout->addWidget(m_pSettingProperties);
    pPaper->setLayout(pBoxLayout);
    QObject::connect(m_pSettingProperties, &QPropertyTree::signalChanged, pPaper, [ = ] {NotifyDesignerSettingChanges(false);
        });
    QObject::connect(m_pSettingProperties, &QPropertyTree::signalContinuousChange, pPaper, [ = ] {NotifyDesignerSettingChanges(true);
        });
}

void DesignerSubPanel::UpdateSettingsTab()
{
    if (!m_pSettingProperties)
    {
        return;
    }
    m_pSettingProperties->revertNoninterrupting();
}

void DesignerSubPanel::UpdateEngineFlagsTab()
{
    CSelectionGroup* pSelection = GetIEditor()->GetObjectManager()->GetSelection();
    for (int i = 0, iCount(pSelection->GetCount()); i < iCount; ++i)
    {
        CBaseObject* pObj = pSelection->GetObject(i);
        if (pObj->GetType() == OBJTYPE_SOLID)
        {
            ((DesignerObject*)pObj)->UpdateEngineFlags();
        }
    }
}

void DesignerSubPanel::NotifyDesignerSettingChanges(bool continous)
{
    gDesignerSettings.Update(continous);
    if (m_pDesignerTool && m_pDesignerTool->GetCurrentTool())
    {
        UpdateBackFaceFlag(m_pDesignerTool->GetCurrentTool()->GetMainContext());
    }
}

void DesignerSubPanel::UpdateBackFaceCheckBoxFromContext()
{
    if (m_pDesignerTool)
    {
        UpdateBackFaceCheckBox(m_pDesignerTool->GetModel());
    }
    else
    {
        std::vector<CD::SSelectedInfo> selection;
        CD::GetSelectedObjectList(selection);
        if (selection.size() > 0)
        {
            UpdateBackFaceCheckBox(selection[0].m_pModel);
        }
        else
        {
            UpdateBackFaceCheckBox(NULL);
        }
    }
}

void DesignerSubPanel::OnEditorNotifyEvent(EEditorNotifyEvent event)
{
    switch (event)
    {
    case eNotify_OnSelectionChange:
        UpdateBackFaceCheckBoxFromContext();
        break;
    }
}

void DesignerSubPanel::UpdateBackFaceFlag(CD::SMainContext& mc)
{
    int modelFlag = mc.pModel->GetModeFlag();
    bool bDesignerDisplayBackFace = modelFlag & CD::eDesignerMode_DisplayBackFace;
    if (gDesignerSettings.bDisplayBackFaces == bDesignerDisplayBackFace)
    {
        return;
    }

    if (gDesignerSettings.bDisplayBackFaces)
    {
        mc.pModel->SetModeFlag(modelFlag | CD::eDesignerMode_DisplayBackFace);
    }
    else
    {
        mc.pModel->SetModeFlag(modelFlag & (~CD::eDesignerMode_DisplayBackFace));
    }

    mc.pModel->GetSmoothingGroupMgr()->InvalidateAll();
    CD::UpdateAll(mc, CD::eUT_SyncPrefab | CD::eUT_Mesh);
}

void DesignerSubPanel::UpdateBackFaceCheckBox(CD::Model* pModel)
{
    gDesignerSettings.bDisplayBackFaces = false;
    if (pModel && pModel->CheckModeFlag(CD::eDesignerMode_DisplayBackFace))
    {
        gDesignerSettings.bDisplayBackFaces = true;
    }
    m_pSettingProperties->revert();
}

REGISTER_GENERALPANEL(SettingPanel, DesignerSubPanel, DesignerSubPanelConstructor);