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

// Description : Interface to use serialization helpers


#ifndef CRYINCLUDE_CRYCOMMON_ISERIALIZEHELPER_H
#define CRYINCLUDE_CRYCOMMON_ISERIALIZEHELPER_H
#pragma once


struct ISerializedObject
{
    // <interfuscator:shuffle>
    virtual ~ISerializedObject() {}
    virtual uint32 GetGUID() const = 0;
    virtual void GetMemoryUsage(ICrySizer* pSizer) const = 0;

    virtual void AddRef() = 0;
    virtual void Release() = 0;

    // IsEmpty
    //  Returns True if object contains no serialized data
    virtual bool IsEmpty() const = 0;

    // Reset
    //  Resets the serialized object to an initial (empty) state
    virtual void Reset() = 0;

    // Serialize
    //  Call to inject the serialized data into another TSerialize object
    // Arguments:
    //  TSerialize &serialize - The TSerialize object to use
    virtual void Serialize(TSerialize& serialize) = 0;
    // </interfuscator:shuffle>
};

struct ISerializeHelper
{
    typedef bool (* TSerializeFunc)(TSerialize serialize, void* pArgument);

    // <interfuscator:shuffle>
    virtual ~ISerializeHelper() {}
    virtual void GetMemoryUsage(ICrySizer* pSizer) const = 0;

    virtual void AddRef() = 0;
    virtual void Release() = 0;

    // CreateSerializedObject
    //  Returns an ISerializedObject to be used with this helper
    // Arguments:
    //  const char* szSection - Name of the serialized object's section
    virtual _smart_ptr<ISerializedObject> CreateSerializedObject(const char* szSection) = 0;

    // Write
    //  Begins the writing process using the supplied functor
    // Arguments:
    //  ISerializedObject *pObject - Serialization object to write with
    //  TSerializeFunc serializeFunc - Functor called to supply the serialization logic
    //  void *pArgument - Optional argument passed in to functor.
    // Returns:
    //  True if writing occurred with given serialization object
    virtual bool Write(ISerializedObject* pObject, TSerializeFunc serializeFunc, void* pArgument = NULL) = 0;

    // Read
    //  Begins the reading process using the supplied functor
    // Arguments:
    //  ISerializedObject *pObject - Serialization object to read from
    //  TSerializeFunc serializeFunc - Functor called to supply the serialization logic
    //  void *pArgument - Optional argument passed in to functor.
    // Returns:
    //  True if writing occurred with given serialization object
    virtual bool Read(ISerializedObject* pObject, TSerializeFunc serializeFunc, void* pArgument = NULL) = 0;
    // </interfuscator:shuffle>
};

#endif // CRYINCLUDE_CRYCOMMON_ISERIALIZEHELPER_H
