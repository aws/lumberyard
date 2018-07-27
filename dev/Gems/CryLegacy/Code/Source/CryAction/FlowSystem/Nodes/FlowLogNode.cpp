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

#include "CryLegacy_precompiled.h"
#include "FlowLogNode.h"
#include <FlowSystem/Nodes/FlowBaseNode.h>

CFlowLogNode::CFlowLogNode()
{
    m_refs = 0;
}

void CFlowLogNode::AddRef()
{
    ++m_refs;
}

void CFlowLogNode::Release()
{
    if (0 == --m_refs)
    {
        delete this;
    }
}

IFlowNodePtr CFlowLogNode::Clone(SActivationInfo* pActInfo)
{
    // we keep no state:
    return this;
}

void CFlowLogNode::GetConfiguration(SFlowNodeConfig& config)
{
    static const SInputPortConfig inconfig[] = {
        InputPortConfig_Void("input"),
        InputPortConfig<string>("message", "no message set"),
        {0}
    };

    config.pInputPorts = inconfig;
    config.pOutputPorts = NULL;
    config.SetCategory(EFLN_DEBUG);
}

void CFlowLogNode::ProcessEvent(EFlowEvent event, SActivationInfo* pActInfo)
{
    if (event == eFE_Activate && pActInfo->pInputPorts[0].IsUserFlagSet())
    {
        string data;
        pActInfo->pInputPorts[1].GetValueWithConversion(data);
        CryLogAlways("[flow-log] %s", data.c_str());
    }
}

bool CFlowLogNode::SerializeXML(SActivationInfo*, const XmlNodeRef& root, bool reading)
{
    return true;
}

void CFlowLogNode::Serialize(SActivationInfo*, TSerialize ser)
{
}

//////////////////////////////////////////////////////////////////////////
class CFlowNode_CSVDumper
    : public CFlowBaseNode<eNCT_Instanced>
{
public:
    CFlowNode_CSVDumper(SActivationInfo* pActInfo)
        : m_file(0) {};
    virtual ~CFlowNode_CSVDumper()
    {
        if (m_file)
        {
            fclose(m_file);
        }
        m_file = 0;
    };
    virtual IFlowNodePtr Clone(SActivationInfo* pActInfo) { return new CFlowNode_CSVDumper(pActInfo); }
    void Serialize(SActivationInfo*, TSerialize ser)
    {
        if (ser.IsReading())
        {
            m_file = 0;
        }
    }

    virtual void GetMemoryUsage(ICrySizer* s) const
    {
        s->Add(*this);
    }

    virtual void GetConfiguration(SFlowNodeConfig& config)
    {
        static const SInputPortConfig in_config[] = {
            InputPortConfig<string>("filename"),
            InputPortConfig<string>("name"),
            InputPortConfig_AnyType("value0"),
            InputPortConfig<string>("name"),
            InputPortConfig_AnyType("value1"),
            InputPortConfig<string>("name"),
            InputPortConfig_AnyType("value2"),
            InputPortConfig<string>("name"),
            InputPortConfig_AnyType("value3"),
            InputPortConfig<string>("name"),
            InputPortConfig_AnyType("value4"),
            InputPortConfig<string>("name"),
            InputPortConfig_AnyType("value5"),
            InputPortConfig<string>("name"),
            InputPortConfig_AnyType("value6"),
            InputPortConfig<string>("name"),
            InputPortConfig_AnyType("value7"),
            InputPortConfig<string>("name"),
            InputPortConfig_AnyType("value8"),
            InputPortConfig<string>("name"),
            InputPortConfig_AnyType("value9"),
            {0}
        };
        static const SOutputPortConfig out_config[] = {
            {0}
        };
        config.pInputPorts = in_config;
        config.pOutputPorts = out_config;
        config.SetCategory(EFLN_ADVANCED);
    }
    virtual void ProcessEvent(EFlowEvent event, SActivationInfo* pActInfo)
    {
        switch (event)
        {
        case eFE_Activate:
        {
            if (!m_file)
            {
                const string& filename = GetPortString(pActInfo, 0);
                if (filename.empty())
                {
                    return;
                }

                azfopen(&m_file, filename.c_str(), "w+");
                if (!m_file)
                {
                    return;
                }

                string name;
                name.reserve(256);

                for (int i = 0; i < 10; i++)
                {
                    if (GetPortString(pActInfo, 1 + i * 2).length())
                    {
                        name += GetPortString(pActInfo, 1 + i * 2) + ",";
                    }
                }

                fputs(name.c_str(), m_file);
                fputs("\n", m_file);
            }

            string value;
            value.reserve(256);

            for (int i = 0; i < 10; i++)
            {
                if (GetPortString(pActInfo, 1 + i * 2).length())
                {
                    value += GetPortString(pActInfo, 2 + i * 2) + ",";
                }
            }

            fputs(value.c_str(), m_file);
            fputs("\n", m_file);
            fflush(m_file);
        }
        break;
        }
        ;
    };

private:
    FILE* m_file;
};

REGISTER_FLOW_NODE("Debug:CSVDumper", CFlowNode_CSVDumper);
