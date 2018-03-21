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
#include <StaticData_precompiled.h>
#include <Nodes/FlowNode_GetStaticData.h>
#include "IGemManager.h"
#include <StaticData/StaticDataBus.h>
#include <StaticDataInterface.h>

namespace LmbrAWS
{
    namespace FlowNode_GetStaticDataInternal
    {
        static const char* CLASS_TAG = "AWS:StaticData:GetStaticData";
    }

    FlowNode_GetStaticData::FlowNode_GetStaticData(SActivationInfo* activationInfo)
        : BaseMaglevFlowNode< eNCT_Instanced >(activationInfo)
    {
        m_staticDataTypeString = GetDataEnumString();
        BusConnect();
    }

    Aws::String FlowNode_GetStaticData::GetDataEnumString() const
    {
        Aws::String returnString;

        CloudCanvas::StaticData::StaticDataTypeList returnList;
        EBUS_EVENT_RESULT(returnList, CloudCanvas::StaticData::StaticDataRequestBus, GetDataTypeList);

        // This is a temporary solution to allow this control to show what types of data are loaded
        // That functionality may remain but it will also respond to dynamic data type changes
        returnString = "enum_string:";
        if (returnList.size())
        {
            for (auto thisElement : returnList)
            {
                returnString += thisElement.c_str();
                returnString += "=";
                returnString += thisElement.c_str();
                returnString += ",";
            }
            // erase our trailing comma
            returnString.erase(returnString.length() - 1);
        }

        return returnString;
    }

    Aws::Vector<SInputPortConfig> FlowNode_GetStaticData::GetInputPorts() const
    {
        static Aws::Vector<SInputPortConfig> inputPortConfiguration =
        {
            InputPortConfig_Void("Get", "Retrieve a value from Static Data"),
            InputPortConfig<string, string>("StaticDataType", "", _HELP("Your static data type to retrieve"), "StaticDataType",
                _UICONFIG(m_staticDataTypeString.c_str())),
            InputPortConfig<string>("StaticDataId", "The identifier for this static data definition within the table."),
            InputPortConfig<string>("StaticDataField", "Field name for the data to retrieve."),
            InputPortConfig_Void("ActivateOnUpdate", "Fire the node again the next time an update of the data takes place"),
        };

        return inputPortConfiguration;
    }

    Aws::Vector<SOutputPortConfig> FlowNode_GetStaticData::GetOutputPorts() const
    {
        static const Aws::Vector<SOutputPortConfig> outputPorts = {
            OutputPortConfig<string>("StringOut", _HELP("Output of a string field")),
            OutputPortConfig<int>("NumberOut", _HELP("Output of a numeric field")),
            OutputPortConfig<bool>("BoolOut", _HELP("Output of a bool field")),
            OutputPortConfig<int>("FloatOut", _HELP("Output of a floating point number")),
        };
        return outputPorts;
    }

    void FlowNode_GetStaticData::ProcessEvent_Internal(EFlowEvent event, SActivationInfo* activationInfo)
    {
        switch (event)
        {
        case eFE_Initialize:
        {
            m_actInfo = *activationInfo;
        }
        break;
        case eFE_Activate:
        {
            if (IsPortActive(activationInfo, EIP_Get))
            {
                DoNodeActivation(activationInfo);
            }
            if (IsPortActive(activationInfo, EIP_ActivatOnUpdate))
            {
                m_activateNextUpdate = true;
                if (m_hasUpdate)
                {
                    activationInfo->pGraph->SetRegularlyUpdated(activationInfo->myID, true);
                }
            }
        }
        break;
        case eFE_Update:
        {
            if (m_hasUpdate && m_activateNextUpdate)
            {
                DoNodeActivation(activationInfo);
            }
        }
        break;
        default:
        {
        }
        }
    }

