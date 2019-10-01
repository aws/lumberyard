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
#include "XMLCPB_Reader.h"
#include "CryActionCVars.h"
#include <IZLibCompressor.h>


using namespace XMLCPB;

int CReader::MAX_NUM_LIVE_NODES = 64;


//////////////////////////////////////////////////////////////////////////
_smart_ptr<IGeneralMemoryHeap> CReader::CreateHeap()
{
    return CryGetIMemoryManager()->CreateGeneralExpandingMemoryHeap(8 * 1024 * 1024, 0, "Load heap");
}

//////////////////////////////////////////////////////////////////////////

CReader::CReader(IGeneralMemoryHeap* pHeap)
    : m_pHeap(pHeap)
    , m_liveNodes(NAlloc::GeneralHeapAlloc(pHeap))
    , m_buffer(pHeap)
    , m_tableTags(pHeap)
    , m_tableAttrNames(pHeap)
    , m_tableStrData(pHeap)
    , m_tableAttrSets(pHeap)
    , m_firstFreeLiveNode(0)
    , m_maxNumActiveNodes(0)
    , m_numActiveNodes(0)
    , m_errorReading(false)
    , m_nodesAddrTable(NAlloc::GeneralHeapAlloc(pHeap))
    , m_totalSize(0)
    , m_nodesDataSize(0)
    , m_pZLibBuffer(NULL)
    , m_pZLibCompressedBuffer(NULL)
    , m_ZLibBufferSizeWithData(0)
    , m_ZLibBufferSizeAlreadyRead(0)
    , m_numNodes(0)
{
    InitializeDataTypeInfo();
    m_liveNodes.resize(MAX_NUM_LIVE_NODES, CNodeLiveReader(*this));
}



//////////////////////////////////////////////////////////////////////////

CReader::~CReader()
{
    SAFE_DELETE_ARRAY(m_pZLibBuffer);
    SAFE_DELETE_ARRAY(m_pZLibCompressedBuffer);
    //  CryLog(" Binary SaveGame Reader: max live nodes active in the reader: %d", m_maxNumActiveNodes );
}


//////////////////////////////////////////////////////////////////////////

CNodeLiveReaderRef CReader::GetRoot()
{
    CNodeLiveReaderRef nodeRef(*this, XMLCPB_ROOTNODE_ID);    // root node always have same live ID and is always valid

    assert(m_liveNodes[0].IsValid());
    return nodeRef;
}


//////////////////////////////////////////////////////////////////////////

CNodeLiveReaderRef CReader::CreateNodeRef()
{
    return CNodeLiveReaderRef(*this);
}


//////////////////////////////////////////////////////////////////////////

const CNodeLiveReader& CReader::ActivateLiveNodeFromCompact(NodeGlobalID nodeId)
{
    NodeLiveID liveId = m_firstFreeLiveNode;
    CNodeLiveReader& node = m_liveNodes[liveId];
    node.ActivateFromCompact(liveId, nodeId);

    // find the now first free live node
    bool found = false;
    uint32 size = m_liveNodes.size();
    for (int i = m_firstFreeLiveNode + 1; i < size; ++i)
    {
        const CNodeLiveReader& nodeIter = m_liveNodes[i];
        if (!nodeIter.IsValid())
        {
            found = true;
            m_firstFreeLiveNode = i;
            break;
        }
    }

    assert(found);
    m_numActiveNodes++;
    if (m_numActiveNodes > m_maxNumActiveNodes)
    {
        m_maxNumActiveNodes = m_numActiveNodes;
    }


    return node;
}



//////////////////////////////////////////////////////////////////////////

CNodeLiveReader* CReader::GetNodeLive(NodeLiveID nodeId)
{
    CNodeLiveReader* pNode = NULL;
    assert(nodeId < m_liveNodes.size());

    if (nodeId < m_liveNodes.size())
    {
        pNode = &(m_liveNodes[ nodeId ]);
    }

    return pNode;
}


//////////////////////////////////////////////////////////////////////////

void CReader::FreeNodeLive(NodeLiveID nodeId)
{
    if (nodeId < m_firstFreeLiveNode)
    {
        m_firstFreeLiveNode = nodeId;
    }

    assert(nodeId < m_liveNodes.size());

    m_liveNodes[nodeId].Reset();
    m_numActiveNodes--;
}



