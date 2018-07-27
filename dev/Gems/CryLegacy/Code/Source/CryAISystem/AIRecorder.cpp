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

// Description : Detailed event-based AI recorder for visual debugging

// Notes       : Don't include the header in CAISystem - below it is redundant



#include "CryLegacy_precompiled.h"
#include "AIRecorder.h"
#include "PipeUser.h"
#include "StringUtils.h"
#include "IGameFramework.h"

#ifdef CRYAISYSTEM_DEBUG

// New AI recorder
#define AIRECORDER_FOLDER               "@LOG@/Recordings"
#define AIRECORDER_DEFAULT_FILE         "AIRECORD"
#define AIRECORDER_EDITOR_AUTO_APPEND   "EDITORAUTO"
#define AIRECORDER_VERSION 4

// We use some magic numbers to help when debugging the save structure
#define RECORDER_MAGIC  0xAA1234BB
#define UNIT_MAGIC          0xCC9876DD
#define EVENT_MAGIC         0xEE0921FF

// Anything larger than this is assumed to be a corrupt file
#define MAX_NAME_LENGTH 1024
#define MAX_UNITS               10024
#define MAX_EVENT_STRING_LENGTH 1024

AZ::IO::HandleType CAIRecorder::m_fileHandle = AZ::IO::InvalidHandle; // Hack!

struct RecorderFileHeader
{
    int version;
    int unitsNumber;
    int magic;

    RecorderFileHeader()
        : magic(RECORDER_MAGIC) {}
    bool check(void) { return RECORDER_MAGIC == magic && unitsNumber < MAX_UNITS; }
};

struct UnitHeader
{
    int nameLength;
    TAIRecorderUnitId ID;
    int magic;
    // And the variable length name follows this
    UnitHeader()
        : magic(UNIT_MAGIC) {}
    bool check(void) { return (UNIT_MAGIC == magic && nameLength < MAX_NAME_LENGTH); }
};

struct StreamedEventHeader
{
    TAIRecorderUnitId unitID;
    int streamID;
    int magic;

    StreamedEventHeader()
        : magic(EVENT_MAGIC) {}
    bool check(void) { return EVENT_MAGIC == magic; }
};




//----------------------------------------------------------------------------------------------
void    CRecorderUnit::RecordEvent(IAIRecordable::e_AIDbgEvent event, const IAIRecordable::RecorderEventData* pEventData)
{
    if (!m_pRecorder || !m_pRecorder->IsRunning())
    {
        return;
    }

    float time = (GetAISystem()->GetFrameStartTime() - m_startTime).GetSeconds();

    if (CAIRecorder::m_fileHandle != AZ::IO::InvalidHandle) //(Hack!)
    {
        bool bSuccess;
        StreamedEventHeader header;
        header.unitID = m_id;
        header.streamID = event;

        bSuccess = gEnv->pFileIO->Write(CAIRecorder::m_fileHandle, &header, sizeof(header));
        // We go through the stream to get the right virtual function

        bSuccess &= m_Streams[event]->WriteValue(pEventData, time);
        if (!bSuccess)
        {
            gEnv->pLog->LogError("[AI Recorder] Failed to write stream event");
        }
    }
    else
    {
        m_Streams[event]->AddValue(pEventData, time);
    }
}

//----------------------------------------------------------------------------------------------
bool CRecorderUnit::LoadEvent(IAIRecordable::e_AIDbgEvent stream)
{
    if (CAIRecorder::m_fileHandle != AZ::IO::InvalidHandle) //(Hack!)
    {
        return m_Streams[stream]->LoadValue(CAIRecorder::m_fileHandle);
    }
    return true;
}

//
//----------------------------------------------------------------------------------------------
CRecordable::CRecordable()
    : m_pMyRecord(NULL)
{
}

//
//----------------------------------------------------------------------------------------------
CRecorderUnit* CRecordable::GetOrCreateRecorderUnit(CAIObject* pAIObject, bool bForce)
{
    if (!m_pMyRecord && pAIObject && !gEnv->pSystem->IsSerializingFile())
    {
        m_pMyRecord = gAIEnv.GetAIRecorder()->AddUnit(pAIObject->GetSelfReference(), bForce);
    }

    return m_pMyRecord;
}


//
//----------------------------------------------------------------------------------------------
CRecorderUnit::CRecorderUnit(CAIRecorder* pRecorder, CTimeValue startTime, CWeakRef<CAIObject> refUnit, uint32 lifeIndex)
    : m_startTime(startTime)
    , m_pRecorder(pRecorder)
    , m_id(lifeIndex, refUnit.GetObjectID())
{
    CRY_ASSERT(refUnit.IsSet());
    CRY_ASSERT(m_pRecorder);

    // Get the name of the unit object
    if (refUnit.IsSet())
    {
        m_sName = refUnit.GetAIObject()->GetName();
    }
    else
    {
        m_sName.Format("UNKNOWN_UNIT_%u", (tAIObjectID)m_id);
    }

    m_Streams[IAIRecordable::E_RESET] = new CRecorderUnit::StreamStr("Reset");
    m_Streams[IAIRecordable::E_SIGNALRECIEVED] = new CRecorderUnit::StreamStr("Signal Received", true);
    m_Streams[IAIRecordable::E_SIGNALRECIEVEDAUX] = new CRecorderUnit::StreamStr("Auxilary Signal Received", true);
    m_Streams[IAIRecordable::E_SIGNALEXECUTING] = new CRecorderUnit::StreamStr("Signal Executing", true);
    m_Streams[IAIRecordable::E_GOALPIPESELECTED] = new CRecorderUnit::StreamStr("Goalpipe Selected", true);
    m_Streams[IAIRecordable::E_GOALPIPEINSERTED] = new CRecorderUnit::StreamStr("Goalpipe Inserted", true);
    m_Streams[IAIRecordable::E_GOALPIPERESETED] = new CRecorderUnit::StreamStr("Goalpipe Reset", true);
    m_Streams[IAIRecordable::E_BEHAVIORSELECTED] = new CRecorderUnit::StreamStr("Behavior Selected", true);
    m_Streams[IAIRecordable::E_BEHAVIORDESTRUCTOR] = new CRecorderUnit::StreamStr("Behavior Destructor", true);
    m_Streams[IAIRecordable::E_BEHAVIORCONSTRUCTOR] = new CRecorderUnit::StreamStr("Behavior Constructor", true);
    m_Streams[IAIRecordable::E_ATTENTIONTARGET] = new CRecorderUnit::StreamStr("AttTarget", true);
    m_Streams[IAIRecordable::E_ATTENTIONTARGETPOS] = new CRecorderUnit::StreamVec3("AttTarget Pos", true);
    m_Streams[IAIRecordable::E_REGISTERSTIMULUS] = new CRecorderUnit::StreamStr("Stimulus", true);
    m_Streams[IAIRecordable::E_HANDLERNEVENT] = new CRecorderUnit::StreamStr("Handler Event", true);
    m_Streams[IAIRecordable::E_ACTIONSUSPEND] = new CRecorderUnit::StreamStr("Action Suspend", true);
    m_Streams[IAIRecordable::E_ACTIONRESUME] = new CRecorderUnit::StreamStr("Action Resume", true);
    m_Streams[IAIRecordable::E_ACTIONEND] = new CRecorderUnit::StreamStr("Action End", true);
    m_Streams[IAIRecordable::E_EVENT] = new CRecorderUnit::StreamStr("Event", true);
    m_Streams[IAIRecordable::E_REFPOINTPOS] = new CRecorderUnit::StreamVec3("Ref Point", true);
    m_Streams[IAIRecordable::E_AGENTPOS] = new CRecorderUnit::StreamVec3("Agent Pos", true);
    // (Kevin) Direction should be able to be filtered? But needs a lower threshold. So instead of bool, maybe supply threshold value? (10/08/2009)
    m_Streams[IAIRecordable::E_AGENTDIR] = new CRecorderUnit::StreamVec3("Agent Dir", false);
    m_Streams[IAIRecordable::E_LUACOMMENT] = new CRecorderUnit::StreamStr("Lua Comment", true);
    m_Streams[IAIRecordable::E_PERSONALLOG] = new CRecorderUnit::StreamStr("Personal Log", true);
    m_Streams[IAIRecordable::E_HEALTH] = new CRecorderUnit::StreamFloat("Health", true);
    m_Streams[IAIRecordable::E_HIT_DAMAGE] = new CRecorderUnit::StreamStr("Hit Damage", false);
    m_Streams[IAIRecordable::E_DEATH] = new CRecorderUnit::StreamStr("Death", true);
    m_Streams[IAIRecordable::E_PRESSUREGRAPH] = new CRecorderUnit::StreamFloat("Pressure", true);
    m_Streams[IAIRecordable::E_BOOKMARK] = new CRecorderUnit::StreamFloat("Bookmark", false);
}

