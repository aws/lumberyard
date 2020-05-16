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

#include <QObject>
#include <QVariantMap>
#include <QVariant>

#include <AzCore/EBus/EBus.h>

using PythonWorkerRequestId = int;

class PythonWorkerEvents : public AZ::EBusTraits
{
public:
    using Bus = AZ::EBus<PythonWorkerEvents>;

    // if any OnOutput returns true, it means the command was handled and the worker won't process any further
    virtual bool OnPythonWorkerOutput(PythonWorkerRequestId requestId, const QString& key, const QVariant& value) = 0;
};

class PythonWorkerRequests : public AZ::EBusTraits
{
public:
    using Bus = AZ::EBus<PythonWorkerRequests>;

    static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;

    virtual PythonWorkerRequestId AllocateRequestId() = 0;
    virtual void ExecuteAsync(PythonWorkerRequestId requestId, const char* command, const QVariantMap& args = QVariantMap{}) = 0;
    virtual bool IsStarted() = 0;
};
