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

#ifndef CRYINCLUDE_CRYACTION_SERIALIZATION_SERIALIZESCRIPTTABLEWRITER_H
#define CRYINCLUDE_CRYACTION_SERIALIZATION_SERIALIZESCRIPTTABLEWRITER_H
#pragma once

#include "SimpleSerialize.h"
#include <stack>

class CSerializeScriptTableWriterImpl
    : public CSimpleSerializeImpl<false, eST_Script>
{
public:
    CSerializeScriptTableWriterImpl(SmartScriptTable tbl = SmartScriptTable());

    template <class T>
    void Value(const char* name, const T& value)
    {
        IScriptTable* pTbl = CurTable();
        pTbl->SetValue(name, value);
    }

    void Value(const char* name, SNetObjectID value)
    {
        CRY_ASSERT(false);
    }

    void Value(const char* name, XmlNodeRef ref)
    {
        CRY_ASSERT(false);
    }

    void Value(const char* name, Vec3 value);
    void Value(const char* name, Vec2 value);
    void Value(const char* name, const SSerializeString& value);
    void Value(const char* name, int64 value);
    void Value(const char* name, Quat value);
    void Value(const char* name, uint64 value);
    void Value(const char* name, CTimeValue value);

    template <class T, class P>
    void Value(const char* name, T& value, const P& p)
    {
        Value(name, value);
    }

    bool BeginGroup(const char* szName);
    bool BeginOptionalGroup(const char* szName, bool cond) { return false; }
    void EndGroup();

private:
    std::stack<SmartScriptTable> m_tables;
    IScriptTable* CurTable() { return m_tables.top().GetPtr(); }
    SmartScriptTable ReuseTable(const char* name);

    IScriptSystem* m_pSS;
};

#endif // CRYINCLUDE_CRYACTION_SERIALIZATION_SERIALIZESCRIPTTABLEWRITER_H