//
//----------------------------------------------------------------------------------------------
CRecorderUnit::~CRecorderUnit()
{
    for (TStreamMap::iterator strItr(m_Streams.begin()); strItr != m_Streams.end(); ++strItr)
    {
        delete strItr->second;
    }
    m_Streams.clear();
}

//
//----------------------------------------------------------------------------------------------
void CRecorderUnit::StreamBase::Seek(float whereTo)
{
    m_CurIdx = 0;

    if (whereTo > FLT_EPSILON)
    {
        const int iMaxIndex = max((int)m_Stream.size() - 1, 0);
        while (m_CurIdx < iMaxIndex && m_Stream[m_CurIdx]->m_StartTime <= whereTo)
        {
            ++m_CurIdx;
        }
        m_CurIdx = max(m_CurIdx - 1, 0);
    }
}

//
//----------------------------------------------------------------------------------------------
int CRecorderUnit::StreamBase::GetCurrentIdx()
{
    return m_CurIdx;
}

//
//----------------------------------------------------------------------------------------------
int CRecorderUnit::StreamBase::GetSize()
{
    return (int)(m_Stream.size());
}

//
//----------------------------------------------------------------------------------------------
void CRecorderUnit::StreamBase::ClearImpl()
{
    for (size_t i = 0; i < m_Stream.size(); ++i)
    {
        delete m_Stream[i];
    }
    m_Stream.clear();
}

//
//----------------------------------------------------------------------------------------------
float   CRecorderUnit::StreamBase::GetStartTime()
{
    if (m_Stream.empty())
    {
        return 0.0f;
    }
    return m_Stream.front()->m_StartTime;
}

//
//----------------------------------------------------------------------------------------------
float   CRecorderUnit::StreamBase::GetEndTime()
{
    if (m_Stream.empty())
    {
        return 0.0f;
    }
    return m_Stream.back()->m_StartTime;
}

//
//----------------------------------------------------------------------------------------------
CRecorderUnit::StreamStr::StreamStr(char const* name, bool bUseIndex /*= false*/)
    : StreamBase(name)
    , m_bUseIndex(bUseIndex)
    , m_uIndexGen(INVALID_INDEX)
{
    if (bUseIndex)
    {
        // Index not supported in disk mode currently
#if !defined(_RELEASE) && !defined(CONSOLE_CONST_CVAR_MODE)
        if (gAIEnv.CVars.Recorder == eAIRM_Disk)
        {
            gEnv->pLog->LogWarning("[AI Recorder] %s is set to use String Index mode but this is not supported in Disk mode yet!", name);
            m_bUseIndex = false;
        }
#endif
    }
}

//
//----------------------------------------------------------------------------------------------
void* CRecorderUnit::StreamStr::GetCurrent(float& startingFrom)
{
    if (m_CurIdx < 0 || m_CurIdx >= (int)m_Stream.size())
    {
        return NULL;
    }
    startingFrom = m_Stream[m_CurIdx]->m_StartTime;
    return (void*)(static_cast<StreamUnit*>(m_Stream[m_CurIdx])->m_String.c_str());
}

//
//----------------------------------------------------------------------------------------------
bool  CRecorderUnit::StreamStr::GetCurrentString(string& sOut, float& startingFrom)
{
    sOut = (char*)GetCurrent(startingFrom);
    return (!sOut.empty());
}

//
//----------------------------------------------------------------------------------------------
void* CRecorderUnit::StreamStr::GetNext(float& startingFrom)
{
    if (++m_CurIdx >= (int)m_Stream.size())
    {
        m_CurIdx = (int)m_Stream.size();
        return NULL;
    }
    startingFrom = m_Stream[m_CurIdx]->m_StartTime;
    return (void*)(static_cast<StreamUnit*>(m_Stream[m_CurIdx])->m_String.c_str());
}

//
//----------------------------------------------------------------------------------------------
bool CRecorderUnit::StreamStr::LoadStringIndex(AZ::IO::HandleType fileHandle)
{
    // Read number of indexes made
    if (!gEnv->pFileIO->Read(fileHandle, &m_uIndexGen, sizeof(m_uIndexGen), true))
    {
        return false;
    }

    for (uint32 uCount = 0; uCount < m_uIndexGen; ++uCount)
    {
        uint32 uIndex = INVALID_INDEX;
        if (!gEnv->pFileIO->Read(fileHandle, &uIndex, sizeof(uIndex), true))
        {
            return false;
        }

        int strLen;
        if (!gEnv->pFileIO->Read(fileHandle, &strLen, sizeof(strLen), true))
        {
            return false;
        }

        if (strLen < 0 || strLen > MAX_EVENT_STRING_LENGTH)
        {
            return false;
        }

        string sString;
        if (strLen != 0)
        {
            std::vector<char> buffer;
            buffer.resize(strLen);
            if (!gEnv->pFileIO->Read(fileHandle, &buffer[0], sizeof(char) * strLen, true))
            {
                return false;
            }
            sString.assign(&buffer[0], strLen);
        }
        else
        {
            sString = "<Empty string>";
        }

        // Add to map
        m_StrIndexLookup.insert(TStrIndexLookup::value_type(sString.c_str(), uIndex));
    }

    return true;
}

