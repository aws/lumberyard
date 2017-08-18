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

#ifndef CRYINCLUDE_EDITOR_HYPERGRAPH_FLOWGRAPHSEARCHCTRL_H
#define CRYINCLUDE_EDITOR_HYPERGRAPH_FLOWGRAPHSEARCHCTRL_H
#pragma once

#include <AzQtComponents/Components/StyledDockWidget.h>

#include <QDockWidget>
#include <QScopedPointer>


namespace Ui {
    class FlowGraphSearchCtrl;
}

#include "FlowGraph.h"

// fwd declare
class CFlowGraphSearchResultsCtrl;
class CHyperGraphDialog;

class CFlowGraphSearchOptions
{
public:
    enum EFindLocation
    {
        eFL_Current = 0,
        eFL_AIActions,
        eFL_CustomActions,
        eFL_Entities,
        eFL_All
    };

    enum EFindSpecial
    {
        eFLS_None = 0,
        eFLS_NoEntity,
        eFLS_NoLinks,
        eFLS_Approved,
        eFLS_Advanced,
        eFLS_Debug,
        //eFLS_Legacy,
        //eFLS_WIP,
        //eFLS_NoCategory,
        eFLS_Obsolete,
    };

public:
    static CFlowGraphSearchOptions* GetSearchOptions();

    void Serialize(bool bLoading); // serialize to/from registry

public:
    BOOL           m_bIncludePorts;
    BOOL           m_bIncludeValues;
    BOOL       m_bIncludeEntities;
    BOOL       m_bIncludeIDs;
    BOOL       m_bExactMatch;
    QString    m_strFind;
    int        m_LookinIndex;
    int        m_findSpecial;

    QStringList m_lstFindHistory;

protected:
    CFlowGraphSearchOptions();
    CFlowGraphSearchOptions(const CFlowGraphSearchOptions&);
};

//////////////////////////////////////////////////////////////////////////
class CFlowGraphSearchCtrl
    : public AzQtComponents::StyledDockWidget
{
    Q_OBJECT
public:
    CFlowGraphSearchCtrl(CHyperGraphDialog* pDialog);
    virtual ~CFlowGraphSearchCtrl();

    // called from CHyperGraphDialog...
    void SetResultsCtrl(CFlowGraphSearchResultsCtrl* pCtrl);

    void Find(const QString& searchString, bool bSetTextOnly = false, bool bSelectFirst = false, bool bTempExactMatch = false);

    typedef struct Result
    {
        Result(CFlowGraph* pGraph, HyperNodeID nodeId, const QString& context)
        {
            m_pGraph = pGraph;
            m_nodeId = nodeId;
            m_context = context;
        }
        CFlowGraph* m_pGraph;
        HyperNodeID m_nodeId;
        QString     m_context;
    } Result;
    typedef std::vector<Result> ResultVec;

protected:
    void UpdateOptions();
    void DoTheFind(ResultVec& vec);
    void FindAll(bool bSelectFirst = false);

protected:
    CFlowGraphSearchResultsCtrl* m_pResultsCtrl;
    CHyperGraphDialog* m_pDialog;

    QScopedPointer<Ui::FlowGraphSearchCtrl> ui;

protected slots:
    void OnButtonFindAll();
    void OnIncludePortsClicked();
    void OnIncludeValuesClicked();
    void OnIncludeEntitiesClicked();
    void OnIncludeIDsClicked();
    void OnOptionChanged();
};


class CFlowGraphSearchResultsModel;
class CFlowGraphSearchResultsProxyModel;
class CFlowGraphSearchResultsView;


class CFlowGraphSearchResultsCtrl
    : public AzQtComponents::StyledDockWidget
{
    Q_OBJECT
public:
    enum
    {
        IDD_FG_SEARCH_RESULTS = 2410
    };
    enum
    {
        IDD = IDD_FG_SEARCH_RESULTS
    };

    CFlowGraphSearchResultsCtrl(CHyperGraphDialog* pDialog);
    ~CFlowGraphSearchResultsCtrl() {}

    void SelectFirstResult();
    void ShowSelectedResult();

    void Populate(const CFlowGraphSearchCtrl::ResultVec& results);

protected slots:
    void OnReportColumnRClick(const QPoint& pos);

protected:
    void GroupBy(int pColumn); // pColumn may be 0 -> ungroup
    CHyperGraphDialog* m_pDialog;
    CFlowGraphSearchResultsView* m_dataView;
    CFlowGraphSearchResultsModel* m_model;
    CFlowGraphSearchResultsProxyModel* m_proxyModel;
};


#endif // CRYINCLUDE_EDITOR_HYPERGRAPH_FLOWGRAPHSEARCHCTRL_H
