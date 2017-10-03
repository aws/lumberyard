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

#define MARSHALL_AWS_BACKGROUND_REQUEST(clientNamespace, client, operation, memberFunction, requestStruct, context)     \
  { using operation##RequestJob = AWS_API_REQUEST_JOB(clientNamespace, operation);                                      \
    operation##RequestJob::Config config(operation##RequestJob::GetDefaultConfig());                                    \
    auto requestJob = operation##RequestJob::Create(                                                                           \
        [this, context](operation##RequestJob* successJob )                                                                    \
    {                                                                                                                   \
        Aws::clientNamespace::Model::operation##Outcome outcome(std::move(successJob->result));                                \
        this->ApplyResult(successJob->request, outcome, context);                                                              \
    }, [this, context](operation##RequestJob* failedJob)  {                                                                   \
        Aws::clientNamespace::Model::operation##Outcome outcome(std::move(failedJob->error));                                 \
        this->ApplyResult(failedJob->request, outcome, context);                                                              \
    },                                                                                                                  \
    &config                                                                                                             \
    );                                                                                                                  \
    requestJob->request = std::move(requestStruct);                                                                            \
    requestJob->Start(); }