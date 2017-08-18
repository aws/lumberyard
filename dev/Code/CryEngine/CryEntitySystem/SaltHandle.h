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

#ifndef CRYINCLUDE_CRYENTITYSYSTEM_SALTHANDLE_H
#define CRYINCLUDE_CRYENTITYSYSTEM_SALTHANDLE_H
#pragma once


// handle can be ==0 nil, or !=0 to store a reference to an object
// this object can be valid or invalid
template <class TSalt = unsigned short, class TIndex = unsigned short>
class CSaltHandle
{
public:
    // default constructor (set to invalid handle)
    CSaltHandle()
    {
        SetNil();
    }

    // constructor
    CSaltHandle(TSalt Salt, TIndex Index)
        : m_Index(Index)
        , m_Salt(Salt)
    {
    }

    // comparison operator
    bool operator==(const CSaltHandle<TSalt, TIndex>& rhs) const
    {
        return m_Salt == rhs.m_Salt && m_Index == rhs.m_Index;
    }

    // comparison operator
    bool operator!=(const CSaltHandle<TSalt, TIndex>& rhs) const
    {
        return !(*this == rhs);
    }

    // conversion to bool
    // e.g. if(id){ ..nil.. } else { ..valid or not valid.. }
    operator bool() const
    {
        return m_Salt != 0 || m_Index != 0;
    }

    // useful for logging/debugging
    // and getting a small index to cache data for an entity
    // (check with GetSalt() if the cache element is still valid)
    TIndex GetIndex() const
    {
        return m_Index;
    }

    // used by the SaltBufferArray<>
    TSalt GetSalt() const
    {
        return m_Salt;
    }

private: // --------------------------------------------------------------

    TSalt                                       m_Salt;                                 // 1.. is counting up on every remove, should never wrap around, 0 means invlid handle
    TIndex                                  m_Index;                                // Index in CSaltBufferArray

    // handle can be nil, or store a reference to an object
    void SetNil()
    {
        m_Salt = 0;
        m_Index = 0;
    }
};


#endif // CRYINCLUDE_CRYENTITYSYSTEM_SALTHANDLE_H