//
//----------------------------------------------------------------------------------------------
bool CRecorderUnit::StreamStr::SaveStringIndex(AZ::IO::HandleType fileHandle)
{
    // Write number of indexes made

    if (!gEnv->pFileIO->Write(fileHandle, &m_uIndexGen, sizeof(m_uIndexGen)))
    {
        return false;
    }

    TStrIndexLookup::const_iterator itIter = m_StrIndexLookup.begin();
    TStrIndexLookup::const_iterator itIterEnd = m_StrIndexLookup.end();
    while (itIter != itIterEnd)
    {
        const string& str(itIter->first);
        const uint32& uIndex(itIter->second);
        const int strLen = (str ? str.size() : 0);

        if (!gEnv->pFileIO->Write(fileHandle, &uIndex, sizeof(uIndex)))
        {
            return false;
        }
        if (!gEnv->pFileIO->Write(fileHandle, &strLen, sizeof(strLen)))
        {
            return false;
        }
        if (!str.empty())
        {
            if (!gEnv->pFileIO->Write(fileHandle, str.c_str(), sizeof(char) * strLen))
            {
                return false;
            }
        }

        ++itIter;
    }

    return true;
}

//
//----------------------------------------------------------------------------------------------
uint32 CRecorderUnit::StreamStr::GetOrMakeStringIndex(const char* szString)
{
    uint32 uResult = INVALID_INDEX;

    if (m_bUseIndex && szString)
    {
        uResult = stl::find_in_map(m_StrIndexLookup, szString, INVALID_INDEX);
        if (INVALID_INDEX == uResult)
        {
            // Add it and make new one
            uResult = ++m_uIndexGen;
            m_StrIndexLookup.insert(TStrIndexLookup::value_type(szString, uResult));
        }
    }

    return uResult;
}

//
//----------------------------------------------------------------------------------------------
bool CRecorderUnit::StreamStr::GetStringFromIndex(uint32 uIndex, string& sOut) const
{
    CRY_ASSERT(m_bUseIndex);

    sOut.clear();

    bool bResult = false;
    TStrIndexLookup::const_iterator itIter = m_StrIndexLookup.begin();
    TStrIndexLookup::const_iterator itIterEnd = m_StrIndexLookup.end();
    while (itIter != itIterEnd)
    {
        if (itIter->second == uIndex)
        {
            sOut = itIter->first;
            bResult = true;
            break;
        }
        ++itIter;
    }

    return bResult;
}

//
//----------------------------------------------------------------------------------------------
void CRecorderUnit::StreamStr::AddValue(const IAIRecordable::RecorderEventData* pEventData, float t)
{
    // Make an index for it, but don't record it. It'll be saved to file later with the right index
    if (m_bUseIndex)
    {
        GetOrMakeStringIndex(pEventData->pString);
    }

    m_Stream.push_back(new StreamUnit(t, pEventData->pString));
}


//
//----------------------------------------------------------------------------------------------
bool CRecorderUnit::StreamStr::WriteValue(const IAIRecordable::RecorderEventData* pEventData, float t)
{
    return WriteValue(t, pEventData->pString, CAIRecorder::m_fileHandle);
}


//
//----------------------------------------------------------------------------------------------
bool CRecorderUnit::StreamStr::WriteValue(float t, const char* str, AZ::IO::HandleType fileHandle)
{
    // When using index, we have the string length be all bits on to represent this fact
    int strLen = ~0;
    if (!m_bUseIndex)
    {
        strLen = (str ? strlen(str) : 0);
    }


    if (!gEnv->pFileIO->Write(fileHandle, &t, sizeof(t)))
    {
        return false;
    }
    if (!gEnv->pFileIO->Write(fileHandle, &strLen, sizeof(strLen)))
    {
        return false;
    }
    if (m_bUseIndex)
    {
        uint32 uIndex = GetOrMakeStringIndex(str);
        if (uIndex > INVALID_INDEX)
        {
            if (!gEnv->pFileIO->Write(fileHandle, &uIndex, sizeof(uint32)))
            {
                return false;
            }
        }
    }
    else if (str)
    {
        if (!gEnv->pFileIO->Write(fileHandle, str, sizeof(char) * strLen))
        {
            return false;
        }
    }

    return true;
}

//----------------------------------------------------------------------------------------------
bool CRecorderUnit::StreamStr::LoadValue(AZ::IO::HandleType fileHandle)
{
    string name;
    float time;
    if (!LoadValue(time, name, fileHandle))
    {
        return false;
    }

    // Check chronological ordering
    if (!m_Stream.empty() && m_Stream.back()->m_StartTime > time)
    {
        gEnv->pLog->LogError("[AI Recorder] Aborting - events are not recorded in chronological order");
        return false;
    }

    m_Stream.push_back(new StreamUnit(time, name.c_str()));
    return true;
}

//
//----------------------------------------------------------------------------------------------
void CRecorderUnit::StreamStr::Clear()
{
    CRecorderUnit::StreamBase::Clear();

    m_StrIndexLookup.clear();
    m_uIndexGen = INVALID_INDEX;
}

//
//----------------------------------------------------------------------------------------------
bool CRecorderUnit::StreamStr::SaveStream(AZ::IO::HandleType fileHandle)
{
    int counter(m_Stream.size());

    if (!gEnv->pFileIO->Write(fileHandle, &counter, sizeof(counter)))
    {
        return false;
    }
    for (uint32 idx(0); idx < m_Stream.size(); ++idx)
    {
        StreamUnit* pCurUnit = static_cast<StreamUnit*>(m_Stream[idx]);
        float t = pCurUnit->m_StartTime;
        const char* pStr = pCurUnit->m_String.c_str();
        if (!WriteValue(t, pStr, fileHandle))
        {
            return false;
        }
    }
    return true;
}

bool CRecorderUnit::StreamStr::LoadValue(float& t, string& name, AZ::IO::HandleType fileHandle)
{
    int strLen;
    uint64_t bytesRead = 0;
    if (!gEnv->pFileIO->Read(fileHandle, &t, sizeof(t), true))
    {
        return false;
    }
    if (!gEnv->pFileIO->Read(fileHandle, &strLen, sizeof(strLen), true))
    {
        return false;
    }

    // When using index, we have the string length be all bits on to represent this fact
    bool bIsEmpty = true;
    if (strLen == ~0)
    {
        uint32 uIndex = INVALID_INDEX;
        if (!gEnv->pFileIO->Read(fileHandle, &uIndex, sizeof(uint32), true))
        {
            return false;
        }
        bIsEmpty = (!GetStringFromIndex(uIndex, name));
    }
    else
    {
        if (strLen < 0 || strLen > MAX_EVENT_STRING_LENGTH)
        {
            return false;
        }
        if (strLen != 0)
        {
            std::vector<char> buffer;
            buffer.resize(strLen);
            if (!gEnv->pFileIO->Read(fileHandle, &buffer[0], sizeof(char) * strLen, true))
            {
                return false;
            }
            name.assign(&buffer[0], strLen);
            bIsEmpty = false;
        }
    }

    if (bIsEmpty)
    {
        name = "<Empty string>";
    }

    return true;
}

