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

#ifndef CRYINCLUDE_CRYACTION_FLOWSYSTEM_NODES_FLOWSCRIPTEDNODE_H
#define CRYINCLUDE_CRYACTION_FLOWSYSTEM_NODES_FLOWSCRIPTEDNODE_H
#pragma once

#include "IFlowSystem.h"

class CFlowScriptedNodeFactory;
TYPEDEF_AUTOPTR(CFlowScriptedNodeFactory);
typedef CFlowScriptedNodeFactory_AutoPtr CFlowScriptedNodeFactoryPtr;
class CFlowSimpleScriptedNodeFactory;
TYPEDEF_AUTOPTR(CFlowSimpleScriptedNodeFactory);
typedef CFlowSimpleScriptedNodeFactory_AutoPtr CFlowSimpleScriptedNodeFactoryPtr;

class CFlowScriptedNode
    : public IFlowNode
{
public:
    CFlowScriptedNode(const SActivationInfo*, CFlowScriptedNodeFactoryPtr, SmartScriptTable);
    ~CFlowScriptedNode();

    // IFlowNode
    virtual void AddRef();
    virtual void Release();
    virtual IFlowNodePtr Clone(SActivationInfo* pActInfo);
    virtual void GetConfiguration(SFlowNodeConfig&);
    virtual void ProcessEvent(EFlowEvent event, SActivationInfo*);
    virtual bool SerializeXML(SActivationInfo*, const XmlNodeRef& root, bool reading);
    virtual void Serialize(SActivationInfo*, TSerialize ser);
    virtual void PostSerialize(SActivationInfo*){}
    virtual void GetMemoryUsage(ICrySizer* s) const;
    // ~IFlowNode

    int ActivatePort(IFunctionHandler* pH, size_t nOutput, const TFlowInputData& data);

private:
    int m_refs;
    SActivationInfo m_info;
    SmartScriptTable m_table;
    CFlowScriptedNodeFactoryPtr m_pFactory;
};

class CFlowScriptedNodeFactory
    : public IFlowNodeFactory
{
public:
    CFlowScriptedNodeFactory();
    ~CFlowScriptedNodeFactory();

    bool Init(const char* path, const char* name);

    virtual void AddRef();
    virtual void Release();
    virtual IFlowNodePtr Create(IFlowNode::SActivationInfo*);

    ILINE size_t NumInputs() const { return m_inputs.size() - 1; }
    ILINE const char* InputName(int n) const { return m_inputs[n].name; }
    void GetConfiguration(SFlowNodeConfig&);

    virtual void GetMemoryUsage(ICrySizer* s) const;

    void Reset() {}

private:
    int m_refs;
    SmartScriptTable m_table;

    std::set<string> m_stringTable;
    std::vector<SInputPortConfig> m_inputs;
    std::vector<SOutputPortConfig> m_outputs;
    uint32 m_category;

    const string& AddString(const char* str);

    static int ActivateFunction(IFunctionHandler* pH, void* pBuffer, int nSize);
};

class CFlowSimpleScriptedNode
    : public IFlowNode
{
public:
    CFlowSimpleScriptedNode(const SActivationInfo*, CFlowSimpleScriptedNodeFactoryPtr);
    ~CFlowSimpleScriptedNode();

    // IFlowNode
    virtual void AddRef();
    virtual void Release();
    virtual IFlowNodePtr Clone(SActivationInfo* pActInfo);
    virtual void GetConfiguration(SFlowNodeConfig&);
    virtual void ProcessEvent(EFlowEvent event, SActivationInfo*);
    virtual bool SerializeXML(SActivationInfo*, const XmlNodeRef& root, bool reading);
    virtual void Serialize(SActivationInfo*, TSerialize ser);
    virtual void PostSerialize(SActivationInfo*){};
    // ~IFlowNode

    int ActivatePort(IFunctionHandler* pH, size_t nOutput, const TFlowInputData& data);

    virtual void GetMemoryUsage(ICrySizer* s) const
    {
        s->Add(*this);
    }

private:
    int m_refs;
    CFlowSimpleScriptedNodeFactoryPtr m_pFactory;
};

class CFlowSimpleScriptedNodeFactory
    : public IFlowNodeFactory
{
public:
    CFlowSimpleScriptedNodeFactory();
    ~CFlowSimpleScriptedNodeFactory();

    bool Init(const char* path, const char* name);

    virtual void AddRef();
    virtual void Release();
    virtual IFlowNodePtr Create(IFlowNode::SActivationInfo*);

    ILINE size_t NumInputs() const { return m_inputs.size() - 1; }
    ILINE const char* InputName(int n) const { return m_inputs[n].name; }
    ILINE int GetActivateFlags() { return activateFlags; }
    void GetConfiguration(SFlowNodeConfig&);

    bool CallFunction(IFlowNode::SActivationInfo* pInputData);
    HSCRIPTFUNCTION GetFunction() { return m_func; }

    virtual void GetMemoryUsage(ICrySizer* s) const;

    void Reset() {}

private:
    int m_refs;
    HSCRIPTFUNCTION m_func;

    std::set<string> m_stringTable;
    std::vector<SInputPortConfig> m_inputs;
    std::vector<SOutputPortConfig> m_outputs;
    std::vector<ScriptAnyValue> m_outputValues;
    uint32 m_category;

    const string& AddString(const char* str);
    int activateFlags;  // one bit per input port; true means a value change will activate the node
};

#endif // CRYINCLUDE_CRYACTION_FLOWSYSTEM_NODES_FLOWSCRIPTEDNODE_H
