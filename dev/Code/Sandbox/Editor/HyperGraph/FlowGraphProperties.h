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

#ifndef CRYINCLUDE_EDITOR_HYPERGRAPH_FLOWGRAPHPROPERTIES_H
#define CRYINCLUDE_EDITOR_HYPERGRAPH_FLOWGRAPHPROPERTIES_H
#pragma once


#include "FlowGraph.h"

#include <QWidget>
#include <QScopedPointer>

class ReflectedPropertyControl;
class CHyperGraphDialog;
class CHyperGraph;

namespace Ui
{
    class FlowGraphProperties;
}



class CFlowGraphProperties
    : public QWidget
{
public:
    CFlowGraphProperties(QWidget* parent);
    virtual ~CFlowGraphProperties();

    void SetDialog(CHyperGraphDialog* dlg) { m_pParent = dlg; }
    void SetGraph(CHyperGraph* pGraph);
    void UpdateGraphProperties(CHyperGraph* pGraph);

    void SetDescription(QString new_desc);

    bool IsShowMultiPlayer()
    {
        return m_bShowMP;
    }

    void ShowMultiPlayer(bool bShow);

protected:
    void FillProps();
    void OnVarChange(IVariable* pVar);
    void ResizeProps(bool bHide);

protected:
    CFlowGraph*        m_pGraph;
    CHyperGraphDialog* m_pParent;

    ReflectedPropertyControl*      m_graphProps;

    CSmartVariable<bool> m_varEnabled;
    CSmartVariableEnum<int> m_varMultiPlayer;

    bool m_bUpdate;
    bool m_bShowMP;

    QScopedPointer<Ui::FlowGraphProperties> ui;
};

#endif // CRYINCLUDE_EDITOR_HYPERGRAPH_FLOWGRAPHPROPERTIES_H
