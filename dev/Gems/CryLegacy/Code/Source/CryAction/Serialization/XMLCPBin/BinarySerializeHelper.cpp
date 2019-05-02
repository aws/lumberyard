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

// Description : Manages one-off reader/writer usages for binary serialization


#include "CryLegacy_precompiled.h"
#include "BinarySerializeHelper.h"

#if defined(AZ_RESTRICTED_PLATFORM)
    #if defined(AZ_PLATFORM_XENIA)
        #include "Xenia/BinarySerializeHelper_cpp_xenia.inl"
    #elif defined(AZ_PLATFORM_PROVO)
        #include "Provo/BinarySerializeHelper_cpp_provo.inl"
    #endif
#endif

//////////////////////////////////////////////////////////////////////////
CBinarySerializedObject::CBinarySerializedObject(const char* szSection)
    : m_sSection(szSection)
    , m_nRefCount(0)
    , m_uSerializedDataSize(0)
    , m_pSerializedData(0)
{
    assert(szSection && szSection[0]);
}

//////////////////////////////////////////////////////////////////////////
CBinarySerializedObject::~CBinarySerializedObject()
{
    FreeData();
}

//////////////////////////////////////////////////////////////////////////
void CBinarySerializedObject::FreeData()
{
    //SAFE_DELETE_ARRAY(m_pSerializedData);
    if (m_pSerializedData)
    {
        free(m_pSerializedData);
        m_pSerializedData = NULL;
    }

    m_uSerializedDataSize = 0;
}

//////////////////////////////////////////////////////////////////////////
void CBinarySerializedObject::GetMemoryUsage(ICrySizer* pSizer) const
{
    pSizer->Add(*this);
    pSizer->Add(m_pSerializedData, m_uSerializedDataSize);
}

//////////////////////////////////////////////////////////////////////////
bool CBinarySerializedObject::IsEmpty() const
{
    return (!m_pSerializedData || m_uSerializedDataSize == 0);
}

//////////////////////////////////////////////////////////////////////////
void CBinarySerializedObject::Reset()
{
    FreeData();
}

//////////////////////////////////////////////////////////////////////////
void CBinarySerializedObject::Serialize(TSerialize& serialize)
{
    ISerialize* pISerialize = GetImpl(serialize);

    // FIXME: ISerialize helper does not support POD arrays...
    if (serialize.IsReading())
    {
        CSimpleSerializeWithDefaults<CSerializeReaderXMLCPBin>* pReaderSerialize = static_cast<CSimpleSerializeWithDefaults<CSerializeReaderXMLCPBin>*>(pISerialize);
        pReaderSerialize->GetInnerImpl()->ValueByteArray("m_pSerializedData", m_pSerializedData, m_uSerializedDataSize);
    }
    else
    {
        CSimpleSerializeWithDefaults<CSerializeWriterXMLCPBin>* pWriterSerialize = static_cast<CSimpleSerializeWithDefaults<CSerializeWriterXMLCPBin>*>(pISerialize);
        pWriterSerialize->GetInnerImpl()->ValueByteArray("m_pSerializedData", m_pSerializedData, m_uSerializedDataSize);
    }
}

//////////////////////////////////////////////////////////////////////////
bool CBinarySerializedObject::FinishWriting(XMLCPB::CWriterInterface& Writer)
{
    return Writer.WriteAllIntoMemory(m_pSerializedData, m_uSerializedDataSize);
}

//////////////////////////////////////////////////////////////////////////
bool CBinarySerializedObject::PrepareReading(XMLCPB::CReaderInterface& Reader)
{
    bool bResult = false;
    if (!IsEmpty())
    {
        bResult = Reader.ReadBinaryMemory(m_pSerializedData, m_uSerializedDataSize);
    }
    return bResult;
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////
CBinarySerializeHelper::CBinarySerializeHelper()
    : m_nRefCount(1)
{
}

//////////////////////////////////////////////////////////////////////////
CBinarySerializeHelper::~CBinarySerializeHelper()
{
}

//////////////////////////////////////////////////////////////////////////
void CBinarySerializeHelper::GetMemoryUsage(ICrySizer* pSizer) const
{
    pSizer->Add(*this);
}

//////////////////////////////////////////////////////////////////////////
_smart_ptr<ISerializedObject> CBinarySerializeHelper::CreateSerializedObject(const char* szSection)
{
    assert(szSection && szSection[0]);
    return _smart_ptr<ISerializedObject>(new CBinarySerializedObject(szSection));
}

//////////////////////////////////////////////////////////////////////////
CBinarySerializedObject* CBinarySerializeHelper::GetBinarySerializedObject(ISerializedObject* pObject)
{
    assert(pObject);

    CBinarySerializedObject* pBinaryObject = NULL;
    if (pObject && CBinarySerializedObject::GUID == pObject->GetGUID())
    {
        pBinaryObject = static_cast<CBinarySerializedObject*>(pObject);
        assert(pBinaryObject);
    }

    return pBinaryObject;
}

//////////////////////////////////////////////////////////////////////////
bool CBinarySerializeHelper::Write(ISerializedObject* pObject, TSerializeFunc serializeFunc, void* pArgument /*= NULL*/)
{
    assert(pObject);

    bool bResult = false;

    if (CBinarySerializedObject* pBinaryObject = GetBinarySerializedObject(pObject))
    {
        XMLCPB::CWriterInterface Writer;
        Writer.Init("BSHelperWriter", NULL);

        XMLCPB::CNodeLiveWriterRef writeNode = Writer.GetRoot()->AddChildNode(pBinaryObject->GetSectionName());

        std::unique_ptr<CSerializeWriterXMLCPBin> pWriterXMLCPBin = std::make_unique<CSerializeWriterXMLCPBin>(writeNode, Writer);
        std::unique_ptr<ISerialize> pWriter = std::make_unique<CSimpleSerializeWithDefaults<CSerializeWriterXMLCPBin> >(*pWriterXMLCPBin);

        TSerialize stateWriter(pWriter.get());
        if (serializeFunc(stateWriter, pArgument))
        {
            bResult = pBinaryObject->FinishWriting(Writer);
        }
    }

    return bResult;
}

//////////////////////////////////////////////////////////////////////////
bool CBinarySerializeHelper::Read(ISerializedObject* pObject, TSerializeFunc serializeFunc, void* pArgument /*= NULL*/)
{
    assert(pObject);

    bool bResult = false;

    if (CBinarySerializedObject* pBinaryObject = GetBinarySerializedObject(pObject))
    {
        _smart_ptr<IGeneralMemoryHeap> pHeap = XMLCPB::CReader::CreateHeap();
        XMLCPB::CReaderInterface Reader(pHeap);
        if (pBinaryObject->PrepareReading(Reader))
        {
            XMLCPB::CNodeLiveReaderRef readNode = Reader.GetRoot()->GetChildNode(pBinaryObject->GetSectionName());
            assert(readNode.IsValid());

            std::unique_ptr<CSerializeReaderXMLCPBin> pReaderXMLCPBin = std::make_unique<CSerializeReaderXMLCPBin>(readNode, Reader);
            std::unique_ptr<ISerialize> pReader = std::make_unique<CSimpleSerializeWithDefaults<CSerializeReaderXMLCPBin> >(*pReaderXMLCPBin);

            TSerialize stateReader(pReader.get());
            if (serializeFunc(stateReader, pArgument))
            {
                bResult = (!pBinaryObject->IsEmpty());
            }
        }
    }

    return bResult;
}
