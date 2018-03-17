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

#ifndef CRYINCLUDE_EDITOR_HYPERGRAPH_FLOWGRAPHTOKENS_H
#define CRYINCLUDE_EDITOR_HYPERGRAPH_FLOWGRAPHTOKENS_H
#pragma once

#include <QDialog>
#include <QScopedPointer>

#include "FlowGraph.h"
#include "Resource.h"

class ReflectedPropertyControl;
class CHyperGraphDialog;
class CHyperGraph;

//////////////////////////////////////////////////////////////////////////
//  CFlowGraphTokensCtrl - read-only display of graph tokens
//      in the right-hand properties panel
//////////////////////////////////////////////////////////////////////////

class CFlowGraphTokensCtrl
    : public QWidget
{
public:
    CFlowGraphTokensCtrl(QWidget* pParent);
    virtual ~CFlowGraphTokensCtrl();

    void SetGraph(CHyperGraph* pGraph);
    void UpdateGraphProperties(CHyperGraph* pGraph);

protected:
    void FillProps();
    void ResizeProps(bool bHide = false);

protected:

    CSmartVariableEnum<QString> CreateVariable(const char* name, EFlowDataTypes type);

    CFlowGraph*        m_pGraph;

    ReflectedPropertyControl*      m_graphProps;

    bool m_bUpdate;
};

//////////////////////////////////////////////////////////////////////////
// CFlowGraphEditTokensDlg - popup dialog allowing adding /
//  removing / editing graph tokens
//////////////////////////////////////////////////////////////////////////

struct STokenData
{
    STokenData()
        : type(eFDT_Void)
    {}

    inline bool operator==(const STokenData& other)
    {
        return (name == other.name) && (type == other.type);
    }

    QString name;
    EFlowDataTypes type;
};
typedef std::vector<STokenData> TTokens;

namespace Ui {
    class CFlowGraphTokensDlg;
}

class CFlowGraphTokensDlg
    : public QDialog
{
public:
    CFlowGraphTokensDlg(QWidget* pParent = NULL);   // standard constructor
    virtual ~CFlowGraphTokensDlg();

    void RefreshControl();

    TTokens& GetTokenData() { return m_tokens; }
    void SetTokenData(const TTokens& tokens) { m_tokens = tokens; RefreshControl(); }

protected:
    static bool SortByAsc(const STokenData& lhs, const STokenData& rhs) { return lhs.name < rhs.name; }

    void OnAddNewToken();
    void OnDeleteToken();
    void OnEditToken();

    TTokens m_tokens;

    QScopedPointer<Ui::CFlowGraphTokensDlg> ui;
    Q_DISABLE_COPY(CFlowGraphTokensDlg);
};


//////////////////////////////////////////////////////////////////////////
// CFlowGraphNewTokenDlg - popup dialog creating a new graph token
//  or editing an existing one
//////////////////////////////////////////////////////////////////////////

namespace Ui {
    class CFlowGraphNewTokenDlg;
}

class CFlowGraphNewTokenDlg
    : public QDialog
{
    Q_OBJECT

public:
    CFlowGraphNewTokenDlg(STokenData* pTokenData, TTokens tokenList, QWidget* pParent = NULL);   // standard constructor
    virtual ~CFlowGraphNewTokenDlg();

    void setTokenName(const QString& name);
    void setTokenType(const EFlowDataTypes type);

protected:

    void RefreshControl();
    bool DoesTokenExist(QString tokenName);

    STokenData* m_pTokenData;
    TTokens m_tokens;

    QScopedPointer<Ui::CFlowGraphNewTokenDlg> ui;
    Q_DISABLE_COPY(CFlowGraphNewTokenDlg);

protected slots:
    void onOK();

private:
    // Dialog Data
    int m_tokenType;
    QString m_tokenName;
};


#endif // CRYINCLUDE_EDITOR_HYPERGRAPH_FLOWGRAPHTOKENS_H
