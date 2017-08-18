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
#include "DesignerPanel.h"
#include "Core/SmoothingGroupManager.h"
#include "Tools/BaseTool.h"
#include "../EditorCommon/QtViewPane.h"
#include "Material/MaterialManager.h"

#include <QTabWidget>
#include <QPushButton>
#include <QBoxLayout>
#include <QComboBox>
#include <QLabel>
#include <QFrame>

using namespace CD;

#define CREATE_BUTTON(tool, row, column)  QObject::connect(AddButton(tool, wc, row, column), &QPushButton::clicked, this, [ = ] {OnClickedButton(tool); })
#define CREATE_SPANNED_BUTTON(tool, row) QObject::connect(AddButton(tool, wc, row, 0, true), &QPushButton::clicked, this, [ = ] {OnClickedButton(tool); })

namespace
{
    DesignerPanel* s_pBrushDesignerPanel = NULL;
    int s_pBrushDesignerPanelId = 0;
}

namespace CD
{
    QWidget* GetMainPanelPtr()
    {
        return s_pBrushDesignerPanel;
    }
}

class DesignerPanelConstructor
{
public:
    static IDesignerPanel* Create()
    {
        if (!s_pBrushDesignerPanel)
        {
            s_pBrushDesignerPanel = new DesignerPanel;
        }
        if (!s_pBrushDesignerPanelId)
        {
            s_pBrushDesignerPanelId = GetIEditor()->AddRollUpPage(ROLLUP_OBJECTS, _T("Designer Menu"), s_pBrushDesignerPanel);
        }

        return s_pBrushDesignerPanel;
    }
};

DesignerPanel::DesignerPanel(QWidget* parent)
    : QWidget(parent)
{
    QTabWidget* pSelectionTabWidget = new QTabWidget;
    pSelectionTabWidget->setFixedHeight(160);
    pSelectionTabWidget->setTabPosition(QTabWidget::North);
    OrganizeSelectionLayout(pSelectionTabWidget);
    QTabWidget* pAdvancedTabWidget = new QTabWidget;
    pAdvancedTabWidget->setTabPosition(QTabWidget::North);
    OrganizeShapeLayout(pAdvancedTabWidget);
    OrganizeEditLayout(pAdvancedTabWidget);
    OrganizeModifyLayout(pAdvancedTabWidget);
    OrganizeSurfaceLayout(pAdvancedTabWidget);
    OrganizeMiscLayout(pAdvancedTabWidget);

    QBoxLayout* pMainLayout = new QBoxLayout(QBoxLayout::TopToBottom);
    pMainLayout->addWidget(pSelectionTabWidget, 0, Qt::AlignTop);
    pMainLayout->addWidget(pAdvancedTabWidget, 1, Qt::AlignTop);
    setLayout(pMainLayout);

    m_pAttributeTab = NULL;
}

void DesignerPanel::Done()
{
    if (s_pBrushDesignerPanel)
    {
        GetIEditor()->RemoveRollUpPage(ROLLUP_OBJECTS, s_pBrushDesignerPanelId);
        s_pBrushDesignerPanel = 0;
        s_pBrushDesignerPanelId = 0;
    }
}

QPushButton* DesignerPanel::AddButton(CD::EDesignerTool tool, const SWidgetContext& wc, int row, int column, bool bColumnSpan)
{
    QPushButton* pButton = new QPushButton(Serialization::getEnumDescription<CD::EDesignerTool>().name(tool));
    if (bColumnSpan)
    {
        wc.pGridLayout->addWidget(pButton, row, column, 1, 2, Qt::AlignTop);
    }
    else
    {
        wc.pGridLayout->addWidget(pButton, row, column, Qt::AlignTop);
    }
    m_Buttons[tool] = SMenuButton(wc.pTab, wc.pPaper, pButton);
    pButton->setAutoFillBackground(true);
    return pButton;
}

void DesignerPanel::SetAttributeWidget(const char* name, QWidget* pAttributeWidget)
{
    QBoxLayout* pLayout = (QBoxLayout*)layout();
    RemoveAttributeWidget();
    m_pAttributeTab = new QTabWidget;

    if (pAttributeWidget->inherits("CD::QPropertyTreeWidget"))
    {
        QSize size = ((QPropertyTreeWidget*)pAttributeWidget)->contentSize();
        const int visibleHeight = size.height() + 25;
        if (visibleHeight < CD::MaximumAttributePanelHeight)
        {
            m_pAttributeTab->setFixedHeight(visibleHeight);
        }
        else
        {
            m_pAttributeTab->setFixedHeight(CD::MaximumAttributePanelHeight);
        }
    }

    m_pAttributeTab->insertTab(m_pAttributeTab->count(), pAttributeWidget, name);
    pLayout->addWidget(m_pAttributeTab, 2, Qt::AlignTop);
}

