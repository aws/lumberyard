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

// Description : Telemetry system interface.


#ifndef CRYINCLUDE_CRYCOMMON_ITELEMETRYSYSTEM_H
#define CRYINCLUDE_CRYCOMMON_ITELEMETRYSYSTEM_H
#pragma once


#include "GenericParam.h"
#include "TelemetryConfig.h"
#include "TelemetryParams.h"

#define TELEMETRY_NOP do {} while (0)

#if TELEMETRY_MIN_VERBOSITY != TELEMETRY_VERBOSITY_OFF
#define TELEMETRY_SET_GLOBAL_VERBOSITY(verbosity)                                       gEnv->pTelemetrySystem->SetGlobalVerbosity((verbosity))
#define TELEMETRY_SET_ASPECT_VERBOSITY(aspect, verbosity)                       gEnv->pTelemetrySystem->SetAspectVerbosity((aspect), (verbosity))
#else
#define TELEMETRY_SET_GLOBAL_VERBOSITY(verbosity)                                       TELEMETRY_NOP
#define TELEMETRY_SET_ASPECT_VERBOSITY(aspect, verbosity)                       TELEMETRY_NOP
#endif //TELEMETRY_MIN_VERBOSITY

#if TELEMETRY_MIN_VERBOSITY >= TELEMETRY_VERBOSITY_LOWEST
#define TELEMETRY_SHOULD_SEND_LOWEST(aspect)                                                gEnv->pTelemetrySystem->ShouldSend((aspect), TELEMETRY_VERBOSITY_LOWEST)
#define TELEMETRY_SEND_LOWEST(aspect, event, tableParams, params)       gEnv->pTelemetrySystem->Send((aspect), (event), (tableParams), (params), TELEMETRY_VERBOSITY_LOWEST)
#else
#define TELEMETRY_SHOULD_SEND_LOWEST(aspect, verbosity)                         false
#define TELEMETRY_SEND_LOWEST(aspect, event, tableParams, params)       TELEMETRY_NOP
#endif //TELEMETRY_MIN_VERBOSITY

#if TELEMETRY_MIN_VERBOSITY >= TELEMETRY_VERBOSITY_LOW
#define TELEMETRY_SHOULD_SEND_LOW(aspect)                                                       gEnv->pTelemetrySystem->ShouldSend((aspect), TELEMETRY_VERBOSITY_LOWEST)
#define TELEMETRY_SEND_LOW(aspect, event, tableParams, params)          gEnv->pTelemetrySystem->Send((aspect), (event), (tableParams), (params), TELEMETRY_VERBOSITY_LOW)
#else
#define TELEMETRY_SHOULD_SEND_LOW(aspect)                                                       false
#define TELEMETRY_SEND_LOW(aspect, event, tableParams, params)          TELEMETRY_NOP
#endif //TELEMETRY_MIN_VERBOSITY

#if TELEMETRY_MIN_VERBOSITY >= TELEMETRY_VERBOSITY_DEFAULT
#define TELEMETRY_SHOULD_SEND_DEFAULT(aspect)                                               gEnv->pTelemetrySystem->ShouldSend((aspect), TELEMETRY_VERBOSITY_DEFAULT)
#define TELEMETRY_SEND_DEFAULT(aspect, event, tableParams, params)  gEnv->pTelemetrySystem->Send((aspect), (event), (tableParams), (params), TELEMETRY_VERBOSITY_DEFAULT)
#else
#define TELEMETRY_SHOULD_SEND_DEFAULT(aspect)                                               false
#define TELEMETRY_SEND_DEFAULT(aspect, event, tableParams, params)  TELEMETRY_NOP
#endif //TELEMETRY_MIN_VERBOSITY

#if TELEMETRY_MIN_VERBOSITY >= TELEMETRY_VERBOSITY_HIGH
#define TELEMETRY_SHOULD_SEND_HIGH(aspect)                                                  gEnv->pTelemetrySystem->ShouldSend((aspect), TELEMETRY_VERBOSITY_HIGH)
#define TELEMETRY_SEND_HIGH(aspect, event, tableParams, params)         gEnv->pTelemetrySystem->Send((aspect), (event), (tableParams), (params), TELEMETRY_VERBOSITY_HIGH)
#else
#define TELEMETRY_SHOULD_SEND_HIGH(aspect)                                                  false
#define TELEMETRY_SEND_HIGH(aspect, event, tableParams, params)         TELEMETRY_NOP
#endif //TELEMETRY_MIN_VERBOSITY

