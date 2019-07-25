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
#include "XMLCPB_StringTableReader.h"
#include "XMLCPB_Reader.h"

using namespace XMLCPB;


//////////////////////////////////////////////////////////////////////////
CStringTableReader::CStringTableReader(IGeneralMemoryHeap* pHeap)
    : m_buffer(pHeap)
    , m_stringAddrs(NAlloc::GeneralHeapAlloc(pHeap))
{
}

//////////////////////////////////////////////////////////////////////////
const char* CStringTableReader::GetString(StringID stringId) const
{
    assert(stringId < m_stringAddrs.size());
    char* pString = (char*)(m_buffer.GetPointer(m_stringAddrs[stringId]));
    return pString;
}

//////////////////////////////////////////////////////////////////////////

void CStringTableReader::ReadFromMemory(CReader& Reader, const uint8* pData, uint32 dataSize, const SFileHeader::SStringTable& headerInfo, uint32& outReadLoc)
{
    assert(m_stringAddrs.empty());

    m_stringAddrs.resize(headerInfo.m_numStrings);
    FlatAddr* pstringAddrsTable = &(m_stringAddrs[0]);
    Reader.ReadDataFromMemory(pData, dataSize, pstringAddrsTable, sizeof(FlatAddr) * headerInfo.m_numStrings, outReadLoc);

    m_buffer.ReadFromMemory(Reader, pData, dataSize, headerInfo.m_sizeStringData, outReadLoc);
}


//////////////////////////////////////////////////////////////////////////
// debug / statistics purposes

#ifndef _RELEASE
void CStringTableReader::WriteStringsIntoTextFile(const char* pFileName)
{
    AZ::IO::HandleType fileHandle = gEnv->pCryPak->FOpen(pFileName, "wb");
    if (fileHandle == AZ::IO::InvalidHandle)
    {
        return;
    }
    for (int i = 0; i < m_stringAddrs.size(); ++i)
    {
        string logString;
        logString.Format("%s\r\n", GetString(i));
        gEnv->pCryPak->FWrite(logString.c_str(), logString.size(), fileHandle);
    }
    gEnv->pCryPak->FClose(fileHandle);
}
#endif