//
//----------------------------------------------------------------------------------------------
bool CRecorderUnit::StreamStr::LoadStream(AZ::IO::HandleType fileHandle)
{
    string name;
    int counter;
    gEnv->pFileIO->Read(fileHandle, &counter, sizeof(counter));

    for (int idx(0); idx < counter; ++idx)
    {
        if (!LoadValue(fileHandle))
        {
            return false;
        }
    }

    return true;
}



//
//----------------------------------------------------------------------------------------------
void* CRecorderUnit::StreamVec3::GetCurrent(float& startingFrom)
{
    if (m_CurIdx < 0 || m_CurIdx >= (int)m_Stream.size())
    {
        return NULL;
    }
    startingFrom = m_Stream[m_CurIdx]->m_StartTime;
    return (void*)(&static_cast<StreamUnit*>(m_Stream[m_CurIdx])->m_Pos);
}

//
//----------------------------------------------------------------------------------------------
bool  CRecorderUnit::StreamVec3::GetCurrentString(string& sOut, float& startingFrom)
{
    sOut.clear();

    Vec3* pVec = (Vec3*)GetCurrent(startingFrom);
    if (pVec)
    {
        sOut = CryStringUtils::ToString(*pVec);
    }

    return (!sOut.empty());
}

//
//----------------------------------------------------------------------------------------------
void* CRecorderUnit::StreamVec3::GetNext(float& startingFrom)
{
    if (++m_CurIdx >= (int)m_Stream.size())
    {
        m_CurIdx = m_Stream.size();
        return NULL;
    }
    startingFrom = m_Stream[m_CurIdx]->m_StartTime;
    return (void*)(&static_cast<StreamUnit*>(m_Stream[m_CurIdx])->m_Pos);
}

//
//----------------------------------------------------------------------------------------------
bool CRecorderUnit::StreamVec3::FilterPoint(const IAIRecordable::RecorderEventData* pEventData) const
{
    bool bResult = true;

    // Simple point filtering.
    if (m_bUseFilter && !m_Stream.empty())
    {
        StreamUnit* pLastUnit(static_cast<StreamUnit*>(m_Stream.back()));
        const Vec3 vDelta = pLastUnit->m_Pos - pEventData->pos;
        bResult = (vDelta.len2() > 0.25f);
    }

    return bResult;
}

//
//----------------------------------------------------------------------------------------------
void CRecorderUnit::StreamVec3::AddValue(const IAIRecordable::RecorderEventData* pEventData, float t)
{
    if (FilterPoint(pEventData))
    {
        m_Stream.push_back(new StreamUnit(t, pEventData->pos));
    }
}


//
//----------------------------------------------------------------------------------------------
bool CRecorderUnit::StreamVec3::WriteValue(const IAIRecordable::RecorderEventData* pEventData, float t)
{
    bool bResult = true;

    if (FilterPoint(pEventData))
    {
        bResult = WriteValue(t, pEventData->pos, CAIRecorder::m_fileHandle);
    }

    return bResult;
}


//
//----------------------------------------------------------------------------------------------
bool CRecorderUnit::StreamVec3::WriteValue(float t, const Vec3& vec, AZ::IO::HandleType fileHandle)
{
    if (!gEnv->pFileIO->Write(fileHandle, &t, sizeof(t)))
    {
        return false;
    }
    if (!gEnv->pFileIO->Write(fileHandle, &vec.x, sizeof(vec.x)))
    {
        return false;
    }
    if (!gEnv->pFileIO->Write(fileHandle, &vec.y, sizeof(vec.y)))
    {
        return false;
    }
    if (!gEnv->pFileIO->Write(fileHandle, &vec.z, sizeof(vec.z)))
    {
        return false;
    }
    return true;
}

//
//----------------------------------------------------------------------------------------------
bool CRecorderUnit::StreamVec3::LoadValue(float& t, Vec3& vec, AZ::IO::HandleType fileHandle)
{
    if (!gEnv->pFileIO->Read(fileHandle, &t, sizeof(t), true))
    {
        return false;
    }
    if (!gEnv->pFileIO->Read(fileHandle, &vec.x, sizeof(vec.x), true))
    {
        return false;
    }
    if (!gEnv->pFileIO->Read(fileHandle, &vec.y, sizeof(vec.y), true))
    {
        return false;
    }
    if (!gEnv->pFileIO->Read(fileHandle, &vec.z, sizeof(vec.z), true))
    {
        return false;
    }
    return true;
}


//----------------------------------------------------------------------------------------------
bool CRecorderUnit::StreamVec3::LoadValue(AZ::IO::HandleType fileHandle)
{
    Vec3 vec;
    float time;
    if (!LoadValue(time, vec, fileHandle))
    {
        return false;
    }

    // Check chronological ordering
    if (!m_Stream.empty() && m_Stream.back()->m_StartTime > time)
    {
        gEnv->pLog->LogError("[AI Recorder] Aborting - events are not recorded in chronological order");
        return false;
    }

    m_Stream.push_back(new StreamUnit(time, vec));
    return true;
}

//
//----------------------------------------------------------------------------------------------
bool CRecorderUnit::StreamVec3::LoadStream(AZ::IO::HandleType fileHandle)
{
    int counter;
    gEnv->pFileIO->Read(fileHandle, &counter, sizeof(counter));

    for (int idx(0); idx < counter; ++idx)
    {
        if (!LoadValue(fileHandle))
        {
            return false;
        }
    }

    return true;
}

//
//----------------------------------------------------------------------------------------------
bool CRecorderUnit::StreamVec3::SaveStream(AZ::IO::HandleType fileHandle)
{
    int counter(m_Stream.size());
    if (!gEnv->pFileIO->Write(fileHandle, &counter, sizeof(counter)))
    {
        return false;
    }
    for (uint32 idx(0); idx < m_Stream.size(); ++idx)
    {
        StreamUnit* pCurUnit(static_cast<StreamUnit*>(m_Stream[idx]));
        float time = pCurUnit->m_StartTime;
        Vec3& vect = pCurUnit->m_Pos;
        if (!WriteValue(time, vect, fileHandle))
        {
            return false;
        }
    }
    return true;
}


//
//----------------------------------------------------------------------------------------------
void* CRecorderUnit::StreamFloat::GetCurrent(float& startingFrom)
{
    if (m_CurIdx < 0 || m_CurIdx >= (int)m_Stream.size())
    {
        return NULL;
    }
    startingFrom = m_Stream[m_CurIdx]->m_StartTime;
    return (void*)(&static_cast<StreamUnit*>(m_Stream[m_CurIdx])->m_Val);
}

//
//----------------------------------------------------------------------------------------------
bool  CRecorderUnit::StreamFloat::GetCurrentString(string& sOut, float& startingFrom)
{
    sOut.clear();

    float* pF = (float*)GetCurrent(startingFrom);
    if (pF)
    {
        sOut = CryStringUtils::ToString(*pF);
    }

    return (!sOut.empty());
}

//
//----------------------------------------------------------------------------------------------
void* CRecorderUnit::StreamFloat::GetNext(float& startingFrom)
{
    if (++m_CurIdx >= (int)m_Stream.size())
    {
        m_CurIdx = (int)m_Stream.size();
        return NULL;
    }
    startingFrom = m_Stream[m_CurIdx]->m_StartTime;
    return (void*)(&static_cast<StreamUnit*>(m_Stream[m_CurIdx])->m_Val);
}

