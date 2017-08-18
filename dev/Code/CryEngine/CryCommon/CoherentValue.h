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

// Description : a value which can tell whether it's changed


#ifndef CRYINCLUDE_CRYCOMMON_COHERENTVALUE_H
#define CRYINCLUDE_CRYCOMMON_COHERENTVALUE_H
#pragma once

#include <IScriptSystem.h> // <> required for Interfuscator

template <typename T>
class CCoherentValue
{
public:
    ILINE CCoherentValue()
        : m_dirty(false)
    {
    }

    ILINE CCoherentValue(const T& val)
        : m_val(val)
        , m_dirty(true)
    {
    }

    ILINE CCoherentValue(const CCoherentValue& other)
    {
        if (m_val != other.m_val)
        {
            m_dirty = true;
        }
        m_val = other.m_val;
    }

    ILINE CCoherentValue& operator=(const CCoherentValue& rhs)
    {
        if (this != &rhs)
        {
            if (m_val != rhs.m_val)
            {
                m_dirty = true;
            }
            m_val = rhs.m_val;
        }
        return *this;
    }

    ILINE CCoherentValue operator+=(const CCoherentValue& rhs)
    {
        m_dirty = true;
        m_val += rhs.m_val;

        return *this;
    }

    ILINE bool IsDirty() const
    {
        return m_dirty;
    }

    ILINE void Clear()
    {
        m_dirty = false;
    }

    ILINE const T& Value() const
    {
        return m_val;
    }

    ILINE void SetDirtyValue(CScriptSetGetChain& chain, const char* name)
    {
        if (IsDirty())
        {
            chain.SetValue(name, m_val);
            Clear();
        }
    }

    ILINE void Serialize(TSerialize ser, const char* name)
    {
        ser.Value(name, m_val);
        if (ser.IsReading())
        {
            m_dirty = true;
        }
    }

    ILINE operator T() const
    {
        return m_val;
    }
private:
    T           m_val;
    bool    m_dirty;
};

#endif // CRYINCLUDE_CRYCOMMON_COHERENTVALUE_H
