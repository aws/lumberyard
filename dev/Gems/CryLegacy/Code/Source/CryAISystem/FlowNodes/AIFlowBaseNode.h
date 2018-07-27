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

#ifndef CRYINCLUDE_CRYAISYSTEM_FLOWNODES_AIFLOWBASENODE_H
#define CRYINCLUDE_CRYAISYSTEM_FLOWNODES_AIFLOWBASENODE_H
#pragma once

#include <IFlowSystem.h>
#include "AILog.h"

//////////////////////////////////////////////////////////////////////////
// Enum used for templating node base class
//////////////////////////////////////////////////////////////////////////
enum ENodeCloneType
{
    eNCT_Singleton,     // node has only one instance; not cloned
    eNCT_Instanced      // node is fully cloned
};


//////////////////////////////////////////////////////////////////////////
// Auto registration for flow nodes. Handles both instanced and
//  singleton nodes
//////////////////////////////////////////////////////////////////////////

struct AIFlowBaseNode
{
    static void RegisterFactory();
};

class CAIAutoRegFlowNodeBase
    : public IFlowNodeFactory
{
public:
    CAIAutoRegFlowNodeBase(const char* sClassName)
    {
        m_sClassName = sClassName;
        m_pNext = 0;
        if (!m_pLast)
        {
            m_pFirst = this;
        }
        else
        {
            m_pLast->m_pNext = this;
        }

        m_pLast = this;
    }

    void AddRef() {}
    void Release() {}

    void Reset() {}

    virtual void GetMemoryUsage(ICrySizer* s) const
    {
        SIZER_SUBCOMPONENT_NAME(s, "CAIAutoRegFlowNode");
    }

    //////////////////////////////////////////////////////////////////////////
    const char* m_sClassName;
    CAIAutoRegFlowNodeBase* m_pNext;
    static CAIAutoRegFlowNodeBase* m_pFirst;
    static CAIAutoRegFlowNodeBase* m_pLast;
    //////////////////////////////////////////////////////////////////////////
};

template <class T>
class CAIAutoRegFlowNode
    : public CAIAutoRegFlowNodeBase
{
public:
    CAIAutoRegFlowNode(const char* sClassName)
        : CAIAutoRegFlowNodeBase(sClassName)
    {
        PREFAST_SUPPRESS_WARNING(6326)
        if (T::myCloneType == eNCT_Singleton)
        {
            m_pInstance = new T(NULL);
        }
    }

    IFlowNodePtr Create(IFlowNode::SActivationInfo* pActInfo)
    {
        PREFAST_SUPPRESS_WARNING(6326)
        if (T::myCloneType == eNCT_Singleton)
        {
            return m_pInstance;
        }
        else if (T::myCloneType == eNCT_Instanced)
        {
            return new T(pActInfo);
        }
        else
        {
            assert(false);
        }
    }

private:
    IFlowNodePtr m_pInstance;   // only applies for singleton nodes
};

#if defined(AZ_PLATFORM_WINDOWS) && defined(AZ_MONOLITHIC_BUILD)
#define CRY_EXPORT_STATIC_LINK_VARIABLE(Var)                \
    extern "C" { size_t lib_func_##Var() { return reinterpret_cast<uintptr_t>(&Var); } \
    }                                                       \
    __pragma(message("#pragma comment(linker,\"/include:_lib_func_"#Var "\")"))
#else
#define CRY_EXPORT_STATIC_LINK_VARIABLE(Var)
#endif

//////////////////////////////////////////////////////////////////////////
// Use this define to register a new flow node class. Handles
//  both instanced and singleton nodes.
// Ex. REGISTER_FLOW_NODE( "Delay",CFlowDelayNode )
//////////////////////////////////////////////////////////////////////////
#define REGISTER_FLOW_NODE(FlowNodeClassName, FlowNodeClass)                        \
    CAIAutoRegFlowNode<FlowNodeClass> g_AutoReg##FlowNodeClass (FlowNodeClassName); \
    CRY_EXPORT_STATIC_LINK_VARIABLE(g_AutoReg##FlowNodeClass)

#define REGISTER_FLOW_NODE_EX(FlowNodeClassName, FlowNodeClass, RegName)      \
    CAIAutoRegFlowNode<FlowNodeClass> g_AutoReg##RegName (FlowNodeClassName); \
    CRY_EXPORT_STATIC_LINK_VARIABLE(g_AutoReg##RegName)

//////////////////////////////////////////////////////////////////////////
// Internal base class for flow nodes. Node classes shouldn't be
//  derived directly from this, but should derive from CFlowBaseNode<>
//////////////////////////////////////////////////////////////////////////
class CFlowBaseNodeInternal
    : public IFlowNode
{
    template <ENodeCloneType CLONE_TYPE>
    friend class CFlowBaseNode;

private:
    // private ctor/dtor to prevent classes directly derived from this;
    //  the exception is CFlowBaseNode (friended above)
    CFlowBaseNodeInternal() { m_refs = 0; };
    virtual ~CFlowBaseNodeInternal() {}

public:
    //////////////////////////////////////////////////////////////////////////
    // IFlowNode
    virtual void AddRef() { ++m_refs; };
    virtual void Release()
    {
        if (0 >= --m_refs)
        {
            delete this;
        }
    };

    virtual IFlowNodePtr Clone(SActivationInfo* pActInfo) = 0;
    virtual bool SerializeXML(SActivationInfo*, const XmlNodeRef&, bool) { return true; }
    virtual void Serialize(SActivationInfo*, TSerialize ser) {}
    virtual void PostSerialize(SActivationInfo*) {}
    virtual void ProcessEvent(EFlowEvent event, SActivationInfo* pActInfo) {};
    //////////////////////////////////////////////////////////////////////////

private:
    int m_refs;
};

//////////////////////////////////////////////////////////////////////////
// Base class that nodes can derive from.
//  eg1: Singleton node:
//  class CSingletonNode : public CFlowBaseNode<eNCT_Singleton>
//
//  eg2: Instanced node:
//  class CInstancedNode : public CFlowBaseNode<eNCT_Instanced>
//
//  Instanced nodes must override Clone (pure virtual in base), while
//  Singleton nodes cannot - will result in a compile error.
//////////////////////////////////////////////////////////////////////////

template <ENodeCloneType CLONE_TYPE>
class CFlowBaseNode
    : public CFlowBaseNodeInternal
{
public:
    CFlowBaseNode()
        : CFlowBaseNodeInternal() {}

    static const int myCloneType = CLONE_TYPE;
};

// specialization for singleton nodes: Clone() is
//  marked so derived classes cannot override it.
template <>
class CFlowBaseNode<eNCT_Singleton>
    : public CFlowBaseNodeInternal
{
public:
    CFlowBaseNode<eNCT_Singleton>() : CFlowBaseNodeInternal()   {
    }

    virtual IFlowNodePtr Clone(SActivationInfo* pActInfo) final { return this; }

    static const int myCloneType = eNCT_Singleton;
};

#endif // CRYINCLUDE_CRYAISYSTEM_FLOWNODES_AIFLOWBASENODE_H
