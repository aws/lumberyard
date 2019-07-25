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

#ifndef CRYINCLUDE_CRYACTION_SERIALIZATION_XMLCPBIN_READER_XMLCPB_BUFFERREADER_H
#define CRYINCLUDE_CRYACTION_SERIALIZATION_XMLCPBIN_READER_XMLCPB_BUFFERREADER_H
#pragma once

#include "../XMLCPB_Common.h"
#include "IPlatformOS.h"

namespace XMLCPB {
    class CReader;

    struct SBufferReader
    {
        explicit SBufferReader(IGeneralMemoryHeap* pHeap)
            : m_pHeap(pHeap)
            , m_pBuffer(NULL)
            , m_bufferSize(0)
        {
        }

        ~SBufferReader()
        {
            if (m_pBuffer)
            {
                m_pHeap->Free(m_pBuffer);
            }
        }

        inline const uint8* GetPointer(FlatAddr addr) const
        {
            assert(addr < m_bufferSize);
            return &(m_pBuffer[addr]);
        }

        inline void CopyTo(uint8* pDst, FlatAddr srcAddr, uint32 bytesToCopy) const
        {
            assert(srcAddr < m_bufferSize);
            assert(srcAddr + bytesToCopy <= m_bufferSize);

            memcpy(pDst, GetPointer(srcAddr), bytesToCopy);
        }

        void ReadFromMemory(CReader& Reader, const uint8* pData, uint32 dataSize, uint32 readSize, uint32& outReadLoc);

    private:
        SBufferReader(const SBufferReader&);
        SBufferReader& operator = (const SBufferReader&);

    private:
        _smart_ptr<IGeneralMemoryHeap> m_pHeap;
        uint8* m_pBuffer;
        size_t m_bufferSize;
    };
}  // end namespace


#endif // CRYINCLUDE_CRYACTION_SERIALIZATION_XMLCPBIN_READER_XMLCPB_BUFFERREADER_H
