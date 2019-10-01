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

#include "CryLegacy_precompiled.h"
#include "XMLCPB_BufferReader.h"
#include "XMLCPB_Reader.h"

using namespace XMLCPB;

//////////////////////////////////////////////////////////////////////////

void SBufferReader::ReadFromMemory(CReader& Reader, const uint8* pData, uint32 dataSize, uint32 readSize, uint32& outReadLoc)
{
    assert(m_bufferSize == 0);
    assert (!m_pBuffer);

    m_pBuffer = (uint8*)m_pHeap->Malloc(readSize, "");

    m_bufferSize = readSize;

    Reader.ReadDataFromMemory(pData, dataSize, const_cast<uint8*>(GetPointer(0)), readSize, outReadLoc);
}

