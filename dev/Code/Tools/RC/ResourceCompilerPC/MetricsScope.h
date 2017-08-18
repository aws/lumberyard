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

#include <stdint.h>

class IConfig;
struct IConvertor;
class MetricsScope
{
public:
    MetricsScope(bool enable, const char* applicationName, uint32_t interval);
    ~MetricsScope();

    bool IsInitialized() const;

    static bool HasDccName(const IConfig* config);
    void RecordDccName(const IConvertor* convertor, const IConfig* config, const char* compilerName, const char* sourceFile);

private:
    bool m_initialized;
};