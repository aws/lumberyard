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

#include <ISystem.h>
#include <LmbrAWS/ILmbrAWS.h>

#include <aws/core/utils/memory/AWSMemory.h>
#include <aws/core/client/AsyncCallerContext.h>

#include <functional>

namespace LmbrAWS
{
    // Because the compilers we use do not all yet support properly initializing move-only types inside a lambda (which we need for the outcome),
    // we make a templated wrapper class rather than just creating a lambda on the fly
    template< typename R, typename O, typename TClient >
    class AWSAsyncFlowgraphResult
        : public IAWSMainThreadRunnable
    {
    public:

        using ApplyResultFunctionType = std::function<void(const R&, const O&, const std::shared_ptr<const Aws::Client::AsyncCallerContext>&)>;

        AWSAsyncFlowgraphResult(ApplyResultFunctionType&& applicator, const R& request, O&& outcome, const std::shared_ptr<const Aws::Client::AsyncCallerContext>& context, const std::shared_ptr<TClient>& unmanagedClient)
            : m_applicator(std::move(applicator))
            , m_request(request)
            , m_outcome(std::move(outcome))
            , m_context(context)
            , m_unmanagedClient(unmanagedClient)
        {}

        ~AWSAsyncFlowgraphResult() {}

        virtual void RunOnMainThread() override
        {
            m_applicator(m_request, m_outcome, m_context);
            delete this;
        }

    private:

        ApplyResultFunctionType m_applicator;
        R m_request;
        O m_outcome;
        std::shared_ptr<const Aws::Client::AsyncCallerContext> m_context;
        std::shared_ptr<TClient> m_unmanagedClient;
    };


    template< typename FN, typename R, typename O, typename TClient >
    void PostAwsMainThreadResult(FN* flowNode,
        void (FN::* memberFunction)(const R&, const O&, const std::shared_ptr<const Aws::Client::AsyncCallerContext>&),
        const R& request,
        const O& outcome,
        const std::shared_ptr<const Aws::Client::AsyncCallerContext>& context,
        const std::shared_ptr<TClient>& unmanagedClient)
    {
        if (gEnv->pLmbrAWS)
        {
            auto mainThreadTask = new AWSAsyncFlowgraphResult<R, O, TClient>(std::bind(memberFunction, flowNode, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3),
                    request,
                    std::move(const_cast<O&>(outcome)),
                    context,
                    unmanagedClient);

            gEnv->pLmbrAWS->PostToMainThread(mainThreadTask);
        }
    }

    /*
     ToDo: rewrite

     Macro that replaces some boilerplate code required in each FlowNode that makes an async call.
     You don't have to use it, it's only provided for convenience. Since flow nodes need to run in UI thread
     whenever we process the callback that will then likely make a flownode update, that update should be done
     in the main thread. This macro marshalls your callback, back into the main ui thread.

    Params:
      clientNamespace   - the base namespace of the service that we're calling
      client            - Aws::ManagedClient object that we'll be invoking an operation on
      operation         - plain text prefix for the async operation that will be performed
      memberFunction    - qualified name of the member function that will handle the async result back on the main thread
      requestName       - Your request object l-value name.
      context           - Aws::Client::AsyncCallerContext for the request

    IMPORTANT: a copy of the shared_ptr for the unmanaged client is passed through the closure
    for the lambda expression. Should the client configuration change while the async call is
    in progress, this prevents the client from being deleted prematurely.

    */
} // namespace LmbrAWS

#define MARSHALL_AWS_BACKGROUND_REQUEST(clientNamespace, client, operation, memberFunction, requestName, context)       \
    { auto unmanagedClient = client.GetUnmanagedClient();                                                               \
      auto lambda = [this, requestName, context, unmanagedClient](                                                      \
              const decltype(unmanagedClient)::element_type* sender,                                                    \
              const Aws::clientNamespace::Model::operation##Request& request,                                           \
              const Aws::clientNamespace::Model::operation##Outcome& outcome,                                           \
              const std::shared_ptr<const Aws::Client::AsyncCallerContext>& baseContext)                                \
          { LmbrAWS::PostAwsMainThreadResult(this, &memberFunction, requestName, outcome, context, unmanagedClient); }; \
      client->operation##Async(requestName, lambda, context); }
