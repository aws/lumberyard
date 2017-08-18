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

namespace LmbrAWS
{
    //////////////////////////////////////////////////////////////////////////////////////
    ///
    /// Check the status of a given file from the dynamic content back end
    ///
    //////////////////////////////////////////////////////////////////////////////////////
    class FlowNode_RequestFileStatus : public BaseMaglevFlowNode< eNCT_Singleton >
    {

    public:
        FlowNode_RequestFileStatus(SActivationInfo* activationInfo);
        
        virtual ~FlowNode_RequestFileStatus() {}

        enum EInputs
        {
            EIP_RequestStatus = EIP_StartIndex,
            EIP_FileName,
        };

        enum EOutputs
        {
            EOP_Finished = EOP_StartIndex,
        };

        virtual const char* GetClassTag() const override;
    protected:
        const char* GetFlowNodeDescription() const override { return _HELP("Check the status of the given file"); }
        void ProcessEvent_Internal(EFlowEvent event, SActivationInfo* activationInfo) override;
        Aws::Vector<SInputPortConfig> GetInputPorts() const override;
        Aws::Vector<SOutputPortConfig> GetOutputPorts() const override;  

        void ProcessResult(std::shared_ptr<const Aws::Client::AsyncCallerContext> requestContext, std::shared_ptr<FlowGraphContext> fgContext);
    };
   

} // namespace LmbrAWS



