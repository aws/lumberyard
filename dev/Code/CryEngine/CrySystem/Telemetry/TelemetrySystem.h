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


#ifndef CRYINCLUDE_CRYSYSTEM_TELEMETRY_TELEMETRYSYSTEM_H
#define CRYINCLUDE_CRYSYSTEM_TELEMETRY_TELEMETRYSYSTEM_H
#pragma once


#include "StlUtils.h"
#include "TelemetryConfig.h"
#include "TelemetryParams.h"
#include "ITelemetryStream.h"
#include "ITelemetrySystem.h"

namespace Telemetry
{
    class CTelemetrySystem
        : public ITelemetrySystem
    {
    public:

        CTelemetrySystem();

        ~CTelemetrySystem();

        bool Init();

        void Shutdown();

        void Release();

        void Update();

        bool AttachStream(IStream* pStream);

        void DetachStream(IStream* pStream);

        void SetGlobalVerbosity(uint32 verbosity);

        void SetAspectVerbosity(uint32 aspectId, uint32 verbosity);

        bool ShouldSend(uint32 aspectId, uint32 verbosity);

        void Send(uint32 aspectId, uint32 eventId, const char* pTableParams, const TVariadicParams& params, uint32 verbosity);

        void Flush();

    private:

        struct SAspectDeclaration
        {
            inline SAspectDeclaration()
                : verbosity(TELEMETRY_VERBOSITY_NOT_SET)
            {
            }

            uint32  verbosity;
        };

        typedef AZStd::unordered_map<uint32, SAspectDeclaration> TAspectDeclarationMap;

        typedef std::vector<uint8> TTypeIdVector;

        struct SEventDeclaration
        {
            inline SEventDeclaration()
            {
            }

            inline SEventDeclaration(const char* pTableParams, const TVariadicParams& params, uint32 _verbosity)
                : verbosity(_verbosity)
            {
                if (pTableParams)
                {
                    tableParams = pTableParams;
                }

                if (size_t   paramCount = params.ParamCount())
                {
                    paramTypeIds.reserve(paramCount);

                    for (size_t iParam = 0; iParam != paramCount; ++iParam)
                    {
                        paramTypeIds.push_back(params[iParam].TypeId());
                    }
                }
            }

            inline bool operator == (const SEventDeclaration& rhs) const
            {
                return (tableParams == rhs.tableParams) && (paramTypeIds == rhs.paramTypeIds) && (verbosity == rhs.verbosity);
            }

            string              tableParams;

            TTypeIdVector   paramTypeIds;

            uint32              verbosity;
        };

        struct hash_pair
        {
            enum        // parameters for hash table
            {
                bucket_size = 4,        // 0 < bucket_size
                min_buckets = 8
            };    // min_buckets = 2 ^^ N, 0 < N
            size_t operator()(const std::pair<uint32, uint32>& key) const
            {
                return key.first ^ ((key.second << 13) | (key.second >> 19));
            }
        };
        typedef AZStd::unordered_map<std::pair<uint32, uint32>, SEventDeclaration, hash_pair> TEventDeclarationMap;

        class CSendBuffer
        {
        public:

            CSendBuffer();

            ~CSendBuffer();

            bool Init(size_t size, uint64 sessionId);

            void Shutdown();

            void Update();

            bool AttachStream(IStream* pStream);

            void DetachStream(IStream* pStream);

            bool BeginChunk(uint16 type);

            bool Write(const void* pData, size_t size);

            bool Write(const string& data);

            bool Write(const TVariadicParams& params);

            template <typename TYPE>
            inline bool Write(const TYPE& data)
            {
                if (m_pChunkPos)
                {
                    return DoWrite(&data, sizeof(data));
                }
                else
                {
                    return false;
                }
            }

            bool EndChunk();

            void Flush(IStream* pStream = NULL);

        private:

            typedef std::vector<IStream*> TStreamVector;

            void WritePacketHeader();

            bool DoWrite(const void* pData, size_t size);

            uint8* m_pBegin, * m_pEnd, * m_pPos, * m_pChunkPos;

            uint64              m_sessionId;

            TStreamVector   m_streams;

            float                   m_lastFlushTime;
        };

        typedef CryAutoLock<CryMutex> TScopedLock;

        uint64 GenerateSessionId();

        SAspectDeclaration& GetOrCreateAspectDeclaration(uint32 aspectId);

        SEventDeclaration& GetOrCreateEventDeclaration(uint32 aspectId, uint32 eventId, const char* pTableParams, const TVariadicParams& params, uint32 verbosity);

        TAspectDeclarationMap   m_aspectDeclarations;

        TEventDeclarationMap    m_eventDeclarations;

        CSendBuffer                     m_sendBuffer;

        uint32                              m_globalVerbosity;

        CryMutex                            m_mutex;
    };
}

#endif // CRYINCLUDE_CRYSYSTEM_TELEMETRY_TELEMETRYSYSTEM_H
