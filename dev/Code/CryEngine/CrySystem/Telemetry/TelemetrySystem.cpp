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

// Description : Telemetry system.


#include "StdAfx.h"
#include "TelemetryFormat.h"
#include "TelemetrySystem.h"

namespace Telemetry
{
    CTelemetrySystem::CSendBuffer::CSendBuffer()
        : m_pBegin(NULL)
        , m_pEnd(NULL)
        , m_pPos(NULL)
        , m_pChunkPos(NULL)
        , m_sessionId(0)
        , m_lastFlushTime(FLT_MAX)
    {
    }

    CTelemetrySystem::CSendBuffer::~CSendBuffer()
    {
        Shutdown();
    }

    bool CTelemetrySystem::CSendBuffer::Init(size_t size, uint64 sessionId)
    {
        Shutdown();

        size = std::max(size, sizeof(SChunkHeader) + sizeof(uint64));

        if (m_pBegin = static_cast<uint8*>(CryModuleMalloc(size)))
        {
            m_pEnd                  = m_pBegin + size;
            m_pPos                  = m_pBegin;
            m_sessionId         = sessionId;
            m_lastFlushTime = gEnv->pSystem->GetITimer()->GetCurrTime();

            WritePacketHeader();

            return true;
        }
        else
        {
            Shutdown();

            return false;
        }
    }

    void CTelemetrySystem::CSendBuffer::Shutdown()
    {
        if (m_pBegin)
        {
            CryModuleFree(m_pBegin);
        }

        m_pBegin                = NULL;
        m_pEnd                  = NULL;
        m_pPos                  = NULL;
        m_pChunkPos         = NULL;
        m_sessionId         = 0;
        m_lastFlushTime = FLT_MAX;

        m_streams.clear();
    }

    void CTelemetrySystem::CSendBuffer::Update()
    {
        if (ICVar* pCVar = gEnv->pConsole->GetCVar("sys_telemetry_keep_alive"))
        {
            if ((gEnv->pSystem->GetITimer()->GetCurrTime() - m_lastFlushTime) > pCVar->GetFVal())
            {
                Flush();
            }
        }
    }

    bool CTelemetrySystem::CSendBuffer::AttachStream(IStream* pStream)
    {
        if (pStream)
        {
            return stl::push_back_unique(m_streams, pStream);
        }
        else
        {
            return false;
        }
    }

    void CTelemetrySystem::CSendBuffer::DetachStream(IStream* pStream)
    {
        stl::find_and_erase_all(m_streams, pStream);
    }

    bool CTelemetrySystem::CSendBuffer::BeginChunk(uint16 type)
    {
        if (!m_pChunkPos)
        {
            m_pChunkPos = m_pPos;

            SChunkHeader    chunkHeader;

            chunkHeader.id      = type;
            chunkHeader.size    = 0;

            return DoWrite(&chunkHeader, sizeof(chunkHeader));
        }
        else
        {
            return false;
        }
    }

    bool CTelemetrySystem::CSendBuffer::Write(const void* pData, size_t size)
    {
        if (m_pChunkPos)
        {
            return DoWrite(pData, size);
        }
        else
        {
            return false;
        }
    }

    bool CTelemetrySystem::CSendBuffer::Write(const string& data)
    {
        if (m_pChunkPos)
        {
            return DoWrite(data.c_str(), data.length() + 1);
        }
        else
        {
            return false;
        }
    }

    bool CTelemetrySystem::CSendBuffer::Write(const TVariadicParams& params)
    {
        if (m_pChunkPos)
        {
            bool    error = false;

            for (size_t iParam = 0, iEndParam = params.ParamCount(); iParam != iEndParam; ++iParam)
            {
                const TParam& param = params[iParam];

                if (const void* pData = param.Data())
                {
                    error |= !DoWrite(pData, param.Size());
                }
            }

            return !error;
        }
        else
        {
            return false;
        }
    }

    bool CTelemetrySystem::CSendBuffer::EndChunk()
    {
        if (m_pChunkPos)
        {
            reinterpret_cast<SChunkHeader*>(m_pChunkPos)->size = static_cast<uint16>(m_pPos - m_pChunkPos);

            m_pChunkPos = NULL;

            return true;
        }
        else
        {
            return false;
        }
    }