//
//----------------------------------------------------------------------------------------------
bool CRecorderUnit::StreamFloat::FilterPoint(const IAIRecordable::RecorderEventData* pEventData) const
{
    bool bResult = true;

    // Simple point filtering.
    if (m_bUseFilter && !m_Stream.empty())
    {
        StreamUnit* pLastUnit(static_cast<StreamUnit*>(m_Stream.back()));
        float fDelta = pLastUnit->m_Val - pEventData->val;
        bResult = (fabsf(fDelta) > FLT_EPSILON);
    }

    return bResult;
}

//
//----------------------------------------------------------------------------------------------
void CRecorderUnit::StreamFloat::AddValue(const IAIRecordable::RecorderEventData* pEventData, float t)
{
    if (FilterPoint(pEventData))
    {
        m_Stream.push_back(new StreamUnit(t, pEventData->val));
    }
}


//
//----------------------------------------------------------------------------------------------
bool CRecorderUnit::StreamFloat::WriteValue(const IAIRecordable::RecorderEventData* pEventData, float t)
{
    bool bResult = true;

    if (FilterPoint(pEventData))
    {
        bResult = WriteValue(t, pEventData->val, CAIRecorder::m_fileHandle);
    }

    return bResult;
}


//
//----------------------------------------------------------------------------------------------
bool CRecorderUnit::StreamFloat::WriteValue(float t, float val, AZ::IO::HandleType fileHandle)
{
    if (!gEnv->pFileIO->Write(fileHandle, &t, sizeof(t)))
    {
        return false;
    }
    if (!gEnv->pFileIO->Write(fileHandle, &val, sizeof(val)))
    {
        return false;
    }
    return true;
}

//
//----------------------------------------------------------------------------------------------
bool CRecorderUnit::StreamFloat::LoadValue(float& t, float& val, AZ::IO::HandleType fileHandle)
{
    if (!gEnv->pFileIO->Read(fileHandle, &t, sizeof(t), true))
    {
        return false;
    }
    if (!gEnv->pFileIO->Read(fileHandle, &val, sizeof(val), true))
    {
        return false;
    }
    return true;
}


//----------------------------------------------------------------------------------------------
bool CRecorderUnit::StreamFloat::LoadValue(AZ::IO::HandleType fileHandle)
{
    float   val;
    float time;
    if (!LoadValue(time, val, fileHandle))
    {
        return false;
    }

    // Check chronological ordering
    if (!m_Stream.empty() && m_Stream.back()->m_StartTime > time)
    {
        gEnv->pLog->LogError("[AI Recorder] Aborting - events are not recorded in chronological order");
        return false;
    }

    m_Stream.push_back(new StreamUnit(time, val));
    return true;
}

//
//----------------------------------------------------------------------------------------------
bool CRecorderUnit::StreamFloat::LoadStream(AZ::IO::HandleType fileHandle)
{
    int counter;
    gEnv->pFileIO->Read(fileHandle, &counter, sizeof(counter));

    for (int idx(0); idx < counter; ++idx)
    {
        if (!LoadValue(fileHandle))
        {
            return false;
        }
    }

    return true;
}

//
//----------------------------------------------------------------------------------------------
bool CRecorderUnit::StreamFloat::SaveStream(AZ::IO::HandleType fileHandle)
{
    int counter(m_Stream.size());
    if (!gEnv->pFileIO->Write(fileHandle, &counter, sizeof(counter)))
    {
        return false;
    }
    for (uint32 idx(0); idx < m_Stream.size(); ++idx)
    {
        StreamUnit* pCurUnit(static_cast<StreamUnit*>(m_Stream[idx]));
        float time = pCurUnit->m_StartTime;
        float val = pCurUnit->m_Val;
        if (!WriteValue(time, val, fileHandle))
        {
            return false;
        }
    }
    return true;
}



//
//----------------------------------------------------------------------------------------------
IAIDebugStream* CRecorderUnit::GetStream(IAIRecordable::e_AIDbgEvent streamTag)
{
    return m_Streams[streamTag];
}

//
//----------------------------------------------------------------------------------------------
void CRecorderUnit::ResetStreams(CTimeValue startTime)
{
    for (TStreamMap::iterator strItr(m_Streams.begin()); strItr != m_Streams.end(); ++strItr)
    {
        StreamBase* pStream(strItr->second);
        if (pStream)
        {
            pStream->Clear();
        }
    }
    m_startTime = startTime;
}

//
//----------------------------------------------------------------------------------------------
bool CRecorderUnit::Save(AZ::IO::HandleType fileHandle)
{
    {
        // Make a list of all that are using string indexes
        TStreamMap stringIndexMap;
        for (TStreamMap::iterator strItr(m_Streams.begin()); strItr != m_Streams.end(); ++strItr)
        {
            StreamBase* pStream(strItr->second);
            if (pStream && pStream->IsUsingStringIndex())
            {
                IAIRecordable::e_AIDbgEvent type = ((strItr->first)); // The kind of stream
                stringIndexMap[type] = pStream;
            }
        }

        // Write the number of streams using string-index lookups
        int numStreams(stringIndexMap.size());
        if (!gEnv->pFileIO->Write(fileHandle, &numStreams, sizeof(numStreams)))
        {
            return false;
        }

        for (TStreamMap::iterator strItr(stringIndexMap.begin()); strItr != stringIndexMap.end(); ++strItr)
        {
            StreamBase* pStream(strItr->second);
            const int intType((strItr->first)); // The kind of stream
            if (!gEnv->pFileIO->Write(fileHandle, &intType, sizeof(intType)))
            {
                return false;
            }
            if (pStream && !pStream->SaveStringIndex(fileHandle))
            {
                return false;
            }
        }
    }

    {
        // Write the number of streams stored
        int numStreams(m_Streams.size());
        if (!gEnv->pFileIO->Write(fileHandle, &numStreams, sizeof(numStreams)))
        {
            return false;
        }

        for (TStreamMap::iterator strItr(m_Streams.begin()); strItr != m_Streams.end(); ++strItr)
        {
            StreamBase* pStream(strItr->second);
            const int intType((strItr->first)); // The kind of stream
            if (!gEnv->pFileIO->Write(fileHandle, &intType, sizeof(intType)))
            {
                return false;
            }
            if (pStream && !pStream->SaveStream(fileHandle))
            {
                return false;
            }
        }
    }

    return true;
}


