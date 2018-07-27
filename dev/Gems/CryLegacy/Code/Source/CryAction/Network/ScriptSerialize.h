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

// Description : Helper classes for implementing serialization in script

#ifndef CRYINCLUDE_CRYACTION_NETWORK_SCRIPTSERIALIZE_H
#define CRYINCLUDE_CRYACTION_NETWORK_SCRIPTSERIALIZE_H
#pragma once

#include <vector>

enum EScriptSerializeType
{
    eSST_Bool = 'B',
    eSST_Float = 'f',
    eSST_Int8 = 'b',
    eSST_Int16 = 's',
    eSST_Int32 = 'i',
    eSST_String = 'S',
    eSST_EntityId = 'E',
    eSST_Void = '0',
    eSST_Vec3 = 'V',
    eSST_StringTable = 'T'
};

class CScriptSerialize
{
public:
    bool ReadValue(const char* name, EScriptSerializeType type, TSerialize, IScriptTable*);
    bool WriteValue(const char* name, EScriptSerializeType type, TSerialize, IScriptTable*);

private:
    string m_tempString;
};

class CScriptSerializeAny
{
public:
    CScriptSerializeAny()
        : m_type(eSST_Void) {}
    CScriptSerializeAny(EScriptSerializeType type);
    CScriptSerializeAny(const CScriptSerializeAny&);
    ~CScriptSerializeAny();
    CScriptSerializeAny& operator=(const CScriptSerializeAny&);

    void SerializeWith(TSerialize, const char* name = 0);
    bool SetFromFunction(IFunctionHandler* pFH, int param);
    void PushFuncParam(IScriptSystem* pSS) const;
    bool CopyFromTable(SmartScriptTable& tbl, const char* name);
    void CopyToTable(SmartScriptTable& tbl, const char* name);
    string DebugInfo() const;

private:
    static const size_t BUFFER_SIZE = sizeof(string) > sizeof(Vec3) ? sizeof(string) : sizeof(Vec3);
    char m_buffer[BUFFER_SIZE];
    EScriptSerializeType m_type;

    template <class T>
    T* Ptr()
    {
        CRY_ASSERT(BUFFER_SIZE >= sizeof(T));
        return reinterpret_cast<T*>(m_buffer);
    }
    template <class T>
    const T* Ptr() const
    {
        CRY_ASSERT(BUFFER_SIZE >= sizeof(T));
        return reinterpret_cast<const T*>(m_buffer);
    }
};

// this class helps provide a bridge between script and ISerialize
class CScriptRMISerialize
    : public ISerializable
{
public:
    CScriptRMISerialize(const char* format);

    void SetFormat(const char* format);
    void SerializeWith(TSerialize);
    bool SetFromFunction(IFunctionHandler* pFH, int firstParam);
    void PushFunctionParams(IScriptSystem* pSS);
    string DebugInfo();

private:
    typedef std::vector<CScriptSerializeAny> TValueVec;
    TValueVec m_values;
};

#endif // CRYINCLUDE_CRYACTION_NETWORK_SCRIPTSERIALIZE_H
