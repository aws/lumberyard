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


#ifndef CRYINCLUDE_CRYACTION_SERIALIZATION_XMLCPBIN_BINARYSERIALIZEHELPER_H
#define CRYINCLUDE_CRYACTION_SERIALIZATION_XMLCPBIN_BINARYSERIALIZEHELPER_H
#pragma once

#include "ISerializeHelper.h"

#include "Serialization/SerializeWriterXMLCPBin.h"
#include "Serialization/SerializeReaderXMLCPBin.h"
#include "Serialization/XMLCPBin/Writer/XMLCPB_WriterInterface.h"
#include "Serialization/XMLCPBin/Reader/XMLCPB_ReaderInterface.h"

class CBinarySerializedObject
    : public ISerializedObject
{
public:
    CBinarySerializedObject(const char* szSection);
    virtual ~CBinarySerializedObject();

    enum
    {
        GUID = 0xBDE84A9A
    };
    virtual uint32 GetGUID() const { return GUID; }
    virtual void GetMemoryUsage(ICrySizer* pSizer) const;

    virtual void AddRef() { ++m_nRefCount; }
    virtual void Release()
    {
        if (--m_nRefCount <= 0)
        {
            delete this;
        }
    }

    virtual bool IsEmpty() const;
    virtual void Reset();
    virtual void Serialize(TSerialize& serialize);

    bool FinishWriting(XMLCPB::CWriterInterface& Writer);
    bool PrepareReading(XMLCPB::CReaderInterface& Reader);
    const char* GetSectionName() const { return m_sSection.c_str(); }

private:
    void FreeData();

    string m_sSection;

    int m_nRefCount;
    uint32 m_uSerializedDataSize;
    uint8* m_pSerializedData;
};

class CBinarySerializeHelper
    : public ISerializeHelper
{
public:
    CBinarySerializeHelper();
    virtual ~CBinarySerializeHelper();

    virtual void GetMemoryUsage(ICrySizer* pSizer) const;

    virtual void AddRef() { ++m_nRefCount; }
    virtual void Release()
    {
        if (--m_nRefCount <= 0)
        {
            delete this;
        }
    }

    virtual _smart_ptr<ISerializedObject> CreateSerializedObject(const char* szSection);
    virtual bool Write(ISerializedObject* pObject, TSerializeFunc serializeFunc, void* pArgument = NULL);
    virtual bool Read(ISerializedObject* pObject, TSerializeFunc serializeFunc, void* pArgument = NULL);

private:
    static CBinarySerializedObject* GetBinarySerializedObject(ISerializedObject* pObject);

private:
    int m_nRefCount;
};

#endif // CRYINCLUDE_CRYACTION_SERIALIZATION_XMLCPBIN_BINARYSERIALIZEHELPER_H