//
//----------------------------------------------------------------------------------------------
bool CRecorderUnit::Load(AZ::IO::HandleType fileHandle)
{
    for (TStreamMap::iterator strItr(m_Streams.begin()); strItr != m_Streams.end(); ++strItr)
    {
        StreamBase* pStream(strItr->second);
        if (pStream)
        {
            pStream->Clear();
        }
    }

    {
        // Read the number of streams using string-index lookups stored
        int numStreams;
        if (!gEnv->pFileIO->Read(fileHandle, &numStreams, sizeof(numStreams), true))
        {
            return false;
        }

        for (int i(0); i < numStreams; ++i)
        {
            IAIRecordable::e_AIDbgEvent intType; // The kind of stream
            if (!gEnv->pFileIO->Read(fileHandle, &intType, sizeof(intType), true))
            {
                return false;
            }

            if (!m_Streams[intType]->LoadStringIndex(fileHandle))
            {
                return false;
            }
        }
    }

    {
        // Read the number of streams stored
        int numStreams;
        if (!gEnv->pFileIO->Read(fileHandle, &numStreams, sizeof(numStreams), true))
        {
            return false;
        }

        for (int i(0); i < numStreams; ++i)
        {
            IAIRecordable::e_AIDbgEvent intType; // The kind of stream
            if (!gEnv->pFileIO->Read(fileHandle, &intType, sizeof(intType), true))
            {
                return false;
            }

            if (!m_Streams[intType]->LoadStream(fileHandle))
            {
                return false;
            }
        }
    }

    return true;
}


//
//----------------------------------------------------------------------------------------------
CAIRecorder::CAIRecorder()
{
    m_recordingMode = eAIRM_Off;
    m_pLog = NULL;
    m_unitLifeCounter = 0;
}

//
//----------------------------------------------------------------------------------------------
void CAIRecorder::Init(void)
{
    if (!m_pLog)
    {
        m_pLog = gEnv->pLog;
    }

    if (gEnv->pSystem)
    {
        ISystemEventDispatcher* pDispatcher = gEnv->pSystem->GetISystemEventDispatcher();
        if (pDispatcher)
        {
            pDispatcher->RegisterListener(this);
        }
    }

    if (gEnv->pEntitySystem)
    {
        gEnv->pEntitySystem->GetIEntityPoolManager()->AddListener(this, "AIRecorder", IEntityPoolListener::EntityReturnedToPool);
    }
}

//
//----------------------------------------------------------------------------------------------
bool CAIRecorder::IsRunning(void) const
{
    return (m_recordingMode != eAIRM_Off);
}

//
//----------------------------------------------------------------------------------------------
void CAIRecorder::GetCompleteFilename(char const* szFilename, bool bAppendFileCount, string& sOut) const
{
    const bool bIsEditor = gEnv->IsEditor();

    if (!szFilename || !szFilename[0])
    {
        // Use current level
        szFilename = PathUtil::GetFileName(gEnv->pGame->GetIGameFramework()->GetLevelName());
    }

    if (!szFilename || !szFilename[0])
    {
        // Use defualt
        szFilename = AIRECORDER_DEFAULT_FILE;
    }

    CRY_ASSERT(szFilename && szFilename[0]);

    string sVersion;
    if (bIsEditor)
    {
        sVersion = AIRECORDER_EDITOR_AUTO_APPEND;
    }
    else
    {
        // Get current date line
        char szDate[128];

        time_t ltime;
        time(&ltime);
#ifdef AZ_COMPILER_MSVC
        tm _tm;
        localtime_s(&_tm, &ltime);
        strftime(szDate, AZ_ARRAY_SIZE(szDate), "Date(%d %b %Y) Time(%H %M %S)", &_tm);
#else
        tm* pTm = localtime(&ltime);
        strftime(szDate, 128, "Date(%d %b %Y) Time(%H %M %S)", pTm);
#endif

        // Get current version line
        const SFileVersion& fileVersion = gEnv->pSystem->GetFileVersion();

        sVersion.Format("Build(%d) %s", fileVersion[0], szDate);
    }

    string sBaseFile;
    sBaseFile.Format("%s_%s", szFilename, sVersion.c_str());

    sOut = PathUtil::Make(AIRECORDER_FOLDER, sBaseFile.c_str(), "rcd");
    gEnv->pCryPak->MakeDir(AIRECORDER_FOLDER);

    // Check if file already exists
    int iFileCount = 0;
    AZ::IO::HandleType fileHandleChecker = AZ::IO::InvalidHandle;
    if (bAppendFileCount && !bIsEditor)
    {
        gEnv->pFileIO->Open(sOut.c_str(), AZ::IO::GetOpenModeFromStringMode("rb"), fileHandleChecker);
    }
    if (fileHandleChecker != AZ::IO::InvalidHandle)
    {
        string sNewOut;
        while (fileHandleChecker != AZ::IO::InvalidHandle)
        {
            gEnv->pFileIO->Close(fileHandleChecker);
            sNewOut.Format("%s(%d)", sBaseFile.c_str(), ++iFileCount);
            sOut = PathUtil::Make(AIRECORDER_FOLDER, sNewOut.c_str(), "rcd");
            gEnv->pFileIO->Open(sOut.c_str(), AZ::IO::GetOpenModeFromStringMode("rb"), fileHandleChecker);
        }
    }
}

//
//----------------------------------------------------------------------------------------------
void CAIRecorder::OnReset(IAISystem::EResetReason reason)
{
    //if (!gEnv->IsEditor())
    //  return;

    if (gAIEnv.CVars.DebugRecordAuto == 0)
    {
        return;
    }

    const bool bIsSerializingFile = 0 != gEnv->pSystem->IsSerializingFile();
    if (!bIsSerializingFile)
    {
        // Handle starting/stoping the recorder
        switch (reason)
        {
        case IAISystem::RESET_INTERNAL:
        case IAISystem::RESET_INTERNAL_LOAD:
            Stop();
            Reset();
            break;

        case IAISystem::RESET_EXIT_GAME:
            Stop();
            break;

        case IAISystem::RESET_ENTER_GAME:
            Reset();
            Start(eAIRM_Memory);
            break;
        }
    }
}

//
//----------------------------------------------------------------------------------------------
bool CAIRecorder::AddListener(IAIRecorderListener* pListener)
{
    return stl::push_back_unique(m_Listeners, pListener);
}

//
//----------------------------------------------------------------------------------------------
bool CAIRecorder::RemoveListener(IAIRecorderListener* pListener)
{
    return stl::find_and_erase(m_Listeners, pListener);
}

//
//----------------------------------------------------------------------------------------------
void CAIRecorder::Start(EAIRecorderMode mode, const char* filename)
{
    if (IsRunning())
    {
        return;
    }

    Init(); // make sure

    // Clear any late arriving events from last run
    Reset();

    string sFile;
    GetCompleteFilename(filename, true, sFile);

    switch (mode)
    {
    case eAIRM_Memory:
    {
        m_recordingMode = eAIRM_Memory;

        // No action required to start recording
    }
    break;
    case eAIRM_Disk:
    {
        m_recordingMode = eAIRM_Disk;

        // Redundant check
        if (m_fileHandle != AZ::IO::InvalidHandle)
        {
            gEnv->pFileIO->Close(m_fileHandle);
            m_fileHandle = AZ::IO::InvalidHandle;
        }

        // Open for streaming, using static file pointer
        m_fileHandle = fxopen(sFile.c_str(), "wb");

        if (m_fileHandle != AZ::IO::InvalidHandle)
        {
            // Write preamble
            if (!Write(m_fileHandle))
            {
                gEnv->pFileIO->Close(m_fileHandle);
                m_fileHandle = AZ::IO::InvalidHandle;
            }
        }
        // Leave it open
    }
    break;
    default:
        m_recordingMode = eAIRM_Off;

        // In other modes does nothing
        // In mode 0, will quite happily be started and stopped doing nothing
        break;
    }

    // Notify listeners
    TListeners::iterator itNext = m_Listeners.begin();
    while (itNext != m_Listeners.end())
    {
        TListeners::iterator itListener = itNext++;
        IAIRecorderListener* pListener = *itListener;
        CRY_ASSERT(pListener);

        pListener->OnRecordingStart(m_recordingMode, sFile.c_str());
    }
}