#if TELEMETRY_MIN_VERBOSITY == TELEMETRY_VERBOSITY_HIGHEST
#define TELEMETRY_SHOULD_SEND_HIGHEST(aspect)                                               gEnv->pTelemetrySystem->ShouldSend((aspect), TELEMETRY_VERBOSITY_HIGHEST)
#define TELEMETRY_SEND_HIGHEST(aspect, event, tableParams, params)  gEnv->pTelemetrySystem->Send((aspect), (event), (tableParams), (params), TELEMETRY_VERBOSITY_HIGHEST)
#else
#define TELEMETRY_SHOULD_SEND_HIGHEST(aspect)                                               false
#define TELEMETRY_SEND_HIGHEST(aspect, event, tableParams, params)  TELEMETRY_NOP
#endif //TELEMETRY_MIN_VERBOSITY

namespace Telemetry
{
    struct IStream;

    struct ITelemetrySystem
    {
        // <interfuscator:shuffle>
        virtual ~ITelemetrySystem() {};

        virtual bool Init() = 0;

        virtual void Shutdown() = 0;

        virtual void Release() = 0;

        virtual void Update() = 0;

        virtual bool AttachStream(IStream* pStream) = 0;

        virtual void DetachStream(IStream* pStream) = 0;

        virtual void SetGlobalVerbosity(uint32 verbosity) = 0;

        virtual void SetAspectVerbosity(uint32 aspectId, uint32 verbosity) = 0;

        virtual bool ShouldSend(uint32 aspectId, uint32 verbosity) = 0;

        virtual void Send(uint32 aspectId, uint32 eventId, const char* pTableParams, const TVariadicParams& params, uint32 verbosity) = 0;

        virtual void Flush() = 0;
        // </interfuscator:shuffle>
    };

    //////////////////////////////////////////////////////////////////////////
    // interfaces for display of data
    //////////////////////////////////////////////////////////////////////////

    typedef CGenericParam<GenericParamUtils::CValueStorage> CAnyValue;

    // callback to game code to allow rendering of specific timelines
    typedef void (* TRenderCallback)(struct ITelemetryTimeline* pTimeline);

    // the repository stores all telemetry data
    struct ITelemetryRepository
    {
        // <interfuscator:shuffle>
        virtual ~ITelemetryRepository() {}

        virtual void RegisterTimelineRenderer(const char* timelineName, TRenderCallback pCallback) = 0;

        virtual uint64 GetFilterTimeStart() const = 0;
        virtual uint64 GetFilterTimeEnd() const = 0;
        // </interfuscator:shuffle>
    };

    // recorded data on a single event
    struct STelemetryEvent
    {
        typedef std::vector<CAnyValue> TStates;

        uint64 timeMs;
        Vec3 position;
        struct ITelemetryTimeline* pTimeline;
        TStates states;
        bool filterOk;

        STelemetryEvent()
            : timeMs(-1)
            , pTimeline(0)
            , filterOk(true)
        { }

        STelemetryEvent(int64 time, const Vec3& pos)
            : timeMs(time)
            , position(pos)
            , pTimeline(0)
            , filterOk(true)
        { }

        void swap(STelemetryEvent& other)
        {
            std::swap(timeMs, other.timeMs);
            std::swap(position, other.position);
            std::swap(pTimeline, other.pTimeline);
            std::swap(states, other.states);
            std::swap(filterOk, other.filterOk);
        }
    };

    typedef std::vector<STelemetryEvent*> TEvents;

    // Timelines provide the basic storage of events
    struct ITelemetryTimeline
    {
        // <interfuscator:shuffle>
        virtual ~ITelemetryTimeline() {}

        virtual const char* GetName() const = 0;
        virtual uint32 GetEventCount() const = 0;
        virtual const STelemetryEvent* const* GetEvents() const = 0;

        virtual ITelemetryRepository* const GetRepository() const = 0;
        // </interfuscator:shuffle>
    };
}

#endif // CRYINCLUDE_CRYCOMMON_ITELEMETRYSYSTEM_H