    void CTelemetrySystem::CSendBuffer::Flush(IStream* pStream)
    {
        uint8* pChunkPos = m_pChunkPos;

        if (pChunkPos)
        {
            if (pStream)
            {
                pStream->Write(m_pBegin, pChunkPos - m_pBegin);
            }
            else
            {
                for (TStreamVector::const_iterator iStream = m_streams.begin(), iEndStream = m_streams.end(); iStream != iEndStream; ++iStream)
                {
                    (*iStream)->Write(m_pBegin, pChunkPos - m_pBegin);
                }
            }

            size_t  chunkSize = m_pPos - pChunkPos;

            m_pPos          = m_pBegin;
            m_pChunkPos = NULL;

            WritePacketHeader();

            if (static_cast<ptrdiff_t>(chunkSize) <= (m_pEnd - m_pPos))
            {
                memcpy(m_pPos, pChunkPos, chunkSize);

                m_pChunkPos = m_pPos;

                m_pPos += chunkSize;
            }
            else
            {
                CryWarning(VALIDATOR_MODULE_SYSTEM, VALIDATOR_ERROR, "Telemetry data is too big for send buffer\n");
            }
        }
        else if (m_pPos != m_pBegin)
        {
            if (pStream)
            {
                pStream->Write(m_pBegin, m_pPos - m_pBegin);
            }
            else
            {
                for (TStreamVector::const_iterator iStream = m_streams.begin(), iEndStream = m_streams.end(); iStream != iEndStream; ++iStream)
                {
                    (*iStream)->Write(m_pBegin, m_pPos - m_pBegin);
                }
            }

            m_pPos          = m_pBegin;
            m_pChunkPos = NULL;

            WritePacketHeader();
        }

        m_lastFlushTime = gEnv->pSystem->GetITimer()->GetCurrTime();
    }

    void CTelemetrySystem::CSendBuffer::WritePacketHeader()
    {
        if (BeginChunk(EChunkId::PacketHeader))
        {
            Write(m_sessionId);

            EndChunk();
        }
    }

    bool CTelemetrySystem::CSendBuffer::DoWrite(const void* pData, size_t size)
    {
#if TELEMETRY_DEBUG
        CRY_ASSERT(pData);

        CRY_ASSERT(size > 0);
#endif //TELEMETRY_DEBUG

        if ((m_pPos + size) <= m_pEnd)
        {
            memcpy(m_pPos, pData, size);

            m_pPos += size;
        }
        else
        {
            Flush();

            if ((m_pPos + size) <= m_pEnd)
            {
                memcpy(m_pPos, pData, size);

                m_pPos += size;
            }
            else
            {
                m_pPos          = m_pChunkPos;
                m_pChunkPos = NULL;
            }
        }

        return m_pPos > m_pChunkPos;
    }

    CTelemetrySystem::CTelemetrySystem()
        : m_globalVerbosity(TELEMETRY_VERBOSITY_DEFAULT)
    {
    }

    CTelemetrySystem::~CTelemetrySystem()
    {
        Shutdown();
    }

    bool CTelemetrySystem::Init()
    {
        TScopedLock scopedLock(m_mutex);

        return m_sendBuffer.Init(0x100, GenerateSessionId());
    }

    void CTelemetrySystem::Shutdown()
    {
        TScopedLock scopedLock(m_mutex);

        m_sendBuffer.Flush();

        m_sendBuffer.Shutdown();
    }

    void CTelemetrySystem::Release()
    {
        delete this;
    }

    void CTelemetrySystem::Update()
    {
        TScopedLock scopedLock(m_mutex);

        m_sendBuffer.Update();
    }

    bool CTelemetrySystem::AttachStream(IStream* pStream)
    {
        if (pStream)
        {
            TScopedLock scopedLock(m_mutex);

            m_sendBuffer.Flush();

            if (m_sendBuffer.AttachStream(pStream))
            {
                if (m_sendBuffer.BeginChunk(EChunkId::SessionInfo))
                {
                    ICVar* pGameNameCVar = gEnv->pConsole->GetCVar("sys_game_name");

                    CRY_ASSERT(pGameNameCVar);

                    const string    gameName = pGameNameCVar->GetString();

                    m_sendBuffer.Write(gameName);

                    const string    userName = gEnv->pSystem->GetUserName();

                    m_sendBuffer.Write(userName);

                    const string    buildTime = __DATE__ " " __TIME__;

                    m_sendBuffer.Write(buildTime);

                    const int32 buildVersion = gEnv->pSystem->GetBuildVersion().v[0];

                    m_sendBuffer.Write(buildVersion);

                    m_sendBuffer.EndChunk();
                }

                m_sendBuffer.Flush(pStream);

                return true;
            }
        }

        return false;
    }

    void CTelemetrySystem::DetachStream(IStream* pStream)
    {
        TScopedLock scopedLock(m_mutex);

        m_sendBuffer.DetachStream(pStream);
    }

    void CTelemetrySystem::SetGlobalVerbosity(uint32 verbosity)
    {
#if TELEMETRY_DEBUG
        CRY_ASSERT((verbosity >= TELEMETRY_VERBOSITY_OFF) && (verbosity <= TELEMETRY_VERBOSITY_HIGHEST));
#endif //TELEMETRY_DEBUG

        TScopedLock scopedLock(m_mutex);

        m_globalVerbosity = std::max<uint32>(verbosity, TELEMETRY_MIN_VERBOSITY);
    }

