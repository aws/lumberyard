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
// Original file Copyright Crytek GMBH or its affiliates, used under license.

#ifndef CRYINCLUDE_CRYCOMMON_TELEMETRYPARAMS_H
#define CRYINCLUDE_CRYCOMMON_TELEMETRYPARAMS_H
#pragma once


#include "GenericParam.h"
#include "VariadicParams.h"
#include "TelemetryConfig.h"
#include "TelemetryFormat.h"

namespace Telemetry
{
    typedef CGenericParam<GenericParamUtils::CPtrStorage> TParam;

    typedef CVariadicParams<TParam> TVariadicParams;

    inline TParam FilterParam(const char* value, const char* prev)
    {
        return TParam(value, (value != prev) && (strcmp(value, prev) != 0));
    }

    template <typename TYPE>
    inline TParam FilterParam(const TYPE& value, const TYPE& prev)
    {
        return TParam(value, value != prev);
    }

    template <typename TYPE>
    inline TParam FilterParamDelta(const TYPE& value, const TYPE& prev, const TYPE& minDelta)
    {
        return TParam(value, Abs(value - prev) >= minDelta);
    }
}

#endif // CRYINCLUDE_CRYCOMMON_TELEMETRYPARAMS_H
