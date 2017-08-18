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
#include "FlowGraphTokens.h"
#include "HyperGraphDialog.h"
#include "FlowGraph.h"
#include <HyperGraph/ui_FlowGraphNewTokenDlg.h>
#include <HyperGraph/ui_FlowGraphTokensDlg.h>
#include <Controls/ReflectedPropertyControl/ReflectedPropertyCtrl.h>
#include "QtUtilWin.h"
#include <QMessageBox>
#include <QVBoxLayout>


#define IDC_GRAPH_PROPERTIES 6

namespace
{
    QString typeNames[] =
    {
        "Void",         //  eFDT_Void
        "Int",          //  eFDT_Int
        "Float",        //  eFDT_Float
        "Double",       //  eFDT_Double
        "EntityId",     //  eFDT_EntityId
        "Vec3",         //  eFDT_Vec3
        "String",       //  eFDT_String
        "Bool",         //  eFDT_Bool
    };
};

//////////////////////////////////////////////////////////////////////////
// CFlowGraphTokensCtrl
//////////////////////////////////////////////////////////////////////////

CFlowGraphTokensCtrl::CFlowGraphTokensCtrl(QWidget* pParent)
    :   QWidget(pParent)
    , m_pGraph(0)
    , m_bUpdate(false)
{
    m_graphProps = new ReflectedPropertyControl(this);
    m_graphProps->Setup(false, 120);
    m_graphProps->SetReadOnly(true);

    QVBoxLayout* l = new QVBoxLayout(this);
    l->setMargin(0);
    l->addWidget(m_graphProps);

    FillProps();
    ResizeProps(true);
}

CFlowGraphTokensCtrl::~CFlowGraphTokensCtrl()
{
}

void CFlowGraphTokensCtrl::SetGraph(CHyperGraph* pGraph)
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
    }
    else
    {
        m_graphProps->RemoveAllItems();
        ResizeProps(true);
    }
}

void CFlowGraphTokensCtrl::FillProps()
{
    if (m_pGraph == 0)
    {
        return;
    }

    CVarBlockPtr pVB = new CVarBlock;

    if (m_pGraph)
    {
        IFlowGraph* pIFlowGraph = m_pGraph->GetIFlowGraph();
        if (pIFlowGraph)
        {
            size_t gtCount = pIFlowGraph->GetGraphTokenCount();
            for (size_t i = 0; i < gtCount; ++i)
            {
                const IFlowGraph::SGraphToken* pToken = pIFlowGraph->GetGraphToken(i);
                if (pToken)
                {
                    pVB->AddVariable(CreateVariable(pToken->name.c_str(), pToken->type), pToken->name.c_str());
                }
            }
        }
    }

    m_graphProps->RemoveAllItems();
    m_graphProps->AddVarBlock(pVB, "Tokens");

    ResizeProps(pVB->IsEmpty());
}

void CFlowGraphTokensCtrl::ResizeProps(bool bHide)
{
    // Resize to fit properties.
    int h = m_graphProps->GetVisibleHeight();
    m_graphProps->setFixedHeight(h + 1 + 4);
    m_graphProps->setVisible(!bHide);
}

void CFlowGraphTokensCtrl::UpdateGraphProperties(CHyperGraph* pGraph)
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

CSmartVariableEnum<QString> CFlowGraphTokensCtrl::CreateVariable(const char* name, EFlowDataTypes type)
{
    CSmartVariableEnum<QString> variable;
    variable->SetDescription(name);
    variable->Set(typeNames[type]);

    return variable;
}

//////////////////////////////////////////////////////////////////////////
// CFlowGraphEditTokensDlg
//////////////////////////////////////////////////////////////////////////

CFlowGraphTokensDlg::CFlowGraphTokensDlg(QWidget* pParent /*= NULL*/)
    : QDialog(pParent)
    , ui(new Ui::CFlowGraphTokensDlg)
{
    ui->setupUi(this);
    ui->GRAPH_TOKEN_LIST->setSelectionMode(QAbstractItemView::MultiSelection);

    connect(ui->ADD_GRAPHTOKEN, &QPushButton::clicked, this, &CFlowGraphTokensDlg::OnAddNewToken);
    connect(ui->EDIT_GRAPHTOKEN, &QPushButton::clicked, this, &CFlowGraphTokensDlg::OnEditToken);
    connect(ui->DELETE_GRAPHTOKEN, &QPushButton::clicked, this, &CFlowGraphTokensDlg::OnDeleteToken);
}

CFlowGraphTokensDlg::~CFlowGraphTokensDlg()
{
}

