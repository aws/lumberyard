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
#include <DynamicContent/DynamicContentBus.h>

namespace LmbrAWS
{
    //////////////////////////////////////////////////////////////////////////////////////
    ///
    /// Request to download a pak containing a top level manifest from the dynamic content system
    ///
    //////////////////////////////////////////////////////////////////////////////////////
    class FlowNode_RequestManifest : public BaseMaglevFlowNode< eNCT_Singleton >
        , public CloudCanvas::DynamicContent::DynamicContentUpdateBus::Handler

    {

    public:
        FlowNode_RequestManifest(SActivationInfo* activationInfo);
        
        virtual ~FlowNode_RequestManifest() {}

        enum EInputs
        {
            EIP_RequestManifest = EIP_StartIndex,
            EIP_ManifestName,
        };

        enum EOutputs
        {
            EOP_Finished = EOP_StartIndex,
            EOP_PakContentReady,
            EOP_RequestsCompleted,
        };

        virtual const char* GetClassTag() const override;
    protected:
        const char* GetFlowNodeDescription() const override { return _HELP("Request for a new top level manifest from the dynamic content system"); }
        void ProcessEvent_Internal(EFlowEvent event, SActivationInfo* activationInfo) override;
        Aws::Vector<SInputPortConfig> GetInputPorts() const override;
        Aws::Vector<SOutputPortConfig> GetOutputPorts() const override;

        void NewContentReady(const AZStd::string& outputFile) override;
        void NewPakContentReady(const AZStd::string& pakFileName) override;
        void RequestsCompleted() override;

        SActivationInfo m_activationInfo;
    };
   

} // namespace LmbrAWS



