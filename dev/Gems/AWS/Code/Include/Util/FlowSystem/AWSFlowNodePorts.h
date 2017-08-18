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
#include <LmbrAWS/IAWSClientManager.h>
#include <LmbrAWS/ILmbrAWS.h>
#include <IFlowSystem.h>

namespace LmbrAWS
{
    // These port types provide access to clients configuted to access
    // specific AWS resources. Currently (see TODO below), the port value
    // is a string that is used as the default resource name (DynamoDB table,
    // SQS queue, etc.) and the client is configured using the default client
    // settings. Alturnativly the port value can be the name of a the resoruce
    // settings used to determine the actual resource name and the client
    // settings.
    //
    // TODO: make these custom port type instead of string. Need to extend custom
    // ports types with better UI support, so that resource names can be edited,
    // picked from a list of available configurations, etc. This will allow the
    // CreateXClient calls to happen only when the resource name changes.

    class ResourceClientInputPort
    {
    public:

        SInputPortConfig GetConfiguration(const char* name, const char* description) const
        {
            return InputPortConfig<string>(name, description);
        }

        bool IsActive(IFlowNode::SActivationInfo* pActInfo) const
        {
            return IsPortActive(pActInfo, m_index);
        }

    protected:

        ResourceClientInputPort(int index)
            : m_index{index}
        {
        }

        int m_index;

        static const string& GetPortString(IFlowNode::SActivationInfo* pActInfo, int index)
        {
            return ::GetPortString(pActInfo, index);
        }

        static const string& GetPortString(const std::shared_ptr<const FlowGraphContext>& context, int index)
        {
            return *context->GetFlowGraph()->GetInputValue(context->GetFlowNodeId(), index)->GetPtr<string>();
        }
    };

    namespace DynamoDB
    {
        class TableClientInputPort
            : public ResourceClientInputPort
        {
        public:

            TableClientInputPort(int index)
                : ResourceClientInputPort(index)
            {
            }

            TableClient GetClient(IFlowNode::SActivationInfo* pActInfo) const
            {
                return gEnv->pLmbrAWS->GetClientManager()->GetDynamoDBManager().CreateTableClient(GetPortString(pActInfo, m_index));
            }

            TableClient GetClient(const std::shared_ptr<const FlowGraphContext>& context) const
            {
                return gEnv->pLmbrAWS->GetClientManager()->GetDynamoDBManager().CreateTableClient(GetPortString(context, m_index));
            }
        };
    }

    namespace S3
    {
        class BucketClientInputPort
            : public ResourceClientInputPort
        {
        public:

            BucketClientInputPort(int index)
                : ResourceClientInputPort(index)
            {
            }

            BucketClient GetClient(IFlowNode::SActivationInfo* pActInfo) const
            {
                return gEnv->pLmbrAWS->GetClientManager()->GetS3Manager().CreateBucketClient(GetPortString(pActInfo, m_index));
            }

            BucketClient GetClient(const std::shared_ptr<const FlowGraphContext>& context) const
            {
                return gEnv->pLmbrAWS->GetClientManager()->GetS3Manager().CreateBucketClient(GetPortString(context, m_index));
            }
        };
    }

    namespace Lambda
    {
        class FunctionClientInputPort
            : public ResourceClientInputPort
        {
        public:

            FunctionClientInputPort(int index)
                : ResourceClientInputPort(index)
            {
            }

            FunctionClient GetClient(IFlowNode::SActivationInfo* pActInfo) const
            {
                return gEnv->pLmbrAWS->GetClientManager()->GetLambdaManager().CreateFunctionClient(GetPortString(pActInfo, m_index));
            }

            FunctionClient GetClient(const std::shared_ptr<const FlowGraphContext>& context) const
            {
                return gEnv->pLmbrAWS->GetClientManager()->GetLambdaManager().CreateFunctionClient(GetPortString(context, m_index));
            }
        };
    }

    namespace SNS
    {
        class TopicClientInputPort
            : public ResourceClientInputPort
        {
        public:

            TopicClientInputPort(int index)
                : ResourceClientInputPort(index)
            {
            }

            TopicClient GetClient(IFlowNode::SActivationInfo* pActInfo) const
            {
                return gEnv->pLmbrAWS->GetClientManager()->GetSNSManager().CreateTopicClient(GetPortString(pActInfo, m_index));
            }

            TopicClient GetClient(const std::shared_ptr<const FlowGraphContext>& context) const
            {
                return gEnv->pLmbrAWS->GetClientManager()->GetSNSManager().CreateTopicClient(GetPortString(context, m_index));
            }
        };
    }

    namespace SQS
    {
        class QueueClientInputPort
            : public ResourceClientInputPort
        {
        public:

            QueueClientInputPort(int index)
                : ResourceClientInputPort(index)
            {
            }

            QueueClient GetClient(IFlowNode::SActivationInfo* pActInfo) const
            {
                return gEnv->pLmbrAWS->GetClientManager()->GetSQSManager().CreateQueueClient(GetPortString(pActInfo, m_index));
            }

            QueueClient GetClient(const std::shared_ptr<const FlowGraphContext>& context) const
            {
                return gEnv->pLmbrAWS->GetClientManager()->GetSQSManager().CreateQueueClient(GetPortString(context, m_index));
            }
        };
    }
} // namespace LmbrAWS