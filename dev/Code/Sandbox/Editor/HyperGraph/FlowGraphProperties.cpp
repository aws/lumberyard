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
#include "FlowGraphProperties.h"
#include "HyperGraphDialog.h"
#include "FlowGraph.h"
#include <Controls/ReflectedPropertyControl/ReflectedPropertyCtrl.h>
#include <HyperGraph/ui_FlowGraphProperties.h>
#include <QtUtil.h>
#include <QVBoxLayout>


CFlowGraphProperties::CFlowGraphProperties(QWidget* parentWidget)
    :   QWidget(parentWidget)
    , m_pParent(0)
    , m_pGraph(0)
    , m_bUpdate(false)
    , ui(new Ui::FlowGraphProperties)
{
    ui->setupUi(this);

    QVBoxLayout* layout = new QVBoxLayout();
    layout->setMargin(0);
    m_graphProps = new ReflectedPropertyControl(this);
    m_graphProps->Setup(false, 140);
    layout->addWidget(m_graphProps);
    ui->propertyTree->setLayout(layout);

    m_varEnabled->SetDescription(tr("Enable/Disable the FlowGraph"));

    m_varMultiPlayer->SetDescription(tr("MultiPlayer Option of the FlowGraph (Default: ServerOnly)"));
    m_varMultiPlayer->AddEnumItem("ServerOnly", (int) CFlowGraph::eMPT_ServerOnly);
    m_varMultiPlayer->AddEnumItem("ClientOnly", (int) CFlowGraph::eMPT_ClientOnly);
    m_varMultiPlayer->AddEnumItem("ClientServer", (int) CFlowGraph::eMPT_ClientServer);

    hide();
}

CFlowGraphProperties::~CFlowGraphProperties()
{
}

void CFlowGraphProperties::SetGraph(CHyperGraph* pGraph)
{
    if (pGraph && pGraph->IsFlowGraph())
    {
        m_pGraph = static_cast<CFlowGraph*> (pGraph);
    }
    else
    {
        m_pGraph = 0;
    }

    if (m_pGraph)
    {
        FillProps();
        QString text = m_pGraph->GetDescription();
        text.replace("\\n", "\r\n");
        ui->graphDescription->setText(text);
    }
    else
    {
        m_graphProps->RemoveAllItems();
        ui->graphDescription->setText("");
        ResizeProps(true);
    }
    setVisible(m_pGraph);
}

void CFlowGraphProperties::FillProps()
{
    if (m_pGraph == 0)
    {
        return;
    }

    m_graphProps->EnableUpdateCallback(false);
    m_varEnabled = m_pGraph->IsEnabled();
    m_varMultiPlayer = m_pGraph->GetMPType();
    m_graphProps->EnableUpdateCallback(true);

    CVarBlockPtr pVB = new CVarBlock;
    if (m_pGraph->GetAIAction() == 0)
    {
        pVB->AddVariable(m_varEnabled, "Enabled");
        pVB->AddVariable(m_varMultiPlayer, "MultiPlayer Options");
        int flags = m_varMultiPlayer->GetFlags();
        //      if (m_bShowMP)
        flags &= ~IVariable::UI_DISABLED;
        //      else
        //          flags |=  IVariable::UI_DISABLED;
        m_varMultiPlayer->SetFlags(flags);
    }
    else
    {
        ;
    }

    m_graphProps->RemoveAllItems();
    m_graphProps->AddVarBlock(pVB);
    m_graphProps->SetUpdateCallback(functor(*this, &CFlowGraphProperties::OnVarChange));
    m_graphProps->ExpandAll();

    ResizeProps(pVB->IsEmpty());
}

void CFlowGraphProperties::ResizeProps(bool bHide)
{
    // Resize to fit properties.
    int h = m_graphProps->GetVisibleHeight() + 5;
    ui->propertyTree->setFixedHeight(h);
    ui->propertyTree->setVisible(!bHide);
}

void CFlowGraphProperties::OnVarChange(IVariable* pVar)
{
    assert (m_pGraph != 0);
#if 0
    CString val;
    pVar->Get(val);
    CryLogAlways("Var %s changed to %s", pVar->GetName(), val.GetString());
#endif
    m_pGraph->SetEnabled(m_varEnabled);
    m_pGraph->SetMPType((CFlowGraph::EMultiPlayerType) (int) m_varMultiPlayer);

    m_bUpdate = true;
    m_pParent->UpdateGraphProperties(m_pGraph);
    m_bUpdate = false;
}

void CFlowGraphProperties::UpdateGraphProperties(CHyperGraph* pGraph)
{
    if (m_bUpdate)
    {
        return;
    }

    if (pGraph && pGraph->IsFlowGraph())
    {
        CFlowGraph* pFG = static_cast<CFlowGraph*> (pGraph);
        if (pFG == m_pGraph)
        {
            FillProps();
        }
    }
}

void CFlowGraphProperties::ShowMultiPlayer(bool bShow)
{
    if (m_bShowMP != bShow)
    {
        m_bShowMP = bShow;
        m_bUpdate = true;
        FillProps();
        m_bUpdate = false;
    }
}

void CFlowGraphProperties::SetDescription(QString new_desc)
{
    ui->graphDescription->setText(new_desc);
}
