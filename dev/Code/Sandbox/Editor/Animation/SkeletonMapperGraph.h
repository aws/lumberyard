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

#ifndef CRYINCLUDE_EDITOR_ANIMATION_SKELETONMAPPERGRAPH_H
#define CRYINCLUDE_EDITOR_ANIMATION_SKELETONMAPPERGRAPH_H
#pragma once


#include "../Dialogs/BaseFrameWnd.h"
#include "../PropertiesPanel.h"

#include "../HyperGraph/HyperGraph.h"
#include "../HyperGraph/HyperGraphManager.h"
#include "../HyperGraph/HyperGraphWidget.h"

#include "SkeletonMapper.h"
#include <QScopedPointer>
#include <QWidget>

class QWinWidget;

namespace Skeleton {
    class CMapperGraph;
    class CMapperGraphManager;

    //

    class CMapperGraph
        : public CHyperGraph
        , public IHyperGraphListener
    {
    public:
        static CMapperGraph* Create();

    public:
        CMapperGraph(CHyperGraphManager* pManager);
        virtual ~CMapperGraph();

    public:
        void UpdateMapper();

        CMapper& GetMapper() { return m_mapper; }

        // NOTE: Coupled functionality, make utility class?
        void AddNode(const char* name);
        int32 FindNode_(const char* name);
        void ClearNodes();
        uint32 GetNodeCount() { return (uint32)m_nodes.size(); }
        const char* GetNodeName(uint32 index) { return m_nodes[index]; }

        // NOTE: Coupled functionality, make utility class?
        void AddLocation(const char* name);
        int32 FindLocation(const char* name);
        void ClearLocations();
        uint32 GetLocationCount() { return (uint32)m_locations.size(); }
        const char* GetLocationName(uint32 index) { return m_locations[index]; }

        bool Initialize();

        bool SerializeTo(XmlNodeRef& node);
        bool SerializeFrom(XmlNodeRef& node);

        // IHyperGraphListener
    public:
        void OnHyperGraphEvent(IHyperNode* pNode, EHyperGraphEvent event) { }
        void OnLinkEdit(CHyperEdge* pEdge) { }

    private:
        std::vector<string> m_nodes;
        std::vector<string> m_locations;

        CMapper m_mapper;
    };

    class CMapperGraphManager
        : public CHyperGraphManager
    {
    public:
        static CMapperGraphManager& Instance()
        {
            static CMapperGraphManager instance;
            return instance;
        };

    public:
        class CNode
            : public CHyperNode
        {
        public:
            CNode();
            ~CNode();

            // CHyperNode
        public:
            virtual void Init() { }
            virtual CHyperNode* Clone() { return new CNode(); }
        };

        class COperator
            : public CHyperNode
        {
        public:
            COperator(CMapperOperator* pOperator);
            ~COperator();

        public:
            void SetOperator(CMapperOperator* pOperator) { m_operator = pOperator; }
            CMapperOperator* GetOperator() { return m_operator; }

            bool GetPositionIndexFromPort(uint16 port, uint32& index);
            bool GetOrientationIndexFromPort(uint16 port, uint32& index);

            bool SetInput(uint16 port, CMapperGraphManager::COperator* pOperator);

            // CHyperNode
        public:
            virtual void Init() { }
            virtual void Serialize(XmlNodeRef& node, bool bLoading, CObjectArchive* ar);

        private:
            _smart_ptr<CMapperOperator> m_operator;
        };

        class CLocation
            : public COperator
        {
        public:
            CLocation()
                : COperator(new CMapperLocation()) { }

            // CHyperNode
        public:
            virtual void SetName(const char* name)
            {
                COperator::SetName(name);

                ((CMapperLocation*)GetOperator())->SetName(name);
            }

            virtual CHyperNode* Clone() { return new CLocation(); }
        };

        class COperatorIndexed
            : public COperator
        {
        public:
            COperatorIndexed(uint32 index);

        public:
            virtual CHyperNode* Clone() { return new COperatorIndexed(m_index); }

        private:
            uint32 m_index;
        };

    private:
        CMapperGraphManager();

    public:
        CMapperGraph* Load(const char* path);

        // CHyperGraphManager
    public:
        virtual void ReloadClasses();

        virtual CHyperGraph* CreateGraph();
        virtual CHyperNode* CreateNode(CHyperGraph* pGraph, const char* sNodeClass, HyperNodeID nodeId, const QPointF& pos);
    };

