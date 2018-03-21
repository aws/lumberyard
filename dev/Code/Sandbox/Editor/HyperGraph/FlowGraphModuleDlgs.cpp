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
#include "FlowGraphModuleDlgs.h"
#include "HyperGraphDialog.h"
#include "FlowGraphModuleManager.h"
#include "FlowGraphManager.h"
#include "FlowGraph.h"
#include <HyperGraph/ui_FlowGraphEditModuleDlg.h>
#include <HyperGraph/ui_FlowGraphNewModuleInputDlg.h>
#include <HyperGraph/ui_FlowGraphNewDlg.h>
#include "QtUtilWin.h"
#include <QMessageBox>

//////////////////////////////////////////////////////////////////////////
// CFlowGraphEditModuleDlg
//////////////////////////////////////////////////////////////////////////

std::map<EFlowDataTypes, QString> CFlowGraphEditModuleDlg::m_flowDataTypes;

CFlowGraphEditModuleDlg::CFlowGraphEditModuleDlg(IFlowGraphModule* pModule, QWidget* pParent /*=NULL*/)
    : QDialog(pParent)
    , m_pModule(pModule)
    , ui(new Ui::CFlowGraphEditModuleDlg)
{
    assert(m_pModule);
    ui->setupUi(this);

    // extract copy of ports from graph, so we can edit locally yet still cancel changes
    for (size_t i = 0; i < m_pModule->GetModulePortCount(); ++i)
    {
        const IFlowGraphModule::SModulePortConfig* pPort = m_pModule->GetModulePort(i);
        if (pPort->input)
        {
            m_inputs.push_back(*pPort);
        }
        else
        {
            m_outputs.push_back(*pPort);
        }
    }
    AddDataTypes();
    RefreshControl();

    ui->FG_MODULE_OUTPUT_LIST->setSelectionMode(QAbstractItemView::MultiSelection);
    ui->FG_MODULE_INPUT_LIST->setSelectionMode(QAbstractItemView::MultiSelection);

    connect(ui->ADD_MODULE_INPUT, &QPushButton::clicked, this, &CFlowGraphEditModuleDlg::OnCommand_NewInput);
    connect(ui->EDIT_MODULE_INPUT, &QPushButton::clicked, this, &CFlowGraphEditModuleDlg::OnCommand_EditInput);
    connect(ui->DELETE_MODULE_INPUT, &QPushButton::clicked, this, &CFlowGraphEditModuleDlg::OnCommand_DeleteInput);

    connect(ui->ADD_MODULE_OUTPUT, &QPushButton::clicked, this, &CFlowGraphEditModuleDlg::OnCommand_NewOutput);
    connect(ui->EDIT_MODULE_OUTPUT, &QPushButton::clicked, this, &CFlowGraphEditModuleDlg::OnCommand_EditOutput);
    connect(ui->DELETE_MODULE_OUTPUT, &QPushButton::clicked, this, &CFlowGraphEditModuleDlg::OnCommand_DeleteOutput);

    connect(ui->buttonBox, &QDialogButtonBox::accepted, this, &CFlowGraphEditModuleDlg::OnOK);
}

CFlowGraphEditModuleDlg::~CFlowGraphEditModuleDlg()
{
}

void CFlowGraphEditModuleDlg::AddDataTypes()
{
    m_flowDataTypes[eFDT_Any]           = "Any";
    m_flowDataTypes[eFDT_Int]           = "Int";
    m_flowDataTypes[eFDT_Float]         = "Float";
    m_flowDataTypes[eFDT_String]        = "String";
    m_flowDataTypes[eFDT_Vec3]          = "Vec3";
    m_flowDataTypes[eFDT_EntityId]      = "EntityId";
    m_flowDataTypes[eFDT_Bool]          = "Bool";
    m_flowDataTypes[eFDT_CustomData]    = "Custom";
}

//////////////////////////////////////////////////////////////////////////
void CFlowGraphEditModuleDlg::OnCommand_NewInput()
{
    IFlowGraphModule::SModulePortConfig input;
    input.input = true;
    input.type = eFDT_Any;
    CFlowGraphNewModuleInputDlg dlg(&input);
    if (dlg.exec() == QDialog::Accepted && input.name != "" && input.type != eFDT_Void)
    {
        if (isdigit(input.name[0]))
        {
            QMessageBox::critical(this, "Editor", "Input name should not start with a number!");
            return;
        }
        m_inputs.push_back(input);
    }
    RefreshControl();
}

