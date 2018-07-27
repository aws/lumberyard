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

#ifndef CRYINCLUDE_CRYACTION_IMETADATARECORDER_H
#define CRYINCLUDE_CRYACTION_IMETADATARECORDER_H
#pragma once

struct IMetadata
{
    static IMetadata* CreateInstance();
    void Delete();

    virtual void SetTag(uint32 tag) = 0;
    virtual bool SetValue(uint32 type, const uint8* data, uint8 size) = 0;
    virtual bool AddField(const IMetadata* metadata) = 0;
    virtual bool AddField(uint32 tag, uint32 type, const uint8* data, uint8 size) = 0;

    virtual uint32 GetTag() const = 0;
    virtual size_t GetNumFields() const = 0; // 0 means this is a basic typed value
    virtual const IMetadata* GetFieldByIndex(size_t i) const = 0;
    virtual uint32 GetValueType() const = 0;
    virtual uint8 GetValueSize() const = 0;
    virtual bool GetValue(uint8* data /*[out]*/, uint8* size /*[in|out]*/) const = 0;

    virtual IMetadata* Clone() const = 0;

    virtual void Reset() = 0;

protected:
    virtual ~IMetadata() {}
};

// This interface should be implemented by user of IMetadataRecorder.
struct IMetadataListener
{
    virtual ~IMetadataListener(){}
    virtual void OnData(const IMetadata* metadata) = 0;
};

// Records toplevel metadata - everything being recorded is added to the toplevel in a sequential manner.
struct IMetadataRecorder
{
    static IMetadataRecorder* CreateInstance();
    void Delete();

    virtual bool InitSave(const char* filename) = 0;
    virtual bool InitLoad(const char* filename) = 0;

    virtual void RecordIt(const IMetadata* metadata) = 0;
    virtual void Flush() = 0;

    virtual bool Playback(IMetadataListener* pListener) = 0;

    virtual void Reset() = 0;

protected:
    virtual ~IMetadataRecorder() {}
};

template<typename I>
class CSimpleAutoPtr
{
public:
    CSimpleAutoPtr() { m_pI = I::CreateInstance(); }
    ~CSimpleAutoPtr() { m_pI->Delete(); }
    I* operator->() const { return m_pI; }
    const I* get() const { return m_pI; }
private:
    CSimpleAutoPtr(const CSimpleAutoPtr& rhs);
    CSimpleAutoPtr& operator=(const CSimpleAutoPtr& rhs);
    I* m_pI;
};

typedef CSimpleAutoPtr<IMetadata> IMetadataPtr;
typedef CSimpleAutoPtr<IMetadataRecorder> IMetadataRecorderPtr;

#endif // CRYINCLUDE_CRYACTION_IMETADATARECORDER_H