void DesignerPanel::RemoveAttributeWidget()
{
    if (!m_pAttributeTab)
    {
        return;
    }
    QBoxLayout* pLayout = (QBoxLayout*)layout();
    m_pAttributeTab->setVisible(false);
    pLayout->removeWidget(m_pAttributeTab);
    m_pAttributeTab = NULL;
}

int DesignerPanel::ArrangeButtons(CD::SWidgetContext& wc, CD::EToolGroup toolGroup, int stride, int offset)
{
    std::vector<CD::EDesignerTool> tools = CD::ToolGroupMapper::the().GetToolList(toolGroup);
    int iEnd(tools.size() + offset);
    wc.pGridLayout->setRowStretch((iEnd - 1) / stride, 1);
    for (int i = offset; i < iEnd; ++i)
    {
        CREATE_BUTTON(tools[i - offset], i / stride, i % stride);
    }

    wc.pPaper->setLayout(wc.pGridLayout);

    return iEnd;
}

void DesignerPanel::OrganizeSelectionLayout(QTabWidget* pTab)
{
    QWidget* pPaper = pTab->widget(pTab->insertTab(pTab->count(), new QWidget(pTab), "Selection"));
    QGridLayout* pSelectionLayout = new QGridLayout;
    SWidgetContext wc(pTab, pPaper, pSelectionLayout);

    const int stride = 3;
    int offset = ArrangeButtons(wc, CD::eToolGroup_BasicSelection, stride, 0);
    pSelectionLayout->addWidget(CD::CreateHorizontalLine(), offset / stride + 1, 0, 1, stride, Qt::AlignTop);
    offset = ArrangeButtons(wc, CD::eToolGroup_Selection, stride, ((offset + stride * 2) / stride) * stride);
}

void DesignerPanel::OnClickedButton(EDesignerTool tool)
{
    if (m_pDesignerTool)
    {
        m_pDesignerTool->SwitchTool(tool);
    }
}

void DesignerPanel::OrganizeShapeLayout(QTabWidget* pTab)
{
    QWidget* pPaper = pTab->widget(pTab->insertTab(pTab->count(), new QWidget(pTab), "SH"));
    QGridLayout* pCreateGridLayout = new QGridLayout;
    ArrangeButtons(SWidgetContext(pTab, pPaper, pCreateGridLayout), CD::eToolGroup_Shape, 2, 0);
}

void DesignerPanel::OrganizeEditLayout(QTabWidget* pTab)
{
    QWidget* pPaper = pTab->widget(pTab->insertTab(pTab->count(), new QWidget(pTab), "ED"));
    QGridLayout* pEditGridLayout = new QGridLayout;
    ArrangeButtons(SWidgetContext(pTab, pPaper, pEditGridLayout), CD::eToolGroup_Edit, 2, 0);
}

void DesignerPanel::OrganizeModifyLayout(QTabWidget* pTab)
{
    QWidget* pPaper = pTab->widget(pTab->insertTab(pTab->count(), new QWidget(pTab), "MO"));
    QGridLayout* pModifyGridLayout = new QGridLayout;
    ArrangeButtons(SWidgetContext(pTab, pPaper, pModifyGridLayout), CD::eToolGroup_Modify, 2, 0);
}

void DesignerPanel::OrganizeSurfaceLayout(QTabWidget* pTab)
{
    QWidget* pPaper = pTab->widget(pTab->insertTab(pTab->count(), new QWidget(pTab), "SU"));
    QGridLayout* pSurfaceGridLayout = new QGridLayout;
    int offset = ArrangeButtons(SWidgetContext(pTab, pPaper, pSurfaceGridLayout), CD::eToolGroup_Surface, 2, 0);

    int row = (offset - 1) / 2;
    pSurfaceGridLayout->setRowStretch(row, 0);
    pSurfaceGridLayout->addWidget(CD::CreateHorizontalLine(), ++row, 0, 1, 2, Qt::AlignTop);

    m_pSubMatIDComboBox = new QComboBox;
    QLabel* pSubMatIDLabel = new QLabel("Sub Mat ID");
    pSubMatIDLabel->setAlignment(Qt::AlignHCenter);
    ++row;
    pSurfaceGridLayout->addWidget(pSubMatIDLabel, row, 0, Qt::AlignTop);
    pSurfaceGridLayout->addWidget(m_pSubMatIDComboBox, row, 1, Qt::AlignTop);
    pSurfaceGridLayout->setRowStretch(row, 1);
}