//////////////////////////////////////////////////////////////////////////
void CFlowGraphEditModuleDlg::OnCommand_DeleteInput()
{
    QList<QListWidgetItem*> items = ui->FG_MODULE_INPUT_LIST->selectedItems();

    for (int i = 0; i < items.size(); ++i)
    {
        int itemIndex = ui->FG_MODULE_INPUT_LIST->row(items.at(i));
        m_inputs[itemIndex] = IFlowGraphModule::SModulePortConfig();
    }

    stl::find_and_erase_all(m_inputs, IFlowGraphModule::SModulePortConfig());
    RefreshControl();
}

//////////////////////////////////////////////////////////////////////////
void CFlowGraphEditModuleDlg::OnCommand_EditInput()
{
    QList<QListWidgetItem*> items = ui->FG_MODULE_INPUT_LIST->selectedItems();

    if (items.size() == 1)
    {
        int item = ui->FG_MODULE_INPUT_LIST->row(items.first());
        CFlowGraphNewModuleInputDlg dlg(&m_inputs[item]);
        if (dlg.exec() == QDialog::Accepted)
        {
        }
        RefreshControl();
    }
}

//////////////////////////////////////////////////////////////////////////
void CFlowGraphEditModuleDlg::OnCommand_NewOutput()
{
    IFlowGraphModule::SModulePortConfig output;
    output.input = false;
    output.type = eFDT_Any;
    CFlowGraphNewModuleInputDlg dlg(&output);
    if (dlg.exec() == QDialog::Accepted && output.name != "" && output.type != eFDT_Void)
    {
        if (isdigit(output.name[0]))
        {
            QMessageBox::critical(this, "Editor", "Input name should not start with a number!");
            return;
        }
        m_outputs.push_back(output);
    }
    RefreshControl();
}

//////////////////////////////////////////////////////////////////////////
void CFlowGraphEditModuleDlg::OnCommand_DeleteOutput()
{
    QList<QListWidgetItem*> items = ui->FG_MODULE_OUTPUT_LIST->selectedItems();

    for (int i = 0; i < items.size(); ++i)
    {
        int itemIndex = ui->FG_MODULE_OUTPUT_LIST->row(items.at(i));
        m_outputs[itemIndex] = IFlowGraphModule::SModulePortConfig();
    }

    stl::find_and_erase_all(m_outputs, IFlowGraphModule::SModulePortConfig());
    RefreshControl();
}

//////////////////////////////////////////////////////////////////////////
void CFlowGraphEditModuleDlg::OnCommand_EditOutput()
{
    QList<QListWidgetItem*> items = ui->FG_MODULE_OUTPUT_LIST->selectedItems();

    if (items.size() == 1)
    {
        int item = ui->FG_MODULE_OUTPUT_LIST->row(items.first());
        CFlowGraphNewModuleInputDlg dlg(&m_outputs[item]);
        if (dlg.exec() == QDialog::Accepted)
        {
        }
        RefreshControl();
    }
}

//////////////////////////////////////////////////////////////////////////
void CFlowGraphEditModuleDlg::RefreshControl()
{
    ui->FG_MODULE_INPUT_LIST->clear();
    ui->FG_MODULE_OUTPUT_LIST->clear();

    for (TPorts::const_iterator it = m_inputs.begin(), end = m_inputs.end(); it != end; ++it)
    {
        assert(it->input);

        QString text = QString(it->name);
        text += " (";
        text += GetDataTypeName(it->type);
        text += ")";
        ui->FG_MODULE_INPUT_LIST->addItem(text);
    }

    for (TPorts::const_iterator it = m_outputs.begin(), end = m_outputs.end(); it != end; ++it)
    {
        assert(!it->input);

        QString text = QString::fromLatin1(it->name.c_str());
        text += " (";
        text += GetDataTypeName(it->type);
        text += ")";
        ui->FG_MODULE_OUTPUT_LIST->addItem(text);
    }
}

//////////////////////////////////////////////////////////////////////////
void CFlowGraphEditModuleDlg::OnOK()
{
    if (m_pModule)
    {
        // write inputs/outputs back to graph
        m_pModule->RemoveModulePorts();

        for (TPorts::const_iterator it = m_inputs.begin(), end = m_inputs.end(); it != end; ++it)
        {
            assert(it->input);

            m_pModule->AddModulePort(*it);
        }

        for (TPorts::const_iterator it = m_outputs.begin(), end = m_outputs.end(); it != end; ++it)
        {
            assert(!it->input);
            m_pModule->AddModulePort(*it);
        }

        GetIEditor()->GetFlowGraphModuleManager()->CreateModuleNodes(m_pModule->GetName());
        accept();
        return;
    }
    reject();
}

