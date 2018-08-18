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

#include "stdafx.h"
#include "PipeMessageParser.h"
#include "TimelinePreprocessor.h"

//////////////////////////////////////////////////////////////////////////

CPipeMessageParser::CPipeMessageParser(CTelemetryRepository& repo)
    : m_depth(0)
    , m_currentPath(false)
    , m_repository(repo)
{ }

//////////////////////////////////////////////////////////////////////////

void CPipeMessageParser::OnMessage(const string& message)
{
    int opcode = 0;
    const char* msg = message.c_str();
    sscanf_s(msg, "{%d}", &opcode);

    int lpos = 0;
    while (msg[lpos] != '}' && msg[lpos] != '\0')
    {
        ++lpos;
    }

    CRY_ASSERT(msg[lpos] != '\0');

    switch (opcode)
    {
    case eOC_Begin:
        OnBegin();
        break;
    case eOC_End:
        OnEnd();
        break;
    case eOC_Enter:
        OnEnter(&msg[lpos + 1]);
        break;
    case eOC_Leave:
        OnLeave();
        break;
    case eOC_Timeline:
        OnTimeline(&msg[lpos + 1]);
        break;
    case eOC_Event:
        OnEvent(&msg[lpos + 1]);
        break;
    }
}

//////////////////////////////////////////////////////////////////////////

void CPipeMessageParser::ClearDoneBuffer()
{
    for (size_t i = 0; i != m_doneBuffer.size(); ++i)
    {
        delete m_doneBuffer[i];
    }
    m_doneBuffer.clear();
}

//////////////////////////////////////////////////////////////////////////

void CPipeMessageParser::DeliverMessages()
{
    CryAutoLock<CryMutex> l(m_lock);

    if (m_doneBuffer.size())
    {
        GetIEditor()->BeginUndo();

        m_repository.ClearData(true);

        for (size_t i = 0, size = m_doneBuffer.size(); i != size; ++i)
        {
            CTelemetryTimeline* tl = m_repository.AddTimeline(*m_doneBuffer[i]);
            if (IsPathTimeline(tl->getName()))
            {
                m_repository.CreatePathObject(tl);
            }
        }

        GetIEditor()->AcceptUndo("Telemetry update");

        ClearDoneBuffer();
    }
}

//////////////////////////////////////////////////////////////////////////

void CPipeMessageParser::OnBegin()
{
    CryAutoLock<CryMutex> l(m_lock);

    CRY_ASSERT(m_currentBuffer.empty());

    m_depth = 0;
    m_endProcessed = 0;
    ClearDoneBuffer();
}

//////////////////////////////////////////////////////////////////////////

void CPipeMessageParser::OnEnd()
{
    CryAutoLock<CryMutex> l(m_lock);

    if (m_endProcessed != m_currentBuffer.size())
    {
        ProcessTimelines(m_endProcessed);
        m_endProcessed = m_currentBuffer.size();
    }

    CRY_ASSERT(m_doneBuffer.empty());
    std::swap(m_currentBuffer, m_doneBuffer);
}

//////////////////////////////////////////////////////////////////////////

void CPipeMessageParser::OnEnter(const char* prms)
{
    ++m_depth;

    stack_string params = prms;
    size_t i = params.find(']');

    stack_string name = params.substr(1, i - 1);
    m_currentOwner.reset(new STelemetryNode(name));

    while (i != stack_string::npos && (++i < params.length()))
    {
        size_t epos = params.find('=', i);
        CRY_ASSERT(epos != stack_string::npos);

        stack_string key = params.substr(i, epos - i);

        i = params.find(';', i);
        CRY_ASSERT(epos != stack_string::npos);

        stack_string val = params.substr(epos + 1, i - epos - 1);

        m_currentOwner->addState(key.c_str(), val.c_str());
    }

    if (m_endProcessed != m_currentBuffer.size())
    {
        ProcessTimelines(m_endProcessed);
        m_endProcessed = m_currentBuffer.size();
    }
}

//////////////////////////////////////////////////////////////////////////

void CPipeMessageParser::OnLeave()
{
    m_currentOwner.reset();
    --m_depth;
}

//////////////////////////////////////////////////////////////////////////

void CPipeMessageParser::OnTimeline(const char* name)
{
    m_currentPath = IsPathTimeline(name);
    m_currentBuffer.push_back(new CTelemetryTimeline(&m_repository, name, m_currentOwner));
}

//////////////////////////////////////////////////////////////////////////

void CPipeMessageParser::OnEvent(const char* params)
{
    STelemetryEvent newEvent;
    sscanf_s(params, "[%lld]", &newEvent.timeMs);

    float fval;
    stack_string prms(params);
    size_t i = prms.find(']');

    while (i != stack_string::npos && (++i < prms.length()))
    {
        size_t epos = prms.find('=', i);
        CRY_ASSERT(epos != stack_string::npos);

        stack_string key = prms.substr(i, epos - i);

        i = prms.find(';', i);
        CRY_ASSERT(epos != stack_string::npos);

        stack_string val = prms.substr(epos + 1, i - epos - 1);

        if (m_currentPath && sscanf_s(val.c_str(), "%f", &fval) != 0)
        {
            if (key == "x")
            {
                newEvent.position.x = fval;
            }
            else if (key == "y")
            {
                newEvent.position.y = fval;
            }
            else if (key == "z")
            {
                newEvent.position.z = fval;
            }
        }
        else
        {
            newEvent.states.insert(std::make_pair<string, string>(key.c_str(), val.c_str()));
        }
    }

    m_currentBuffer.back()->AppendEvent(newEvent);
}

//////////////////////////////////////////////////////////////////////////

static bool TimelineEmpty(CTelemetryTimeline* tl)
{
    if (tl->getEventCount() == 0)
    {
        delete tl;
        return true;
    }
    return false;
}

//////////////////////////////////////////////////////////////////////////

void CPipeMessageParser::ProcessTimelines(size_t start)
{
    // Remove empty time lines
    TTLBuffer::iterator empty = std::remove_if(m_currentBuffer.begin() + start, m_currentBuffer.end(), TimelineEmpty);
    m_currentBuffer.erase(empty, m_currentBuffer.end());

    size_t pos = FindPositionTimeline(start);

    if (pos == m_currentBuffer.size())
    {
        for (size_t i = start; i != m_currentBuffer.size(); ++i)
        {
            delete m_currentBuffer[i];
        }
        m_currentBuffer.erase(m_currentBuffer.begin() + start, m_currentBuffer.end());
        return;
    }

    CTimelinePreprocessor preproc;
    preproc.DeduceEventPositions(&m_currentBuffer[start], m_currentBuffer.size() - start, pos - start);
}

//////////////////////////////////////////////////////////////////////////

size_t CPipeMessageParser::FindPositionTimeline(size_t startFrom)
{
    size_t pos;
    for (pos = startFrom; pos != m_currentBuffer.size(); ++pos)
    {
        if (IsPathTimeline(m_currentBuffer[pos]->getName()))
        {
            break;
        }
    }
    return pos;
}

//////////////////////////////////////////////////////////////////////////

bool CPipeMessageParser::IsPathTimeline(const char* name)
{
    return CONST_TEMP_STRING(name) == "position";
}

//////////////////////////////////////////////////////////////////////////