void DesignerPanel::OrganizeMiscLayout(QTabWidget* pTab)
{
    QWidget* pPaper = pTab->widget(pTab->insertTab(pTab->count(), new QWidget(pTab), "MI"));
    QGridLayout* pMiscGridLayout = new QGridLayout;
    ArrangeButtons(SWidgetContext(pTab, pPaper, pMiscGridLayout), CD::eToolGroup_Misc, 2, 0);
}

void DesignerPanel::SetDesignerTool(DesignerTool* pTool)
{
    m_pDesignerTool = pTool;
    UpdateCloneArrayButtons();
    MaterialChanged();
}

void DesignerPanel::UpdateCloneArrayButtons()
{
    if (!m_pDesignerTool)
    {
        return;
    }

    CBaseObject* pObj = m_pDesignerTool->GetBaseObject();
    if (!pObj)
    {
        return;
    }

    if (pObj->GetParent() && pObj->GetParent())
    {
        GetButton(eDesigner_CircleClone)->setEnabled(false);
        GetButton(eDesigner_ArrayClone)->setEnabled(false);
    }
    else if (qobject_cast<DesignerObject*>(pObj))
    {
        GetButton(eDesigner_CircleClone)->setEnabled(true);
        GetButton(eDesigner_ArrayClone)->setEnabled(true);
    }
}

QPushButton* DesignerPanel::GetButton(EDesignerTool tool)
{
    if (m_Buttons.find(tool) == m_Buttons.end())
    {
        return NULL;
    }
    return (QPushButton*)(m_Buttons[(int)tool].pButton);
}

void DesignerPanel::DisableButton(EDesignerTool tool)
{
    QPushButton* pButton = GetButton(tool);
    if (pButton)
    {
        pButton->setEnabled(false);
    }
}

void DesignerPanel::SetButtonCheck(EDesignerTool tool, int nCheckID)
{
    QPushButton* pButton = GetButton(tool);
    if (!pButton)
    {
        return;
    }

    if (nCheckID == CD::eButton_Pressed)
    {
        ShowTab(tool);
        pButton->setStyleSheet(GetSelectButtonStyle(this));
    }
    else if (nCheckID == CD::eButton_Unpressed)
    {
        pButton->setStyleSheet(GetDefaultButtonStyle());
    }
}

void DesignerPanel::ShowTab(EDesignerTool tool)
{
    if (m_Buttons.find(tool) == m_Buttons.end())
    {
        return;
    }
    int nTabIndex = m_Buttons[tool].pTab->indexOf(m_Buttons[tool].pPaper);
    m_Buttons[tool].pTab->setCurrentIndex(nTabIndex);
}

void DesignerPanel::MaterialChanged()
{
    m_pSubMatIDComboBox->clear();
    if (m_pDesignerTool->GetBaseObject() == NULL)
    {
        return;
    }
    FillComboBoxWithSubMaterial(m_pSubMatIDComboBox, m_pDesignerTool->GetBaseObject());
    m_pSubMatIDComboBox->setCurrentIndex(0);
}

void DesignerPanel::OnDataBaseItemEvent(IDataBaseItem* pItem, EDataBaseItemEvent event)
{
    if (pItem == NULL)
    {
        return;
    }

    if (event == EDB_ITEM_EVENT_SELECTED)
    {
        CD::SelectMaterial(m_pDesignerTool->GetBaseObject(), m_pSubMatIDComboBox, (CMaterial*)pItem);
    }
}

void DesignerPanel::SetAttributeTabHeight(int nHeight)
{
    if (m_pAttributeTab)
    {
        m_pAttributeTab->setFixedHeight(nHeight);
    }
}

int DesignerPanel::GetSubMatID() const
{
    return CD::GetSubMatID(m_pSubMatIDComboBox);
}

void DesignerPanel::SetSubMatID(int nID)
{
    CD::SetSubMatID(m_pDesignerTool->GetBaseObject(), m_pSubMatIDComboBox, nID);
}

REGISTER_GENERALPANEL(MainPanel, DesignerPanel, DesignerPanelConstructor);