QString CFlowGraphEditModuleDlg::GetDataTypeName(EFlowDataTypes type)
{
    return stl::find_in_map(m_flowDataTypes, type, "");
}

EFlowDataTypes CFlowGraphEditModuleDlg::GetDataType(const QString& itemType)
{
    for (TDataTypes::iterator iter = m_flowDataTypes.begin(); iter != m_flowDataTypes.end(); ++iter)
    {
        if (iter->second == itemType)
        {
            return iter->first;
        }
    }
    return eFDT_Void;
}

void CFlowGraphEditModuleDlg::FillDataTypes(QComboBox* comboBox)
{
    for (TDataTypes::iterator iter = m_flowDataTypes.begin(); iter != m_flowDataTypes.end(); ++iter)
    {
        comboBox->addItem(iter->second);
    }
}

//////////////////////////////////////////////////////////////////////////
// CFlowGraphNewModuleInputDlg
//////////////////////////////////////////////////////////////////////////

CFlowGraphNewModuleInputDlg::CFlowGraphNewModuleInputDlg(IFlowGraphModule::SModulePortConfig* pPort, QWidget* pParent /*=NULL*/)
    : QDialog(pParent)
    , m_pPort(pPort)
    , ui(new Ui::CFlowGraphNewModuleInputDlg)
{
    ui->setupUi(this);

    CFlowGraphEditModuleDlg::FillDataTypes(ui->MODULE_INPUT_TYPE);

    // input/output names only supports ASCII characters
    QRegExp rx("[_a-zA-Z0-9][_a-zA-Z0-9-]*");
    QValidator* validator = new QRegExpValidator(rx, this);
    ui->MODULE_INPUT_NAME->setValidator(validator);

    ui->MODULE_INPUT_NAME->setText(QString(m_pPort->name));

    const QString typeName = CFlowGraphEditModuleDlg::GetDataTypeName(m_pPort->type);
    int typeIndex = ui->MODULE_INPUT_TYPE->findText(typeName); // we handle any value being -1;
    ui->MODULE_INPUT_TYPE->setCurrentIndex(typeIndex);

    if (m_pPort->input)
    {
        setWindowTitle("Edit Module Input");
    }
    else
    {
        setWindowTitle("Edit Module Output");
    }

    // connect
    connect(ui->buttonBox, &QDialogButtonBox::accepted, this, &CFlowGraphNewModuleInputDlg::OnOK);
    connect(ui->buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
}

CFlowGraphNewModuleInputDlg::~CFlowGraphNewModuleInputDlg()
{
}

void CFlowGraphNewModuleInputDlg::OnOK()
{
    if (m_pPort)
    {
        QString itemName = ui->MODULE_INPUT_NAME->text();
        m_pPort->name = itemName.toLocal8Bit().constData();

        QString itemType = ui->MODULE_INPUT_TYPE->currentText();
        m_pPort->type = CFlowGraphEditModuleDlg::GetDataType(itemType);
        accept();
        return;
    }
    reject();
}

CFlowGraphNewDlg::CFlowGraphNewDlg(const QString& title, const QString& text, QWidget* pParent /*=NULL*/)
    : QDialog(pParent)
    , ui(new Ui::CFlowGraphNewDlg)
    , m_text(text)
{
    ui->setupUi(this);

    setWindowTitle(title);

    connect(ui->buttonBox, &QDialogButtonBox::accepted, this, &CFlowGraphNewDlg::OnOK);
    connect(ui->buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
}

void CFlowGraphNewDlg::OnOK()
{
    QString itemName = ui->FLOWGRAPH_NAME->text();
    m_text = itemName.toLocal8Bit().constData();

    accept();
}

CFlowGraphNewDlg::~CFlowGraphNewDlg()
{
}

int CFlowGraphNewDlg::exec()
{
    // Disable editor's accelerator to avoid triggering hotkeys.
    GetIEditor()->EnableAcceleratos(false);

    int result = QDialog::exec();

    GetIEditor()->EnableAcceleratos(true);

    return result;
}

#include <HyperGraph/FlowGraphModuleDlgs.moc>
