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
#include <FlowSystem/Nodes/FlowBaseNode.h>

//////////////////////////////////////////////////////////////////////////
// String nodes.
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
class CFlowNode_CompareStrings
    : public CFlowBaseNode<eNCT_Singleton>
{
public:
    CFlowNode_CompareStrings(SActivationInfo* pActInfo) {};

    virtual void GetConfiguration(SFlowNodeConfig& config)
    {
        static const SInputPortConfig in_config[] = {
            InputPortConfig_Void("Compare", _HELP("Trigger to compare strings [A] and [B]")),
            InputPortConfig<string>("A", _HELP("String A to compare")),
            InputPortConfig<string>("B", _HELP("String B to compare")),
            InputPortConfig<bool>  ("IgnoreCase", true),
            {0}
        };
        static const SOutputPortConfig out_config[] = {
            OutputPortConfig<int>("Result", _HELP("Outputs -1 if A < B, 0 if A == B, 1 if A > B")),
            OutputPortConfig_Void("False", _HELP("Triggered if A != B")),
            OutputPortConfig_Void("True",  _HELP("Triggered if A == B")),
            {0}
        };
        config.sDescription = _HELP("Compare two strings");
        config.pInputPorts = in_config;
        config.pOutputPorts = out_config;
        config.SetCategory(EFLN_APPROVED);
    }
    virtual void ProcessEvent(EFlowEvent evt, SActivationInfo* pActInfo)
    {
        switch (evt)
        {
        case eFE_Activate:
            if (IsPortActive(pActInfo, 0))
            {
                const string& a = GetPortString(pActInfo, 1);
                const string& b = GetPortString(pActInfo, 2);
                const bool bIgnoreCase = GetPortBool(pActInfo, 3);
                const int result = bIgnoreCase ? _stricmp(a.c_str(), b.c_str()) : a.compare(b);
                ActivateOutput(pActInfo, 0, result);
                ActivateOutput(pActInfo, result != 0 ? 1 : 2, 0);
            }
            break;
        }
    };

    virtual void GetMemoryUsage(ICrySizer* s) const
    {
        s->Add(*this);
    }
};


//////////////////////////////////////////////////////////////////////////
class CFlowNode_SetString
    : public CFlowBaseNode<eNCT_Singleton>
{
public:
    CFlowNode_SetString(SActivationInfo* pActInfo) {};

    virtual void GetConfiguration(SFlowNodeConfig& config)
    {
        static const SInputPortConfig in_config[] = {
            InputPortConfig_Void("Set", _HELP("Send string [In] to [Out]")),
            InputPortConfig<string>("In", _HELP("String to be set on [Out].\nWill also be set in Initialize")),
            {0}
        };
        static const SOutputPortConfig out_config[] = {
            OutputPortConfig<string>("Out"),
            {0}
        };
        config.sDescription = _HELP("Set String Value");
        config.pInputPorts = in_config;
        config.pOutputPorts = out_config;
        config.SetCategory(EFLN_APPROVED);
    }
    virtual void ProcessEvent(EFlowEvent evt, SActivationInfo* pActInfo)
    {
        switch (evt)
        {
        case eFE_Activate:
        case eFE_Initialize:
            if (IsPortActive(pActInfo, 0))
            {
                ActivateOutput(pActInfo, 0, GetPortString(pActInfo, 1));
            }
            break;
        }
    };

    virtual void GetMemoryUsage(ICrySizer* s) const
    {
        s->Add(*this);
    }
};

class CFlowNode_MathSetString
    : public CFlowNode_SetString
{
public:
    CFlowNode_MathSetString(SActivationInfo* pActInfo)
        : CFlowNode_SetString(pActInfo) {}

    virtual void GetConfiguration(SFlowNodeConfig& config)
    {
        CFlowNode_SetString::GetConfiguration(config);
        config.sDescription = _HELP("Deprecated. Use String:SetString instead");
        config.SetCategory(EFLN_OBSOLETE);
    }

    virtual void GetMemoryUsage(ICrySizer* s) const
    {
        s->Add(*this);
    }
};

//////////////////////////////////////////////////////////////////////////
class CFlowNode_StringConcat
    : public CFlowBaseNode<eNCT_Singleton>
{
public:
    CFlowNode_StringConcat(SActivationInfo* pActInfo) {};

    virtual void GetConfiguration(SFlowNodeConfig& config)
    {
        static const SInputPortConfig in_config[] = {
            InputPortConfig_Void("Set", _HELP("Send string [String1+String2] to [Out]")),
            InputPortConfig<string>("String1", _HELP("First string to be concat with second string")),
            InputPortConfig<string>("String2", _HELP("Second string")),
            {0}
        };
        static const SOutputPortConfig out_config[] = {
            OutputPortConfig<string>("Out"),
            {0}
        };
        config.sDescription = _HELP("Concat two string values");
        config.pInputPorts = in_config;
        config.pOutputPorts = out_config;
        config.SetCategory(EFLN_APPROVED);
    }
    virtual void ProcessEvent(EFlowEvent evt, SActivationInfo* pActInfo)
    {
        switch (evt)
        {
        case eFE_Activate:
        case eFE_Initialize:
            if (IsPortActive(pActInfo, 0))
            {
                string str1 = GetPortString(pActInfo, 1);
                string str2 = GetPortString(pActInfo, 2);
                ActivateOutput(pActInfo, 0, str1 + str2);
            }
            break;
        }
    };

    virtual void GetMemoryUsage(ICrySizer* s) const
    {
        s->Add(*this);
    }
};

REGISTER_FLOW_NODE("Math:SetString",  CFlowNode_MathSetString);   // Legacy
REGISTER_FLOW_NODE("String:SetString", CFlowNode_SetString);
REGISTER_FLOW_NODE("String:Compare", CFlowNode_CompareStrings);
REGISTER_FLOW_NODE("String:Concat", CFlowNode_StringConcat);