    void CTelemetrySystem::SetAspectVerbosity(uint32 aspectId, uint32 verbosity)
    {
#if TELEMETRY_DEBUG
        CRY_ASSERT((verbosity >= TELEMETRY_VERBOSITY_OFF) && (verbosity <= TELEMETRY_VERBOSITY_NOT_SET));
#endif //TELEMETRY_DEBUG

        TScopedLock scopedLock(m_mutex);

        GetOrCreateAspectDeclaration(aspectId).verbosity = std::max<uint32>(verbosity, TELEMETRY_MIN_VERBOSITY);
    }

    bool CTelemetrySystem::ShouldSend(uint32 aspectId, uint32 verbosity)
    {
#if TELEMETRY_DEBUG
        CRY_ASSERT((verbosity >= TELEMETRY_VERBOSITY_LOWEST) && (verbosity <= TELEMETRY_VERBOSITY_HIGHEST));
#endif //TELEMETRY_DEBUG

        TScopedLock scopedLock(m_mutex);

        uint32          aspectVerbosity = GetOrCreateAspectDeclaration(aspectId).verbosity;

        if (aspectVerbosity == TELEMETRY_VERBOSITY_NOT_SET)
        {
            return verbosity <= m_globalVerbosity;
        }
        else
        {
            return verbosity <= aspectVerbosity;
        }
    }

    void CTelemetrySystem::Send(uint32 aspectId, uint32 eventId, const char* pTableParams, const TVariadicParams& params, uint32 verbosity)
    {
#if TELEMETRY_DEBUG
        CRY_ASSERT((verbosity >= TELEMETRY_VERBOSITY_LOWEST) && (verbosity <= TELEMETRY_VERBOSITY_HIGHEST));
#endif //TELEMETRY_DEBUG

        TScopedLock scopedLock(m_mutex);

        if (ShouldSend(aspectId, verbosity))
        {
            GetOrCreateEventDeclaration(aspectId, eventId, pTableParams, params, verbosity);

            bool    filter = true;

            if (size_t paramCount = params.ParamCount())
            {
                filter = false;

                for (size_t iParam = 0; iParam != paramCount; ++iParam)
                {
                    filter |= params[iParam].Filter();
                }
            }

            if (filter)
            {
                if (m_sendBuffer.BeginChunk(EChunkId::Event))
                {
                    m_sendBuffer.Write(aspectId);

                    m_sendBuffer.Write(eventId);

                    m_sendBuffer.Write(params);

                    m_sendBuffer.EndChunk();
                }
            }
        }
    }

    void CTelemetrySystem::Flush()
    {
        TScopedLock scopedLock(m_mutex);

        m_sendBuffer.Flush();
    }

    uint64 CTelemetrySystem::GenerateSessionId()
    {
        return ((gEnv->pSystem->GetITimer()->GetAsyncTime().GetValue() << 16) & 0xffffffff00000000LL) | CCrc32::Compute(gEnv->pSystem->GetUserName());
    }

    CTelemetrySystem::SAspectDeclaration& CTelemetrySystem::GetOrCreateAspectDeclaration(uint32 aspectId)
    {
        TAspectDeclarationMap::iterator iAspectDeclaration = m_aspectDeclarations.find(aspectId);

        if (iAspectDeclaration == m_aspectDeclarations.end())
        {
            iAspectDeclaration = m_aspectDeclarations.insert(TAspectDeclarationMap::value_type(aspectId, SAspectDeclaration())).first;
        }

        return iAspectDeclaration->second;
    }

    CTelemetrySystem::SEventDeclaration& CTelemetrySystem::GetOrCreateEventDeclaration(uint32 aspectId, uint32 eventId, const char* pTableParams, const TVariadicParams& params, uint32 verbosity)
    {
        TEventDeclarationMap::iterator  iEventDeclaration = m_eventDeclarations.find(TEventDeclarationMap::key_type(aspectId, eventId));

        if (iEventDeclaration != m_eventDeclarations.end())
        {
#if TELEMETRY_DEBUG
            CRY_ASSERT(iEventDeclaration->second == SEventDeclaration(pTableParams, params, verbosity));
#endif //TELEMETRY_DEBUG
        }
        else
        {
            iEventDeclaration = m_eventDeclarations.insert(TEventDeclarationMap::value_type(TEventDeclarationMap::key_type(aspectId, eventId), SEventDeclaration(pTableParams, params, verbosity))).first;

            if (m_sendBuffer.BeginChunk(EChunkId::EventDeclaration))
            {
                m_sendBuffer.Write(aspectId);

                m_sendBuffer.Write(eventId);

                const SEventDeclaration& eventDeclaration = iEventDeclaration->second;

                m_sendBuffer.Write(eventDeclaration.tableParams);

                uint8   numberOfParams = static_cast<uint8>(eventDeclaration.paramTypeIds.size());

                m_sendBuffer.Write(numberOfParams);

                if (numberOfParams)
                {
                    m_sendBuffer.Write(&eventDeclaration.paramTypeIds[0], numberOfParams);
                }

                m_sendBuffer.EndChunk();
            }
        }

        return iEventDeclaration->second;
    }
}