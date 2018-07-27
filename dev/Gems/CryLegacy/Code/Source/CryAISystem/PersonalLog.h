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

#ifndef CRYINCLUDE_CRYAISYSTEM_PERSONALLOG_H
#define CRYINCLUDE_CRYAISYSTEM_PERSONALLOG_H
#pragma once

#include <queue>

#if !defined(_RELEASE) && defined(WIN32)
#  define AI_COMPILE_WITH_PERSONAL_LOG
#endif

// An actor keeps a personal log where messages are stored.
// This text can be rendered to the screen and will be recorded
// in the AI Recorded for later inspection.
class PersonalLog
{
public:
    typedef std::deque<string> Messages;

    void AddMessage(const EntityId entityId, const char* message);
    const Messages& GetMessages() const { return m_messages; }
    void Clear() { m_messages.clear(); }

private:
    Messages m_messages;
};

#endif // CRYINCLUDE_CRYAISYSTEM_PERSONALLOG_H
