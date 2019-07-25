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

#ifndef CRYINCLUDE_CRYACTION_SERIALIZATION_XMLCPBIN_READER_XMLCPB_ATTRSETTABLEREADER_H
#define CRYINCLUDE_CRYACTION_SERIALIZATION_XMLCPBIN_READER_XMLCPB_ATTRSETTABLEREADER_H
#pragma once

#include "../XMLCPB_Common.h"
#include "XMLCPB_BufferReader.h"
#include "IPlatformOS.h"

// an "AttrSet" defines the datatype + tagId of all the attributes of a node.  each datatype+tagId entry is called a "header".
// those sets are stored in a common table because many nodes use the same type of attrs


namespace XMLCPB {
    class CReader;

    class CAttrSetTableReader
    {
    public:
        explicit CAttrSetTableReader(IGeneralMemoryHeap* pHeap);

        uint32          GetNumAttrs(AttrSetID setId) const;
        uint16          GetHeaderAttr(AttrSetID setId, uint32 indAttr) const;

        uint32          GetNumSets() const { return m_setAddrs.size(); }

        void                ReadFromMemory(CReader& Reader, const uint8* pData, uint32 dataSize, const SFileHeader& headerInfo, uint32& outReadLoc);

    private:
        typedef DynArray<FlatAddr16, int, NArray::SmallDynStorage<NAlloc::GeneralHeapAlloc> > SetAddrVec;
        typedef DynArray<uint8, int, NArray::SmallDynStorage<NAlloc::GeneralHeapAlloc> > NumAttrsVec;

        SetAddrVec                  m_setAddrs;
        NumAttrsVec                 m_numAttrs;
        SBufferReader               m_buffer;
    };
}  // end namespace


#endif // CRYINCLUDE_CRYACTION_SERIALIZATION_XMLCPBIN_READER_XMLCPB_ATTRSETTABLEREADER_H