    void FlowNode_GetStaticData::DoNodeActivation(SActivationInfo* activationInfo)
    {
        m_activateNextUpdate = false;
        m_hasUpdate = false;

        activationInfo->pGraph->SetRegularlyUpdated(activationInfo->myID, false);

        const auto& dataType = Aws::String {
            GetPortString(activationInfo, EIP_StaticDataType).c_str()
        };
        const auto& dataId = Aws::String {
            GetPortString(activationInfo, EIP_StaticDataId).c_str()
        };
        const auto& fieldName = Aws::String {
            GetPortString(activationInfo, EIP_FieldName).c_str()
        };

        AZStd::string strValue;
        int numValue = 0;
        double doubleValue = 0.0;

        bool gotSuccess = false;

        bool wasSuccess = false;
        CloudCanvas::StaticData::ReturnStr strResult;
        EBUS_EVENT_RESULT(strResult, CloudCanvas::StaticData::StaticDataRequestBus,
            GetStrValue, dataType.c_str(), dataId.c_str(), fieldName.c_str(), wasSuccess);
        if (wasSuccess)
        {
            strValue = strResult;
            SFlowAddress addr(activationInfo->myID, EOP_StringOut, true);
            activationInfo->pGraph->ActivatePortCString(addr, strValue.c_str());
            SuccessNotify(activationInfo->pGraph, activationInfo->myID);
            m_lastValue = strValue;
            gotSuccess = true;
        }

        CloudCanvas::StaticData::ReturnInt intResult;
        EBUS_EVENT_RESULT(intResult, CloudCanvas::StaticData::StaticDataRequestBus,
            GetIntValue, dataType.c_str(), dataId.c_str(), fieldName.c_str(), wasSuccess);
        if (wasSuccess)
        {
            SFlowAddress addr(activationInfo->myID, EOP_NumberOut, true);
            activationInfo->pGraph->ActivatePort(addr, intResult);
            SuccessNotify(activationInfo->pGraph, activationInfo->myID);
            gotSuccess = true;
        }

        CloudCanvas::StaticData::ReturnDouble doubleResult;
        EBUS_EVENT_RESULT(doubleResult, CloudCanvas::StaticData::StaticDataRequestBus,
            GetDoubleValue, dataType.c_str(), dataId.c_str(), fieldName.c_str(), wasSuccess);
        if (wasSuccess)
        {
            SFlowAddress addr(activationInfo->myID, EOP_FloatOut, true);
            activationInfo->pGraph->ActivatePort(addr, static_cast<float>(doubleResult));
            SuccessNotify(activationInfo->pGraph, activationInfo->myID);
            gotSuccess = true;
        }

        if (!gotSuccess)
        {
            ErrorNotify(activationInfo->pGraph, activationInfo->myID, "Failed to retrieve static data");
        }
    }

    void FlowNode_GetStaticData::TypeReloaded(const AZStd::string& reloadType)
    {
        auto dataType = AZStd::string(GetPortString(&m_actInfo, EIP_StaticDataType));

        if (dataType != reloadType)
        {
            return;
        }
        auto dataId = AZStd::string(GetPortString(&m_actInfo, EIP_StaticDataId));
        auto fieldName = AZStd::string(GetPortString(&m_actInfo, EIP_FieldName));

        CloudCanvas::StaticData::ReturnStr strResult;
        bool wasSuccess = false;
        EBUS_EVENT_RESULT(strResult, CloudCanvas::StaticData::StaticDataRequestBus,
            GetStrValue, dataType.c_str(), dataId.c_str(), fieldName.c_str(), wasSuccess);
        if (wasSuccess)
        {
            if (m_lastValue != strResult)
            {
                m_lastValue = strResult;
                m_hasUpdate = true;

                if (m_activateNextUpdate)
                {
                    m_actInfo.pGraph->SetRegularlyUpdated(m_actInfo.myID, true);
                }
            }
        }
    }

    IFlowNodePtr FlowNode_GetStaticData::Clone(SActivationInfo* pActInfo)
    {
        return new FlowNode_GetStaticData(pActInfo);
    }
    REGISTER_CLASS_TAG_AND_FLOW_NODE(FlowNode_GetStaticDataInternal::CLASS_TAG, FlowNode_GetStaticData);
}