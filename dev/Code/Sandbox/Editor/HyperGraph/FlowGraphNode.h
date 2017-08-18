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

#ifndef CRYINCLUDE_EDITOR_HYPERGRAPH_FLOWGRAPHNODE_H
#define CRYINCLUDE_EDITOR_HYPERGRAPH_FLOWGRAPHNODE_H
#pragma once

#include <IEntitySystem.h>
#include <IFlowSystem.h>

#include "HyperGraphNode.h"
#include "../Objects/EntityObject.h"

class CFlowNode;
class CEntityObject;

//////////////////////////////////////////////////////////////////////////
class SANDBOX_API CFlowNode
    : public CHyperNode
{
    friend class CFlowGraphManager;

public:
    CFlowNode();
    virtual ~CFlowNode();

    //////////////////////////////////////////////////////////////////////////
    // CHyperNode implementation.
    //////////////////////////////////////////////////////////////////////////
    virtual void Init();
    virtual void Done();
    virtual CHyperNode* Clone();
    virtual QString GetDescription() const;
    virtual void Serialize(XmlNodeRef& node, bool bLoading, CObjectArchive* ar);
    virtual void SetName(const char* sName);
    virtual void OnInputsChanged();
    virtual void OnEnteringGameMode();
    virtual void Unlinked(bool bInput);
    virtual QString GetPortName(const CHyperNodePort& port);
    virtual void PostClone(CBaseObject* pFromObject, CObjectCloneContext& ctx);
    virtual QColor GetCategoryColor();
    virtual void DebugPortActivation(TFlowPortId port, const char* value);
    virtual bool IsPortActivationModified(const CHyperNodePort* port = NULL);
    virtual void ClearDebugPortActivation();
    virtual QString GetDebugPortValue(const CHyperNodePort& pp);
    virtual void ResetDebugPortActivation(CHyperNodePort* port);
    virtual bool IsDebugPortActivated(CHyperNodePort* port);
    virtual bool IsObsolete() const { return m_flowSystemNodeFlags & EFLN_OBSOLETE; }
    virtual TFlowNodeTypeId GetTypeId();
    //////////////////////////////////////////////////////////////////////////

    //////////////////////////////////////////////////////////////////////////
    // FlowSystem Flags. We don't want to replicate those again....
    //////////////////////////////////////////////////////////////////////////
    uint32 GetCoreFlags() const { return m_flowSystemNodeFlags & EFLN_CORE_MASK; }
    uint32 GetCategory() const { return m_flowSystemNodeFlags & EFLN_CATEGORY_MASK; }
    uint32 GetUsageFlags() const { return m_flowSystemNodeFlags & EFLN_USAGE_MASK; }
    QString GetCategoryName() const;
    QString GetUIClassName() const override;

    void SetEntity(CEntityObject* pEntity);
    CEntityObject* GetEntity() const;



    // Takes selected entity as target entity.
    void SetSelectedEntity();
    // Takes graph default entity as target entity.
    void SetDefaultEntity();
    CEntityObject* GetDefaultEntity() const;

    // Returns IFlowNode.
    IFlowGraph* GetIFlowGraph() const;

    // Return ID of the flow node.
    TFlowNodeId GetFlowNodeId() { return m_flowNodeId; }

    void SetInputs(bool bActivate, bool bForceResetEntities = false);
    virtual CVarBlock* GetInputsVarBlock();

    virtual QString GetTitle() const;

    virtual IUndoObject* CreateUndo();
    virtual bool IsEntityValid() const;
    virtual QString GetEntityTitle() const;

    void ResolveEntityId() override;

protected:
    GUID m_entityGuid;
    _smart_ptr<CEntityObject> m_pEntity;
    uint32 m_flowSystemNodeFlags;
    TFlowNodeId m_flowNodeId;
    AZStd::string m_szUIClassName;
    const char* m_szDescription;
    std::map<TFlowPortId, string> m_portActivationMap;
    std::vector<TFlowPortId> m_debugPortActivations;
    bool m_needsEntityIdResolve;
};


#endif // CRYINCLUDE_EDITOR_HYPERGRAPH_FLOWGRAPHNODE_H