//
//----------------------------------------------------------------------------------------------
void CAIRecorder::Stop(const char* filename)
{
    if (!IsRunning())
    {
        return;
    }

    EAIRecorderMode mode = m_recordingMode;
    m_recordingMode = eAIRM_Off;
    m_unitLifeCounter = 0;

    string sFile;
    GetCompleteFilename(filename, true, sFile);

    // Close the recorder file, should have been streaming
    if (m_fileHandle != AZ::IO::InvalidHandle)
    {
        gEnv->pFileIO->Close(m_fileHandle);
        m_fileHandle = AZ::IO::InvalidHandle;
        LogRecorderSaved(sFile.c_str());
    }

    switch (mode)
    {
    case eAIRM_Disk:
    {
        // Only required to close the recorder file
    }
    break;
    case eAIRM_Memory:
    {
        // Dump the history to disk
        Save(filename);
    }
    break;
    default:
        break;
    }

    // Notify listeners
    TListeners::iterator itNext = m_Listeners.begin();
    while (itNext != m_Listeners.end())
    {
        TListeners::iterator itListener = itNext++;
        IAIRecorderListener* pListener = *itListener;
        CRY_ASSERT(pListener);

        pListener->OnRecordingStop(sFile.c_str());
    }
}

//
//----------------------------------------------------------------------------------------------
CAIRecorder::~CAIRecorder()
{
    Shutdown();
}

//
//----------------------------------------------------------------------------------------------
void CAIRecorder::Shutdown(void)
{
    if (IsRunning())
    {
        Stop();
    }

    if (gEnv->pSystem)
    {
        ISystemEventDispatcher* pDispatcher = gEnv->pSystem->GetISystemEventDispatcher();
        if (pDispatcher)
        {
            pDispatcher->RemoveListener(this);
        }
    }

    if (gEnv->pEntitySystem)
    {
        gEnv->pEntitySystem->GetIEntityPoolManager()->RemoveListener(this);
    }

    DestroyDummyObjects();

    for (TUnits::iterator it = m_Units.begin(); it != m_Units.end(); ++it)
    {
        if (CAIObject* aiObject = gAIEnv.pObjectContainer->GetAIObject(it->second->GetId()))
        {
            aiObject->ResetRecorderUnit();
        }

        delete it->second;
    }

    m_Units.clear();
}

//
//----------------------------------------------------------------------------------------------
void CAIRecorder::OnSystemEvent(ESystemEvent event, UINT_PTR wparam, UINT_PTR lparam)
{
    if (event == ESYSTEM_EVENT_FAST_SHUTDOWN ||
        event == ESYSTEM_EVENT_LEVEL_UNLOAD)
    {
        if (IsRunning())
        {
            Stop();
        }
    }
}

//
//----------------------------------------------------------------------------------------------
void CAIRecorder::OnEntityReturnedToPool(EntityId entityId, IEntity* pEntity)
{
    assert(pEntity);

    // If this AI is being recorded, stop recording on it so a new record is created for it later
    CAIObject* pAI = static_cast<CAIObject*>(pEntity->GetAI());
    if (pAI)
    {
        pAI->ResetRecorderUnit();
    }
}

//
//----------------------------------------------------------------------------------------------
void CAIRecorder::Update()
{
    // This is never called so far
}

//
//----------------------------------------------------------------------------------------------
bool CAIRecorder::Load(const char* filename)
{
    if (IsRunning())
    {
        return false;              // To avoid undefined behaviour
    }
    // Open the AI recorder dump file
    string sFile = filename;
    if (sFile.empty())
    {
        GetCompleteFilename(filename, false, sFile);
    }
    AZ::IO::HandleType fileHandle = fxopen(sFile.c_str(), "rb");

    bool bSuccess = false;
    if (fileHandle != AZ::IO::InvalidHandle)
    {
        // Update static file pointer
        m_fileHandle = fileHandle;

        bSuccess = Read(fileHandle);

        m_fileHandle = AZ::IO::InvalidHandle;
        gEnv->pFileIO->Close(fileHandle);
        fileHandle = AZ::IO::InvalidHandle;
    }

    if (bSuccess)
    {
        // Notify listeners
        TListeners::iterator itNext = m_Listeners.begin();
        while (itNext != m_Listeners.end())
        {
            TListeners::iterator itListener = itNext++;
            IAIRecorderListener* pListener = *itListener;
            CRY_ASSERT(pListener);

            pListener->OnRecordingLoaded(sFile.c_str());
        }
    }

    return bSuccess;
}

