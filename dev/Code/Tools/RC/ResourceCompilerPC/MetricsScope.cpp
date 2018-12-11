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

#include "stdafx.h"
#include "IConfig.h"
#include "MetricsScope.h"
#include "StringHelpers.h"
#include "PathHelpers.h"
#include "IConvertor.h"
#include <AzCore/std/string/conversions.h>
#if !defined(LY_IGNORE_METRICS)
#include <LyMetricsProducer/LyMetricsAPI.h>
#endif // LY_IGNORE_METRICS

#if !defined(LY_IGNORE_METRICS)

MetricsScope::MetricsScope(bool enable, const char* applicationName, uint32_t interval)
    : m_initialized(enable ? LyMetrics_Initialize(applicationName, interval, true, nullptr, nullptr, LY_METRICS_BUILD_TIME) : false)
{
    if (!m_initialized && enable)
    {
        RCLogWarning("Failed to initialize LyMetrics.\n");
    }
}
#else
MetricsScope::MetricsScope(bool /*enable*/, const char* /*applicationName*/, uint32_t /*interval*/)
    : m_initialized(false)
{
}
#endif // LY_IGNORE_METRICS

MetricsScope::~MetricsScope()
{
#if !defined(LY_IGNORE_METRICS)
    if (m_initialized)
    {
        LyMetrics_Shutdown();
    }
#endif
}

bool MetricsScope::IsInitialized() const
{
    return m_initialized;
}

bool MetricsScope::HasDccName(const IConfig* config)
{
    return config->HasKey("dcc");
}

void MetricsScope::RecordDccName(const IConvertor* convertor, const IConfig* config, const char* compilerName, const char* sourceFile)
{
#if !defined(LY_IGNORE_METRICS)
    if (m_initialized && HasDccName(config))
    {
        string dccName = config->GetAsString("dcc", "", "");
        if (StringHelpers::ContainsIgnoreCase(dccName, "Max"))
        {
            dccName = "3DS_Max";
        }
        else if (StringHelpers::ContainsIgnoreCase(dccName, "Maya"))
        {
            dccName = "Maya";
        }
        else
        {
            dccName = "Unknown";
        }
        int dccVersion = config->GetAsInt("dccv", -1, -1);

        bool foundValidExtension = false;;
        string inputType = PathHelpers::FindExtension(sourceFile);
        int extensionIndex = 0;
        const char* extension = nullptr;
        do
        {
            extension = convertor->GetExt(extensionIndex);
            if (StringHelpers::CompareIgnoreCase(inputType, extension))
            {
                foundValidExtension = true;
                break;
            }
            extensionIndex++;
        } while (extension);
        if (!foundValidExtension)
        {
            inputType = "Unknown";
        }

        LyMetrics_SendEvent("DccExport",
        {
            { "SourceDcc", dccName.c_str() },
            { "SourceDccVersion", AZStd::to_string(dccVersion).c_str() },
            { "Compiler", compilerName },
            { "InputType", inputType.c_str() }
        });
    }
#else
    (void)convertor;
    (void)config;
    (void)compilerName;
    (void)sourceFile;
#endif // LY_IGNORE_METRICS
}