    struct IMapperGraphViewListener
    {
        virtual void OnSelection(const std::vector<CHyperNode*>& previous, const std::vector<CHyperNode*>& current) = 0;
    };

    class CMapperGraphView;
    class CMapperGraphViewWrapper
        : public CWnd
    {
    public:
        CMapperGraphViewWrapper(CWnd* parent);
        DECLARE_DYNAMIC(CMapperGraphViewWrapper)

        QScopedPointer<QWinWidget> m_widget;
        CMapperGraphView* m_view;

    protected:
        afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
        afx_msg void OnSize(UINT nType, int cx, int cy);
        DECLARE_MESSAGE_MAP()
    };


    class CMapperGraphView
        : public QHyperGraphWidget
    {
    private:
        enum EID
        {
            eID = 2000,

            eID_Delete,
            eID_Save,
            //      eID_Open,

            eID_Location,
            eID_LocationEnd = eID_Location + 500,

            eID_Node = eID_LocationEnd,
            eID_NodeEnd = eID_Node + 500,

            eID_Operator = eID_NodeEnd,
            eID_OperatorEnd = eID_Operator + 500,
        };

    public:
        CMapperGraphView();
        ~CMapperGraphView();

    public:
        CMapperGraph* GetGraph()
        {
            IHyperGraph* pGraph = QHyperGraphWidget::GetGraph();
            if (!pGraph)
            {
                return NULL;
            }

            return (CMapperGraph*)pGraph;
        }

        bool AddListener(IMapperGraphViewListener* pListener) { return stl::push_back_unique(m_listeners, pListener); }
        bool RemoveListener(IMapperGraphViewListener* pListener) { return stl::find_and_erase(m_listeners, pListener); }

    private:
        bool CreateMenuLocation(QMenu& menu);
        bool CreateMenuNode(QMenu& menu);
        bool CreateMenuOperator(QMenu& menu);

        CHyperNode* FindNode(const char* className, const char* name);
        CHyperNode* CreateNode(const char* className, const char* name, const QPoint& point, bool bUnique = true);

        bool CreateLocation(uint32 index, const QPoint& point);
        bool CreateNode(uint32 index, const QPoint& point);
        CMapperGraphManager::COperator* CreateOperator(const char* className, const char* name, CMapperOperator* pOperator, const QPoint& point, bool bUnique = false);
        CMapperGraphManager::COperator* CreateOperator(uint32 index, const QPoint& point);

        void Save();
        //  void Open();

    protected:
        void ShowContextMenu(const QPoint& point, CHyperNode* pNode) override;

        // IHyperGraphListener
    public:
        virtual void OnHyperGraphEvent(IHyperNode* pNode, EHyperGraphEvent event);

    private:
        DynArray<IMapperGraphViewListener*> m_listeners;
        std::vector<CHyperNode*> m_selection;
    };

    class CMapperDialog
        : public CBaseFrameWnd
        , public IMapperGraphViewListener
    {
        DECLARE_DYNCREATE(CMapperDialog)

    public:
        static void RegisterViewClass();

    public:
        CMapperDialog();
        virtual ~CMapperDialog();

    protected:
        virtual BOOL OnInitDialog();

        // Skeleton::IMapperGraphViewListener
    public:
        virtual void OnSelection(const std::vector<CHyperNode*>& previous, const std::vector<CHyperNode*>& current)
        {
        }

    private:
        CXTSplitterWnd m_splitter;
        QScopedPointer<Skeleton::CMapperGraphViewWrapper> m_graphViewWrapper;
        Skeleton::CMapperGraphView* m_graphView;
        CPropertiesPanel m_properties;
    };
} // namespace Skeleton

#endif // CRYINCLUDE_EDITOR_ANIMATION_SKELETONMAPPERGRAPH_H
