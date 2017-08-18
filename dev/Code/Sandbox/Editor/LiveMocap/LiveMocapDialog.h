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

#ifndef CRYINCLUDE_EDITOR_LIVEMOCAP_LIVEMOCAPDIALOG_H
#define CRYINCLUDE_EDITOR_LIVEMOCAP_LIVEMOCAPDIALOG_H
#pragma once


#include "LiveMocap.h"

#include "../Dialogs/BaseFrameWnd.h"
#include "../Controls/SplitterCtrl.h"

#include "../PropertiesPanel.h"

#include "../Animation/SkeletonMapperGraph.h"

#include <QScopedPointer>
#include <QWidget>

static const uint LIVEMOCAP_CONNECTION_MAX = 30;

class CLiveMocapTreeConnection
    : public CTreeCtrl
    , public ILiveMocapSceneListener
{
public:
    enum EID
    {
        eID = 2000,

        eID_AddConnection,
        eID_AddConnectionLast = eID_AddConnection + LIVEMOCAP_CONNECTION_MAX,

        eID_Connection_Connect,
        eID_Connection_Disconnect,
        eID_Connection_SetEntity,

        eID_Actor_SetEntity,
        eID_Actor_UnSetEntity,

        eID_Actor_MappingGraph_New,
        eID_Actor_MappingGraph_Open,
    };

public:
    CLiveMocapTreeConnection();
    ~CLiveMocapTreeConnection();

public:
    void Create(CWnd* pParent, UINT nID);

protected:
    DECLARE_MESSAGE_MAP()

    afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
    afx_msg void OnRButtonUp(UINT nFlags, CPoint point);

    afx_msg void OnAddConnection(UINT nID);

    afx_msg void OnConnection_Connect();
    afx_msg void OnConnection_Disconnect();
    afx_msg void OnConnection_SetEntity();

    afx_msg void OnActor_SetEntity();
    afx_msg void OnActor_UnSetEntity();
    afx_msg void OnActor_MappingGraph_New();
    afx_msg void OnActor_MappingGraph_Open();

private:
    void CreateMenuConnection(CLiveMocapConnection& connection, CMenu& menu);

    CLiveMocapConnection* IsConnection(HTREEITEM hItem);
    CLiveMocapActor* IsActor(HTREEITEM hItem);

    HTREEITEM FindConnectionItem(const CLiveMocapConnection* pConnection);
    HTREEITEM FindConnectionItem(const CLiveMocapScene* pScene);

    IEntity* GetEditorSelectedEntity();

public:
    CLiveMocapConnection* GetSelectedConnection();
    CLiveMocapActor* GetSelectedActor();

    void RefreshConnection(HTREEITEM hConnection, bool bForceExpand = false);

    string GetSubjectName(const CLiveMocapActor* pActor);
    void RefreshConnections();

    // ILiveMocapSceneListener
public:
    void OnCreateActor(const CLiveMocapScene& scene, const CLiveMocapActor& actor);

private:
    CMenu m_menuConnection;
    CMenu m_menuActor;
    CMenu m_menuMappingGraph;

    // TEMP
public:
    Skeleton::CMapperGraphView* m_pGraphView;
};

class CLiveMocapDialog
    : public CBaseFrameWnd
    , public Skeleton::IMapperGraphViewListener
{
    DECLARE_DYNCREATE(CLiveMocapDialog)

private:
    enum EID
    {
        eID = 2000,

        eID_Tree,

        eID_Connect,
        eID_SetEntity,
    };

public:
    static void RegisterViewClass();

public:
    CLiveMocapDialog();
    virtual ~CLiveMocapDialog();

private:
    void DisplayConnectioProperties(CLiveMocapConnection* pConnection);
    void DisplayActorProperties(CLiveMocapActor* pActor);

protected:
    virtual BOOL OnInitDialog();

    virtual void DoDataExchange(CDataExchange* pDX);

protected:
    DECLARE_MESSAGE_MAP()

    afx_msg void OnTreeSelection(NMHDR*, LRESULT*);

    // Skeleton::IMapperGraphViewListener
public:
    virtual void OnSelection(const std::vector<CHyperNode*>& previous, const std::vector<CHyperNode*>& current);

private:
    CXTSplitterWnd m_splitters[2];
    CLiveMocapTreeConnection m_tree;
    CPropertiesPanel m_properties;

    CPropertiesPanel m_graphProperties;
    QScopedPointer<Skeleton::CMapperGraphViewWrapper> m_graphViewWrapper;
    Skeleton::CMapperGraphView* m_graphView;
};

#endif // CRYINCLUDE_EDITOR_LIVEMOCAP_LIVEMOCAPDIALOG_H
