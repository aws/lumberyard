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

#ifndef CRYINCLUDE_CRYACTION_SERIALIZATION_XMLCPBIN_READER_XMLCPB_READERINTERFACE_H
#define CRYINCLUDE_CRYACTION_SERIALIZATION_XMLCPBIN_READER_XMLCPB_READERINTERFACE_H
#pragma once

#include "XMLCPB_Reader.h"

namespace XMLCPB {
    // this class is just to give a clear access to the intended interface functions and only them, without using a bunch of friend declarations
    class CReaderInterface
    {
    public:
        explicit CReaderInterface(IGeneralMemoryHeap* pHeap)
            : m_reader(pHeap)
        {
        }

        CNodeLiveReaderRef          GetRoot () { return m_reader.GetRoot(); }
        CNodeLiveReaderRef          CreateNodeRef () { return m_reader.CreateNodeRef(); }
        bool                                        ReadBinaryFile(const char* pFileName) { return m_reader.ReadBinaryFile(pFileName); }
        bool                                        ReadBinaryMemory(const uint8* pData, uint32 uSize) { return m_reader.ReadBinaryMemory(pData, uSize); }
        void                                        SaveTestFiles() { m_reader.SaveTestFiles(); }

    private:

        CReader                                 m_reader;
    };
}  // end namespace


#endif // CRYINCLUDE_CRYACTION_SERIALIZATION_XMLCPBIN_READER_XMLCPB_READERINTERFACE_H
