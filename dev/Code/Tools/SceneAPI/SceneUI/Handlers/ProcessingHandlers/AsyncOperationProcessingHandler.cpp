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

#include <SceneAPI/SceneUI/Handlers/ProcessingHandlers/AsyncOperationProcessingHandler.h>
#include <AzCore/std/parallel/thread.h>
#include <AzCore/Component/TickBus.h>

namespace AZ
{
    namespace SceneAPI
    {
        namespace SceneUI
        {
            AsyncOperationProcessingHandler::AsyncOperationProcessingHandler(AZStd::function<void()> targetFunction, AZStd::function<void()> onComplete, QObject* parent)
                : ProcessingHandler(parent)
                , m_operationToRun(targetFunction)
                , m_onComplete(onComplete)
            {
            }

            void AsyncOperationProcessingHandler::BeginProcessing()
            {
                emit UserMessageUpdated("Waiting for background processes to complete...");
                m_thread.reset(
                    new AZStd::thread(
                        [this]()
                        {
                            m_operationToRun();
                            EBUS_QUEUE_FUNCTION(AZ::TickBus, AZStd::bind(&AsyncOperationProcessingHandler::OnBackgroundOperationComplete, this));
                        }
                    )
                );
            }

            void AsyncOperationProcessingHandler::OnBackgroundOperationComplete()
            {
                m_thread->detach();
                m_thread.reset(nullptr);

                emit UserMessageUpdated("Processing complete");
                emit AddInfo("Background processing complete");
                m_onComplete();
                emit ProcessingComplete();
            }
        }
    }
}

#include <Handlers/ProcessingHandlers/AsyncOperationProcessingHandler.moc>