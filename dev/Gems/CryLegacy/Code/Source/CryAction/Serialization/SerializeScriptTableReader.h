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

#ifndef CRYINCLUDE_CRYACTION_SERIALIZATION_SERIALIZESCRIPTTABLEREADER_H
#define CRYINCLUDE_CRYACTION_SERIALIZATION_SERIALIZESCRIPTTABLEREADER_H
#pragma once

#include "SimpleSerialize.h"
#include <stack>

class CSerializeScriptTableReaderImpl
    : public CSimpleSerializeImpl<true, eST_Script>
{
public:
    CSerializeScriptTableReaderImpl(SmartScriptTable tbl);

    template <class T>
    void Value(const char* name, T& value)
    {
        IScriptTable* pTbl = CurTable();
        if (pTbl)
        {
            if (pTbl->HaveValue(name))
            {
                if (!pTbl->GetValue(name, value))
                {
                    Failed();
                    GameWarning("Failed to read %s", name);
                }
            }
        }
    }

    void Value(const char* name, EntityId& value);

    void Value(const char* name, SNetObjectID& value)
    {
        CRY_ASSERT(false);
    }

    void Value(const char* name, XmlNodeRef& value)
    {
        CRY_ASSERT(false);
    }

    void Value(const char* name, Quat& value);
    void Value(const char* name, ScriptAnyValue& value);
    void Value(const char* name, SSerializeString& value);
    void Value(const char* name, uint16& value);
    void Value(const char* name, uint64& value);
    void Value(const char* name, int16& value);
    void Value(const char* name, int64& value);
    void Value(const char* name, uint8& value);
    void Value(const char* name, int8& value);
    void Value(const char* name, Vec2& value);
    void Value(const char* name, CTimeValue& value);

    template <class T, class P>
    void Value(const char* name, T& value, const P& p)
    {
        Value(name, value);
    }

    bool BeginGroup(const char* szName);
    bool BeginOptionalGroup(const char* szName, bool cond) { return false; }
    void EndGroup();

private:
    template <class T, class U>
    void NumValue(const char* name, U& value);

    int m_nSkip;
    std::stack<SmartScriptTable> m_tables;

    IScriptTable* CurTable()
    {
        if (m_nSkip)
        {
            return 0;
        }
        else
        {
            return m_tables.top();
        }
    }
};

#endif // CRYINCLUDE_CRYACTION_SERIALIZATION_SERIALIZESCRIPTTABLEREADER_H
