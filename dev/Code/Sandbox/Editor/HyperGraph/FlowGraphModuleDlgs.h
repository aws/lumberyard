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

#ifndef CRYINCLUDE_EDITOR_HYPERGRAPH_FLOWGRAPHMODULEDLGS_H
#define CRYINCLUDE_EDITOR_HYPERGRAPH_FLOWGRAPHMODULEDLGS_H
#pragma once


#include <IFlowGraphModuleManager.h>
#include "Resource.h"

#include <QDialog>
#include <QScopedPointer>

class CHyperGraph;
class QComboBox;

//////////////////////////////////////////////////////////////////////////
// CFlowGraphEditModuleDlg - popup dialog allowing adding /
//  removing / editing module inputs + outputs
//////////////////////////////////////////////////////////////////////////

namespace Ui {
    class CFlowGraphEditModuleDlg;
}

class CFlowGraphEditModuleDlg
    : public QDialog
{
    Q_OBJECT

public:
    CFlowGraphEditModuleDlg(IFlowGraphModule* pModule, QWidget* pParent = NULL);   // standard constructor
    virtual ~CFlowGraphEditModuleDlg();

    static QString GetDataTypeName(EFlowDataTypes type);
    static EFlowDataTypes GetDataType(const QString& itemType);
    static void FillDataTypes(QComboBox* comboBox);

protected:
    void RefreshControl();

    IFlowGraphModule* m_pModule;
    typedef std::vector<IFlowGraphModule::SModulePortConfig> TPorts;
    TPorts m_inputs;
    TPorts m_outputs;

protected slots:
    void OnCommand_NewInput();
    void OnCommand_DeleteInput();
    void OnCommand_EditInput();

    void OnCommand_NewOutput();
    void OnCommand_DeleteOutput();
    void OnCommand_EditOutput();

    virtual void OnOK();

private:
    void AddDataTypes();
    typedef std::map<EFlowDataTypes, QString> TDataTypes;
    static TDataTypes m_flowDataTypes;

    QScopedPointer<Ui::CFlowGraphEditModuleDlg> ui;
    Q_DISABLE_COPY(CFlowGraphEditModuleDlg);
};


//////////////////////////////////////////////////////////////////////////
// CFlowGraphNewModuleInputDlg - popup dialog creating a new module input/output
//  or editing an existing one
//////////////////////////////////////////////////////////////////////////
namespace Ui {
    class CFlowGraphNewModuleInputDlg;
}
class CFlowGraphNewModuleInputDlg
    : public QDialog
{
    Q_OBJECT

public:
    CFlowGraphNewModuleInputDlg(IFlowGraphModule::SModulePortConfig* pPort, QWidget* pParent = NULL);   // standard constructor
    virtual ~CFlowGraphNewModuleInputDlg();

protected:
    virtual void OnOK();

    IFlowGraphModule::SModulePortConfig* m_pPort;
    QScopedPointer<Ui::CFlowGraphNewModuleInputDlg> ui;
    Q_DISABLE_COPY(CFlowGraphNewModuleInputDlg);
};

//////////////////////////////////////////////////////////////////////////
// CFlowGraphNewDlg - Modal dialog invoked when creating a new component flowgraph
//////////////////////////////////////////////////////////////////////////

namespace Ui {
    class CFlowGraphNewDlg;
}

class SANDBOX_API CFlowGraphNewDlg
    : public QDialog
{
    Q_OBJECT

public:
    CFlowGraphNewDlg(const QString& title, const QString& text, QWidget* pParent = nullptr);
    ~CFlowGraphNewDlg() override;

    const QString& GetText() const { return m_text; }

    int exec() override;

protected:

    virtual void OnOK();

    QString m_text;

    QScopedPointer<Ui::CFlowGraphNewDlg> ui;
    Q_DISABLE_COPY(CFlowGraphNewDlg);
};


#endif // CRYINCLUDE_EDITOR_HYPERGRAPH_FLOWGRAPHMODULEDLGS_H
