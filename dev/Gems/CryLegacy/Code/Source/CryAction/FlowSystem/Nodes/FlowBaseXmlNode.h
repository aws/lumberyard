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

// Description : Flowgraph nodes to read/write Xml files

#ifndef CRYINCLUDE_CRYACTION_FLOWSYSTEM_NODES_FLOWBASEXMLNODE_H
#define CRYINCLUDE_CRYACTION_FLOWSYSTEM_NODES_FLOWBASEXMLNODE_H
#pragma once

#include <FlowSystem/Nodes/FlowBaseNode.h>


#define ADD_BASE_INPUTS() \
    InputPortConfig_Void("Execute", _HELP("Execute Xml instruction"))

#define ADD_BASE_OUTPUTS()                                                                          \
    OutputPortConfig<bool>("Success", _HELP("Called if Xml instruction is executed successfully")), \
    OutputPortConfig<bool>("Fail", _HELP("Called if Xml instruction fails")),                       \
    OutputPortConfig<bool>("Done", _HELP("Called when Xml instruction is carried out"))

////////////////////////////////////////////////////
class CFlowXmlNode_Base
    : public CFlowBaseNode<eNCT_Instanced>
{
public:
    ////////////////////////////////////////////////////
    CFlowXmlNode_Base(SActivationInfo* pActInfo);
    virtual ~CFlowXmlNode_Base(void);
    virtual void ProcessEvent(EFlowEvent event, SActivationInfo* pActInfo);

protected:
    enum EInputs
    {
        EIP_Execute,
        EIP_CustomStart,
    };

    enum EOutputs
    {
        EOP_Success,
        EOP_Fail,
        EOP_Done,
        EOP_CustomStart,
    };

    //! Overload to handle Xml execution
    virtual bool Execute(SActivationInfo* pActInfo) = 0;

private:
    SActivationInfo m_actInfo;
    bool m_initialized;
};

////////////////////////////////////////////////////
////////////////////////////////////////////////////

////////////////////////////////////////////////////
struct SXmlDocument
{
    XmlNodeRef root;
    XmlNodeRef active;
    size_t refCount;
};

class CGraphDocManager
{
private:
    CGraphDocManager();
    static CGraphDocManager* m_instance;
public:
    virtual ~CGraphDocManager();
    static void Create();
    static CGraphDocManager* Get();

    void MakeXmlDocument(IFlowGraph* pGraph);
    void DeleteXmlDocument(IFlowGraph* pGraph);
    bool GetXmlDocument(IFlowGraph* pGraph, SXmlDocument** document);

private:
    typedef std::map<IFlowGraph*, SXmlDocument> GraphDocMap;
    GraphDocMap m_GraphDocMap;
};
extern CGraphDocManager* GDM;

#endif // CRYINCLUDE_CRYACTION_FLOWSYSTEM_NODES_FLOWBASEXMLNODE_H
