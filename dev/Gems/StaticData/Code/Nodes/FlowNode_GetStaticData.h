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
#pragma once

#include <Util/FlowSystem/BaseMaglevFlowNode.h>
#include <StaticDataInterface.h>

namespace LmbrAWS
{
    //////////////////////////////////////////////////////////////////////////////////////
    ///
    /// Pulls a data value from static data for a given data type, key name, attribute name combination
    ///
    //////////////////////////////////////////////////////////////////////////////////////
    class FlowNode_GetStaticData
        : public BaseMaglevFlowNode< eNCT_Instanced >
        , public CloudCanvas::StaticData::StaticDataUpdateBus::Handler
    {
    public:
        FlowNode_GetStaticData(SActivationInfo* activationInfo);

        virtual ~FlowNode_GetStaticData() {}

        enum EInputs
        {
            EIP_Get = EIP_StartIndex,
            EIP_StaticDataType,
            EIP_StaticDataId,
            EIP_FieldName,
            EIP_ActivatOnUpdate
        };

        enum EOutputs
        {
            EOP_StringOut = EOP_StartIndex,
            EOP_NumberOut,
            EOP_BoolOut,
            EOP_FloatOut
        };

        Aws::String GetDataEnumString() const;

        virtual const char* GetClassTag() const override;

        virtual IFlowNodePtr Clone(SActivationInfo* pActInfo) override;

        virtual void TypeReloaded(const AZStd::string& reloadType) override;
    protected:
        const char* GetFlowNodeDescription() const override { return _HELP("Retrieve a field from a static data definition."); }
        void ProcessEvent_Internal(EFlowEvent event, SActivationInfo* activationInfo) override;
        Aws::Vector<SInputPortConfig> GetInputPorts() const override;
        Aws::Vector<SOutputPortConfig> GetOutputPorts() const override;

        void DoNodeActivation(SActivationInfo* activationInfo);

        Aws::String m_staticDataTypeString;
        AZStd::string m_lastValue;
        bool m_hasUpdate {
            false
        };
        bool m_activateNextUpdate {
            false
        };

        SActivationInfo m_actInfo;
    };
} // namespace AWS