bool CAIRecorder::Read(AZ::IO::HandleType fileHandle)
{
    CAISystem* pAISystem = GetAISystem();
    CRY_ASSERT(pAISystem);

    // File header
    RecorderFileHeader fileHeader;
    gEnv->pFileIO->Read(fileHandle, &fileHeader, sizeof(fileHeader));
    if (!fileHeader.check())
    {
        return false;
    }
    if (fileHeader.version > AIRECORDER_VERSION)
    {
        m_pLog->LogError("[AI Recorder] Saved AI Recorder log is of wrong version number");
        gEnv->pFileIO->Close(fileHandle);
        return false;
    }

    // Clear all units streams
    Reset();

    // String stores name of each unit
    string name;

    //std::vector <tAIObjectID> tempIDs;  // Ids for units we create just during loading, to skip over data
    //std::map <tAIObjectID,tAIObjectID> idMap;   // Mapping the ids recorded in the file into those used in this run

    // For each record
    for (int i = 0; i < fileHeader.unitsNumber; i++)
    {
        UnitHeader header;
        // Read entity name and ID
        if (!gEnv->pFileIO->Read(fileHandle, &header, sizeof(header), true))
        {
            return false;
        }
        if (!header.check())
        {
            return false;
        }
        if (header.nameLength > 0)
        {
            std::vector<char> buffer;
            buffer.resize(header.nameLength);
            if (!gEnv->pFileIO->Read(fileHandle, &buffer[0], sizeof(char) * header.nameLength, true))
            {
                return false;
            }
            name.assign(&buffer[0], header.nameLength);
        }
        else
        {
            name.clear();
        }

        string sDummyName;
        uint32 uLifeCounter = (header.ID >> 32);
        sDummyName.Format("%s_%u", name.c_str(), uLifeCounter);

        // Create a dummy object to represent this recording
        TDummyObjects::value_type refDummy;
        gAIEnv.pAIObjectManager->CreateDummyObject(refDummy, sDummyName);
        if (!refDummy.IsNil())
        {
            m_DummyObjects.push_back(refDummy);

            // Create map entry from the old, possibly changed ID to the current one
            //idMap[header.ID] = liveID;
            CAIObject* pLiveObject = refDummy.GetAIObject();
            CRecorderUnit* unit = pLiveObject->CreateAIDebugRecord();

            if (!unit)
            {
                return false;
            }
            if (!unit->Load(fileHandle))
            {
                return false;
            }
        }
        else
        {
            m_pLog->LogError("[AI Recorder] Failed to create a Recorder Unit for \'%s\'", name.c_str());
        }
    }

    // After the "announced" data in the file, we continue, looking for more
    // A streamed log will typically have 0 length announced data (providing the IDs/names etc)
    // and all data will follow afterwards

    // For each event that follows
    // (Kevin) - What's the purpose of this section? Is it still needed? (24/8/09)
    bool bEndOfFile = false;
    do
    {
        // Read event metadata
        StreamedEventHeader header;
        bEndOfFile = !gEnv->pFileIO->Read(fileHandle, &header, sizeof(header), true);
        if (!bEndOfFile)
        {
            if (!header.check())
            {
                m_pLog->LogError("[AI Recorder] corrupt event found reading streamed section");
                return false;
            }
            else
            {
                CRY_ASSERT_MESSAGE(false, "Recorder has streamded data. This code needs to be readded.");

                //int liveID = idMap[header.unitID];
                //if (!liveID)
                //{
                //  // New unannounced ID
                //  m_pLog->LogWarning("[AI Recorder] Unknown ID %d found in stream", header.unitID);
                //  // Generate a random ID - unlikely to collide.
                //  liveID = abs( (int)cry_rand()%100000 ) + 10000;
                //  tempIDs.push_back(liveID);
                //  idMap[header.unitID] = liveID;
                //}

                //CRecorderUnit *unit = AddUnit(liveID, true);
                //if (!unit) return false;
                //if (!unit->LoadEvent( (IAIRecordable::e_AIDbgEvent) header.streamID)) return false;
            }
        }
    } while (!bEndOfFile);

    return true;
}

//----------------------------------------------------------------------------------------------
void CAIRecorder::LogRecorderSaved(const char* filename) const
{
    char resolveBuffer[CRYFILE_MAX_PATH] = { 0 };
    gEnv->pFileIO->ResolvePath(filename, resolveBuffer, CRYFILE_MAX_PATH);
    m_pLog->Log("[AI Recorder] File saved to '%s'", resolveBuffer);
}
//----------------------------------------------------------------------------------------------
bool CAIRecorder::Save(const char* filename)
{
    if (IsRunning())
    {
        return false;  // To avoid undefined behaviour
    }
    // This method should not change state at all
    bool bSuccess = false;

    // Open the AI recorder dump file
    string sFile = filename;

    if (sFile.empty() || !gEnv->pCryPak->IsAbsPath(sFile.c_str()))
    {
        GetCompleteFilename(filename, true, sFile);
    }    

    AZ::IO::HandleType fileHandle = fxopen(sFile.c_str(), "wb");

    if (fileHandle != AZ::IO::InvalidHandle)
    {
        // Update static file pointer
        m_fileHandle = fileHandle;

        bSuccess = Write(fileHandle);

        m_fileHandle = AZ::IO::InvalidHandle;

        gEnv->pFileIO->Close(fileHandle);
    }

    if (bSuccess)
    {
        // Notify listeners
        TListeners::iterator itNext = m_Listeners.begin();
        while (itNext != m_Listeners.end())
        {
            TListeners::iterator itListener = itNext++;
            IAIRecorderListener* pListener = *itListener;
            CRY_ASSERT(pListener);

            pListener->OnRecordingSaved(sFile.c_str());
        }

        LogRecorderSaved(sFile.c_str());
    }
    else
    {
        m_pLog->LogError("[AI Recorder] Save dump failed");
    }

    return bSuccess;
}

bool CAIRecorder::Write(AZ::IO::HandleType fileHandle)
{
    // File header
    RecorderFileHeader fileHeader;
    fileHeader.version = AIRECORDER_VERSION;
    fileHeader.unitsNumber = m_Units.size();
    if (!gEnv->pFileIO->Write(fileHandle, &fileHeader, sizeof(fileHeader)))
    {
        return false;
    }

    // For each record
    for (TUnits::iterator unitIter = m_Units.begin(); unitIter != m_Units.end(); ++unitIter)
    {
        CRecorderUnit* pUnit = unitIter->second;
        CRY_ASSERT(pUnit);
        if (!pUnit)
        {
            continue;
        }

        // Record entity name and ID (ID may be not be preserved)
        const string& name = pUnit->GetName();

        UnitHeader header;
        header.nameLength = (int)name.size();
        header.ID = pUnit->GetId();

        if (!gEnv->pFileIO->Write(fileHandle, &header, sizeof(header)))
        {
            return false;
        }
        if (!gEnv->pFileIO->Write(fileHandle, name.c_str(), sizeof(char) * header.nameLength))
        {
            return false;
        }

        if (!pUnit->Save(fileHandle))
        {
            return false;
        }
    }
    return true;
}



//
//----------------------------------------------------------------------------------------------
CRecorderUnit* CAIRecorder::AddUnit(CWeakRef<CAIObject> refObject, bool force)
{
    CRY_ASSERT(refObject.IsSet());

    CRecorderUnit* pNewUnit = NULL;

    if ((IsRunning() || force) && refObject.IsSet())
    {
        // Create a new unit only if activated or required
        const uint32 lifeIndex = ++m_unitLifeCounter;
        pNewUnit = new CRecorderUnit(this, GetAISystem()->GetFrameStartTime(), refObject, lifeIndex);
        m_Units[pNewUnit->GetId()] = pNewUnit;
    }

    return pNewUnit;
}

//
//----------------------------------------------------------------------------------------------
void CAIRecorder::Reset(void)
{
    DestroyDummyObjects();

    const CTimeValue frameStartTime = gEnv->pTimer->GetFrameStartTime();
    for (TUnits::iterator unitIter = m_Units.begin(); unitIter != m_Units.end(); ++unitIter)
    {
        unitIter->second->ResetStreams(frameStartTime);
        if (CAIObject* aiObject = gAIEnv.pObjectContainer->GetAIObject(unitIter->second->GetId()))
        {
            aiObject->ResetRecorderUnit();
        }

        delete unitIter->second;
    }

    m_Units.clear();
}

//
//----------------------------------------------------------------------------------------------
void CAIRecorder::DestroyDummyObjects()
{
    for (TDummyObjects::iterator dummyIter = m_DummyObjects.begin(); dummyIter != m_DummyObjects.end(); ++dummyIter)
    {
        TDummyObjects::value_type refDummy = *dummyIter;
        // this object was created by the manager, so the manager must know to remove it
        gAIEnv.pAIObjectManager->OnObjectRemoved(refDummy.GetAIObject());
        refDummy.Release();
    }

    m_DummyObjects.clear();
}

#endif //CRYAISYSTEM_DEBUG
