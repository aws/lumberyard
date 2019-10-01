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

#ifndef CRYINCLUDE_CRYACTION_SERIALIZATION_XMLCPBIN_READER_XMLCPB_STRINGTABLEREADER_H
#define CRYINCLUDE_CRYACTION_SERIALIZATION_XMLCPBIN_READER_XMLCPB_STRINGTABLEREADER_H
#pragma once

#include "../XMLCPB_Common.h"
#include "XMLCPB_BufferReader.h"
#include "IPlatformOS.h"

namespace XMLCPB {
    class CReader;

    class CStringTableReader
    {
    public:
        explicit CStringTableReader(IGeneralMemoryHeap* pHeap);

        const char* GetString(StringID stringId) const;
        int                 GetNumStrings() const { return m_stringAddrs.size(); }

        void                ReadFromMemory(CReader& Reader, const uint8* pData, uint32 dataSize, const SFileHeader::SStringTable& headerInfo, uint32& outReadLoc);

        // for debug and testing
        #ifndef _RELEASE
        void                WriteStringsIntoTextFile(const char* pFileName);
        #endif

    private:
        typedef DynArray<FlatAddr, int, NArray::SmallDynStorage<NAlloc::GeneralHeapAlloc> > FlatAddrVec;

    private:
        SBufferReader                                       m_buffer;
        FlatAddrVec                                         m_stringAddrs;
    };
}  // end namespace


#endif // CRYINCLUDE_CRYACTION_SERIALIZATION_XMLCPBIN_READER_XMLCPB_STRINGTABLEREADER_H
