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

// Description : Parses messages comming from the pipe


#ifndef CRYINCLUDE_TELEMETRYPLUGIN_PIPEMESSAGEPARSER_H
#define CRYINCLUDE_TELEMETRYPLUGIN_PIPEMESSAGEPARSER_H
#pragma once


#include "PipeClient.h"
#include "TelemetryRepository.h"
#include "TelemetryObjects.h"

class CPipeMessageParser
    : public IPipeClientListener
{
    enum EOpCode
    {
        eOC_Begin = 0,
        eOC_End = 1,
        eOC_Enter = 2,
        eOC_Leave = 3,
        eOC_Timeline = 4,
        eOC_Event = 5,

        eOC_Unknown = 255,
    };

public:

    CPipeMessageParser(CTelemetryRepository& repo);

    virtual void OnMessage(const string& message);

    void DeliverMessages();

private:
    void OnBegin();
    void OnEnd();
    void OnEnter(const char* name);
    void OnLeave();
    void OnTimeline(const char* name);
    void OnEvent(const char* params);

    void ProcessTimelines(size_t startFrom);
    size_t FindPositionTimeline(size_t startFrom);

    void ClearDoneBuffer();

    bool IsPathTimeline(const char* name);

private:
    typedef std::vector<CTelemetryTimeline*> TTLBuffer;
    TTLBuffer m_currentBuffer;
    TTLBuffer m_doneBuffer;

    CTelemetryRepository& m_repository;
    CryMutex m_lock;
    size_t m_depth;
    size_t m_endProcessed;
    TTelemNodePtr m_currentOwner;
    bool m_currentPath;
};

#endif // CRYINCLUDE_TELEMETRYPLUGIN_PIPEMESSAGEPARSER_H
