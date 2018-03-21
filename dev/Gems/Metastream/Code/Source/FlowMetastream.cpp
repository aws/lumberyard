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

#include "Metastream_precompiled.h"
#include <FlowSystem/Nodes/FlowBaseNode.h>
#include <Metastream/MetastreamBus.h>
#include <sstream>

class CFlowNode_MetastreamCacheData
    : public CFlowBaseNode < eNCT_Singleton >
{
    enum InputPorts
    {
        InputPorts_Activate,
        InputPorts_Table,
        InputPorts_Key,
        InputPorts_Value,
    };

    enum OutputPorts
    {
        OutputPorts_Out,
        OutputPorts_Error,
    };

public:

    explicit CFlowNode_MetastreamCacheData(IFlowNode::SActivationInfo*)
    {
    }

    ~CFlowNode_MetastreamCacheData() override
    {
    }

    void GetConfiguration(SFlowNodeConfig& config) override
    {
        static const SInputPortConfig inputs[] =
        {
            InputPortConfig_Void("Activate", _HELP("Exposes the value through Metastream")),
            InputPortConfig<string>("Table", _HELP("Table name inside which this key-value pair will be stored")),
            InputPortConfig<string>("Key", _HELP("Key name for this value")),
            InputPortConfig_AnyType("Value", _HELP("Value to expose")),
            { 0 }
        };
        static const SOutputPortConfig outputs[] =
        {
            OutputPortConfig_Void("Out", _HELP("Signalled when done")),
            OutputPortConfig<bool>("Error", _HELP("Signalled if an error has occurred")),
            { 0 }
        };

        config.pInputPorts = inputs;
        config.pOutputPorts = outputs;
        config.sDescription = _HELP("Exposes data through the Metastream Gem");
        config.SetCategory(EFLN_APPROVED);
    }

    void ProcessEvent(EFlowEvent event, SActivationInfo* pActInfo) override
    {
        switch (event)
        {
        case eFE_Initialize:
            break;

        case eFE_Activate:
            if (IsPortActive(pActInfo, InputPorts_Activate))
            {
                string table = GetPortString(pActInfo, InputPorts_Table);
                string key = GetPortString(pActInfo, InputPorts_Key);

                const TFlowInputData& rawValue = GetPortAny(pActInfo, InputPorts_Value);
                switch (rawValue.GetType())
                {
                case eFDT_Bool:
                {
                    bool boolValue;
                    rawValue.GetValueWithConversion<bool>(boolValue);
                    EBUS_EVENT(Metastream::MetastreamRequestBus, AddBoolToCache, table.c_str(), key.c_str(), boolValue);
                    break;
                }

                case eFDT_Double:
                case eFDT_Float:
                {
                    double doubleValue;
                    rawValue.GetValueWithConversion<double>(doubleValue);
                    EBUS_EVENT(Metastream::MetastreamRequestBus, AddDoubleToCache, table.c_str(), key.c_str(), doubleValue);
                    break;
                }

                case eFDT_EntityId:
                case eFDT_FlowEntityId:
                case eFDT_Pointer:
                {
                    // Convert to a string, then to a unsigned 64 bit value.
                    string value;
                    char *endChar;
                    rawValue.GetValueWithConversion<string>(value);
                    EBUS_EVENT(Metastream::MetastreamRequestBus, AddUnsigned64ToCache, table.c_str(), key.c_str(), static_cast<AZ::u64>(strtoull(value.c_str(), &endChar, 10)));
                    break;
                }
                
                case eFDT_Int:
                {
                    int value;
                    rawValue.GetValueWithConversion<int>(value);
                    EBUS_EVENT(Metastream::MetastreamRequestBus, AddSigned64ToCache, table.c_str(), key.c_str(), static_cast<AZ::s64>(value) );
                    break;
                }

                case eFDT_Vec3:
                {
                    // Construct an array representing the vector
                    Vec3 value;
                    rawValue.GetValueWithConversion<Vec3>(value);
                    EBUS_EVENT(Metastream::MetastreamRequestBus, AddVec3ToCache, table.c_str(), key.c_str(), value);
                    break;
                }
                case eFDT_String:
                default: 
                {
                    string value;
                    rawValue.GetValueWithConversion<string>(value);
                    EBUS_EVENT(Metastream::MetastreamRequestBus, AddStringToCache, table.c_str(), key.c_str(), value.c_str());
                    break;
                }
                }

                // Activate "Out" pin to signal success
                ActivateOutput(pActInfo, OutputPorts_Out, true);
            }

            break;
        }
    }

    void GetMemoryUsage(ICrySizer*) const override
    {
    }
};

class CFlowNode_MetastreamHTTPServer
    : public CFlowBaseNode < eNCT_Singleton >
{
    enum InputPorts
    {
        InputPorts_Start,
        InputPorts_Stop,
    };

    enum OutputPorts
    {
        OutputPorts_Out,
        OutputPorts_Error,
    };

public:
    explicit CFlowNode_MetastreamHTTPServer(IFlowNode::SActivationInfo*)
    {
    }

    ~CFlowNode_MetastreamHTTPServer() override
    {
    }

    void GetConfiguration(SFlowNodeConfig& config) override
    {
        static const SInputPortConfig inputs[] =
        {
            InputPortConfig_Void("Start", _HELP("Starts the Metastream HTTP server")),
            InputPortConfig_Void("Stop", _HELP("Starts the Metastream HTTP server")),
            { 0 }
        };
        static const SOutputPortConfig outputs[] =
        {
            OutputPortConfig_Void("Out", _HELP("Signalled when done")),
            OutputPortConfig<bool>("Error", _HELP("Signalled if an error occurred when trying to start the server")),
            { 0 }
        };

        config.pInputPorts = inputs;
        config.pOutputPorts = outputs;
        config.sDescription = _HELP("Controls for the Metastream HTTP server");
        config.SetCategory(EFLN_APPROVED);
    }

    void ProcessEvent(EFlowEvent event, SActivationInfo* pActInfo) override
    {
        switch (event)
        {
        case eFE_Initialize:
            break;

        case eFE_Activate:
            if (IsPortActive(pActInfo, InputPorts_Start))
            {
                // Start the Metastream HTTP server
                bool result;
                EBUS_EVENT_RESULT(result, Metastream::MetastreamRequestBus, StartHTTPServer);

                if (result)
                {
                    // Activate "Out" pin to signal success
                    ActivateOutput(pActInfo, OutputPorts_Out, true);
                }
                else
                {
                    // Activate "Error" pin to signal error
                    ActivateOutput(pActInfo, OutputPorts_Error, true);
                }
            }

            if (IsPortActive(pActInfo, InputPorts_Stop))
            {
                // Stop the Metastream HTTP server
                EBUS_EVENT(Metastream::MetastreamRequestBus, StopHTTPServer);

                // Activate "Out" pin to signal done
                ActivateOutput(pActInfo, OutputPorts_Out, true);
            }

            break;
        }
    }

    void GetMemoryUsage(ICrySizer*) const override
    {
    }
};

REGISTER_FLOW_NODE("Metastream:CacheData", CFlowNode_MetastreamCacheData);
REGISTER_FLOW_NODE("Metastream:HTTPServer", CFlowNode_MetastreamHTTPServer);