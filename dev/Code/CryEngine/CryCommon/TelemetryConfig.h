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

// Description : Telemetry configuration.

#ifndef CRYINCLUDE_CRYCOMMON_TELEMETRYCONFIG_H
#define CRYINCLUDE_CRYCOMMON_TELEMETRYCONFIG_H
#pragma once


#ifndef TELEMETRY_DEBUG
#ifdef _DEBUG
#define TELEMETRY_DEBUG 1
#else
#define TELEMETRY_DEBUG 0
#endif //_DEBUG
#endif //TELEMETRY_DEBUG

#define TELEMETRY_VERBOSITY_OFF         0
#define TELEMETRY_VERBOSITY_LOWEST  1
#define TELEMETRY_VERBOSITY_LOW         2
#define TELEMETRY_VERBOSITY_DEFAULT 3
#define TELEMETRY_VERBOSITY_HIGH        4
#define TELEMETRY_VERBOSITY_HIGHEST 5
#define TELEMETRY_VERBOSITY_NOT_SET 6

#ifndef TELEMETRY_MIN_VERBOSITY
#define TELEMETRY_MIN_VERBOSITY TELEMETRY_VERBOSITY_HIGHEST
#endif //TELEMETRY_MIN_VERBOSITY

#endif // CRYINCLUDE_CRYCOMMON_TELEMETRYCONFIG_H