//////////////////////////////////////////////////////////////////////////

void CReader::CheckErrorFlag(IPlatformOS::EFileOperationCode code)
{
    if (!m_errorReading)
    {
        m_errorReading = (code != IPlatformOS::eFOC_Success);
    }
}


//////////////////////////////////////////////////////////////////////////

FlatAddr CReader::ReadFromBuffer(FlatAddr addr, uint8*& rdata, uint32 len)
{
    m_buffer.CopyTo(rdata, addr, len);

    return addr + len;
}

//////////////////////////////////////////////////////////////////////////

void CReader::ReadDataFromMemory(const uint8* pData, uint32 dataSize, void* pSrc, uint32 numBytes, uint32& outReadLoc)
{
    assert(pData);
    assert(pSrc);

    if (!m_errorReading && pData && pSrc && numBytes > 0)
    {
        const int dataRemaining = (dataSize - numBytes + outReadLoc);
        m_errorReading = (dataRemaining < 0);

        if (!m_errorReading)
        {
            memcpy(pSrc, &pData[outReadLoc], numBytes);
            outReadLoc += numBytes;
        }
    }
}


//////////////////////////////////////////////////////////////////////////

bool CReader::ReadBinaryMemory(const uint8* pData, uint32 uSize)
{
    m_errorReading = !(pData && uSize > 0);

    if (!m_errorReading)
    {
        uint32 uReadLoc = 0;
        m_totalSize = uSize;

        SFileHeader fileHeader;
        ReadDataFromMemory(pData, uSize, &fileHeader, sizeof(fileHeader), uReadLoc);

        if (fileHeader.m_fileTypeCheck != fileHeader.FILETYPECHECK)
        {
            m_errorReading = true;
        }

        if (!m_errorReading)
        {
            m_nodesDataSize = fileHeader.m_sizeNodes;
            m_numNodes = fileHeader.m_numNodes;
            m_buffer.ReadFromMemory(*this, pData, uSize, fileHeader.m_sizeNodes, uReadLoc);
            m_tableTags.ReadFromMemory(*this, pData, uSize, fileHeader.m_tags, uReadLoc);
            m_tableAttrNames.ReadFromMemory(*this, pData, uSize, fileHeader.m_attrNames, uReadLoc);
            m_tableStrData.ReadFromMemory(*this, pData, uSize, fileHeader.m_strData, uReadLoc);
            m_tableAttrSets.ReadFromMemory(*this, pData, uSize, fileHeader, uReadLoc);
            CreateNodeAddressTables();
        }

        if (!m_errorReading)
        {
            const CNodeLiveReader& root = ActivateLiveNodeFromCompact(m_numNodes - 1);   // the last node is always the root
            assert(root.GetLiveId() == XMLCPB_ROOTNODE_ID);
        }
    }

    if (m_errorReading)
    {
        CryWarning(VALIDATOR_MODULE_SYSTEM, VALIDATOR_ERROR, "XMLCPB ERROR: while reading from memory location");
    }

    return !m_errorReading;
}


//////////////////////////////////////////////////////////////////////////
// for debug purposes
#ifndef _RELEASE
void CReader::SaveTestFiles()
{
    m_tableTags.WriteStringsIntoTextFile("tableTags.txt");
    m_tableAttrNames.WriteStringsIntoTextFile("tableAttrNames.txt");
    m_tableStrData.WriteStringsIntoTextFile("tableStrData.txt");
}
#endif

//////////////////////////////////////////////////////////////////////////

void CReader::CreateNodeAddressTables()
{
    m_nodesAddrTable.resize(m_numNodes);

    FlatAddr addr = 0;

    // TODO (improve): first, because the partially initialized table is used in the node calls,
    //   and also because those nodes are temporary and manually activated just to calculate the address of the next node
    for (uint32 n = 0; n < m_numNodes; ++n)
    {
        m_nodesAddrTable[n] = addr;
        CNodeLiveReader node(*this);
        node.ActivateFromCompact(0, n);
        addr = node.GetAddrNextNode();
        assert(addr > m_nodesAddrTable[n]);
    }

    assert(addr == m_nodesDataSize);
}