void CFlowGraphTokensDlg::OnAddNewToken()
{
    STokenData temp;
    temp.type = eFDT_Bool;
    CFlowGraphNewTokenDlg dlg(&temp, m_tokens);
    if (dlg.exec() == QDialog::Accepted)
    {
        m_tokens.push_back(temp);
    }

    RefreshControl();
}

void CFlowGraphTokensDlg::OnDeleteToken()
{
    QList<QListWidgetItem*> items = ui->GRAPH_TOKEN_LIST->selectedItems();

    for (int i = 0; i < items.size(); ++i)
    {
        int itemIndex = ui->GRAPH_TOKEN_LIST->row(items.at(i));
        m_tokens[itemIndex].name = "";
        m_tokens[itemIndex].type = eFDT_Void;
    }

    stl::find_and_erase_all(m_tokens, STokenData());

    RefreshControl();
}

void CFlowGraphTokensDlg::OnEditToken()
{
    QList<QListWidgetItem*> items = ui->GRAPH_TOKEN_LIST->selectedItems();

    if (items.size() == 1)
    {
        int item = ui->GRAPH_TOKEN_LIST->row(items.first());

        CFlowGraphNewTokenDlg dlg(&m_tokens[item], m_tokens);
        dlg.setTokenName(m_tokens[item].name);
        dlg.setTokenType(m_tokens[item].type);
        dlg.exec();

        RefreshControl();
    }
}

void CFlowGraphTokensDlg::RefreshControl()
{
    ui->GRAPH_TOKEN_LIST->clear();
    int i = 0;
    std::sort(m_tokens.begin(), m_tokens.end(), SortByAsc);
    for (TTokens::const_iterator it = m_tokens.begin(), end = m_tokens.end(); it != end; ++it, ++i)
    {
        QString text = it->name;
        text += " (";
        text += typeNames[it->type];
        text += ")";
        ui->GRAPH_TOKEN_LIST->addItem(text);
    }
}

//////////////////////////////////////////////////////////////////////////
// CFlowGraphNewTokenDlg
//////////////////////////////////////////////////////////////////////////

CFlowGraphNewTokenDlg::CFlowGraphNewTokenDlg(STokenData* pTokenData, TTokens tokenList, QWidget* pParent /*= NULL*/)
    : QDialog(pParent)
    , m_pTokenData(pTokenData)
    , m_tokens(tokenList)
    , ui(new Ui::CFlowGraphNewTokenDlg)
{
    ui->setupUi(this);
    // start at 1 to skip 'void' type
    for (int i = 1; i < sizeof(typeNames) / sizeof(QString); ++i)
    {
        ui->GRAPH_TOKEN_TYPE->addItem(typeNames[i]);
    }

    // Set the window title appropriately since this dialog is used both to
    // create new tokens and edit existing ones
    if (m_pTokenData && m_pTokenData->name.isEmpty())
    {
        setWindowTitle(tr("New Token"));
    }
    else
    {
        setWindowTitle(tr("Edit Token"));
    }

    // connect
    connect(ui->buttonBox, &QDialogButtonBox::accepted, this, &CFlowGraphNewTokenDlg::onOK);
    connect(ui->buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
}

CFlowGraphNewTokenDlg::~CFlowGraphNewTokenDlg()
{
}

void CFlowGraphNewTokenDlg::setTokenName(const QString& name)
{
    m_tokenName = name;
    ui->GRAPH_TOKEN_NAME->setText(m_tokenName);
}

void CFlowGraphNewTokenDlg::setTokenType(const EFlowDataTypes type)
{
    m_tokenType = int(type) - 1; // minus one to avoid "void" type
    ui->GRAPH_TOKEN_TYPE->setCurrentIndex(m_tokenType);
}

void CFlowGraphNewTokenDlg::onOK()
{
    // set token data from controls; close
    if (m_pTokenData)
    {
        QString newTokenName = ui->GRAPH_TOKEN_NAME->text();

        // handle 'edit' or 'new' token command
        if (m_pTokenData->name == newTokenName || !DoesTokenExist(newTokenName))
        {
            m_pTokenData->name = newTokenName;
            QString itemType = ui->GRAPH_TOKEN_TYPE->currentText();

            for (int i = 0; i < sizeof(typeNames) / sizeof(QString); ++i)
            {
                if (itemType == typeNames[i])
                {
                    m_pTokenData->type = (EFlowDataTypes)i;
                    break;
                }
            }
            accept();
        }
        else
        {
            QMessageBox::critical(this, "Editor", "Token name already in use");
        }
    }
}

bool CFlowGraphNewTokenDlg::DoesTokenExist(QString tokenName)
{
    for (int i = 0; i < m_tokens.size(); ++i)
    {
        if (m_tokens[i].name == tokenName)
        {
            return true;
        }
    }
    return false;
}

#include <HyperGraph/FlowGraphTokens.moc>