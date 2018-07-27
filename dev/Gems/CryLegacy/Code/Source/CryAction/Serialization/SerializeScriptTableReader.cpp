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
#include "SerializeScriptTableReader.h"

CSerializeScriptTableReaderImpl::CSerializeScriptTableReaderImpl(SmartScriptTable tbl)
{
    m_nSkip = 0;
    m_tables.push(tbl);
}

template <class T, class U>
void CSerializeScriptTableReaderImpl::NumValue(const char* name, U& value)
{
    T temp;
    IScriptTable* pTbl = CurTable();
    if (pTbl)
    {
        if (pTbl->HaveValue(name))
        {
            if (!pTbl->GetValue(name, temp))
            {
                Failed();
                GameWarning("Failed to read %s", name);
            }
            else
            {
                value = temp;
            }
        }
    }
}

void CSerializeScriptTableReaderImpl::Value(const char* name, EntityId& value)
{
    // values of type entity id might be an unsigned int, or might be a script handle
    IScriptTable* pTbl = CurTable();
    if (pTbl)
    {
        if (pTbl->HaveValue(name))
        {
            if (!pTbl->GetValue(name, value))
            {
                ScriptHandle temp;
                if (!pTbl->GetValue(name, temp))
                {
                    Failed();
                    GameWarning("Failed to read %s", name);
                }
                else
                {
                    value = (EntityId)temp.n;
                }
            }
        }
    }
}

void CSerializeScriptTableReaderImpl::Value(const char* name, CTimeValue& value)
{
    NumValue<float>(name, value);
}

void CSerializeScriptTableReaderImpl::Value(const char* name, Vec2& value)
{
    SmartScriptTable temp;
    IScriptTable* pTbl = CurTable();
    Vec2 tempValue;
    if (pTbl)
    {
        if (pTbl->HaveValue(name))
        {
            if (!pTbl->GetValue(name, temp) || !temp->GetValue("x", tempValue.x) || !temp->GetValue("y", tempValue.y))
            {
                Failed();
                GameWarning("Failed to read %s", name);
            }
            else
            {
                value = tempValue;
            }
        }
    }
}

void CSerializeScriptTableReaderImpl::Value(const char* name, Quat& value)
{
    SmartScriptTable temp;
    IScriptTable* pTbl = CurTable();
    Quat tempValue;
    if (pTbl)
    {
        if (pTbl->HaveValue(name))
        {
            if (!pTbl->GetValue(name, temp) || !temp->GetValue("x", tempValue.v.x) || !temp->GetValue("y", tempValue.v.y) || !temp->GetValue("z", tempValue.v.z) || !temp->GetValue("w", tempValue.w))
            {
                Failed();
                GameWarning("Failed to read %s", name);
            }
            else
            {
                value = tempValue;
            }
        }
    }
}

void CSerializeScriptTableReaderImpl::Value(const char* name, ScriptAnyValue& value)
{
    ScriptAnyValue temp;
    IScriptTable* pTbl = CurTable();
    if (pTbl)
    {
        if (pTbl->GetValueAny(name, temp))
        {
            if (temp.type != ANY_TNIL)
            {
                value = temp;
            }
        }
    }
}

void CSerializeScriptTableReaderImpl::Value(const char* name, uint8& value)
{
    NumValue<int>(name, value);
}

void CSerializeScriptTableReaderImpl::Value(const char* name, int8& value)
{
    NumValue<int>(name, value);
}

void CSerializeScriptTableReaderImpl::Value(const char* name, uint16& value)
{
    NumValue<int>(name, value);
}

void CSerializeScriptTableReaderImpl::Value(const char* name, int16& value)
{
    NumValue<int>(name, value);
}

void CSerializeScriptTableReaderImpl::Value(const char* name, uint64& value)
{
    NumValue<int>(name, value);
}

void CSerializeScriptTableReaderImpl::Value(const char* name, int64& value)
{
    NumValue<int>(name, value);
}

void CSerializeScriptTableReaderImpl::Value(const char* name, SSerializeString& str)
{
    const char* tempstr = str.c_str();
    NumValue<const char*>(name, tempstr);
}

bool CSerializeScriptTableReaderImpl::BeginGroup(const char* szName)
{
    IScriptTable* pTbl = CurTable();
    if (!pTbl)
    {
        m_nSkip++;
    }
    else
    {
        ScriptAnyValue val;
        pTbl->GetValueAny(szName, val);
        switch (val.type)
        {
        case ANY_TTABLE:
            m_tables.push(val.table);
            break;
        default:
            GameWarning("Expected %s to be a table, but it wasn't", szName);
            Failed();
        case ANY_TNIL:
            m_nSkip++;
            break;
        }
    }
    return true;
}

void CSerializeScriptTableReaderImpl::EndGroup()
{
    if (m_nSkip)
    {
        --m_nSkip;
    }
    else
    {
        m_tables.pop();
    }
}
