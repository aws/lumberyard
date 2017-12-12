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

// Description : This class initiates from a given string, and stores the hashed
//               Representation of that string
//               Plus: It can store a copy of the original string for debug purposes

#ifndef _HASHED_STRING_H_
#define _HASHED_STRING_H_

#include <CryCrc32.h>

#if !defined(_RELEASE)
    #define STORE_UNHASHED_STRING  //for debug purposes, it is often interesting to see from which string the hash was created
    #define CHECK_FOR_HASH_CONFLICTS  //will compare the strings if the hash-values are equal, and assert if the hash-values match but the strings dont.
#endif

class CHashedString
{
    static const uint32 INVALID_HASH = (uint32) - 1;

public:
    static const CHashedString& GetEmptyHashedString()
    {
        const static CHashedString emptyInstance;
        return emptyInstance;
    }

    CHashedString()
    {
        m_hash = INVALID_HASH;
    }

    explicit CHashedString(const uint32 hash)
    {
        m_hash = hash;
    }

    CHashedString(const char* pText)
#if defined(STORE_UNHASHED_STRING)
        : m_TextCopy(pText)
#endif
    {
        m_hash = CreateHash(pText);
    }

    CHashedString(const string& text)
#if defined(STORE_UNHASHED_STRING)
        : m_TextCopy(text)
#endif
    {
        m_hash = CreateHash(text.c_str());
    }

    void Clear()
    {
#if defined(STORE_UNHASHED_STRING)
        m_TextCopy.clear();
#endif
        m_hash = INVALID_HASH;
    }

    CHashedString& operator=(const char* pText)
    {
#if defined(STORE_UNHASHED_STRING)
        m_TextCopy = pText;
#endif
        m_hash = CreateHash(pText);
        return *this;
    }

    string GetText() const
    {
#if defined(STORE_UNHASHED_STRING)
        return m_TextCopy;
#else
        char buffer[16];
        sprintf(buffer, "0x%08x", m_hash);
        return buffer;
#endif
    }

    bool IsValid() const
    {
        return m_hash != INVALID_HASH;
    }

    uint32 GetHash() const
    {
        return m_hash;
    }

    bool operator==(const char* text) const
    {
#if defined(CHECK_FOR_HASH_CONFLICTS)
        if (m_hash == CreateHash(text))
        {
            assert(strcmp(m_TextCopy.c_str(), text) == 0);
            return true;
        }
        else
        {
            return false;
        }
#endif
        return m_hash == CreateHash(text);
    }

    bool operator==(const CHashedString& other) const
    {
#if defined(CHECK_FOR_HASH_CONFLICTS)
        if (m_hash == other.m_hash)
        {
            assert(m_TextCopy == other.m_TextCopy);
            return true;
        }
        else
        {
            return false;
        }
#endif
        return m_hash == other.m_hash;
    }

    bool operator!=(const CHashedString& other) const
    {
        return m_hash != other.m_hash;
    }

    bool operator<(const CHashedString& other) const
    {
        //remark: sorted by hash value, not by string. could be changed in "STORE_UNHASHED_STRING" builds if wanted
        return m_hash < other.m_hash;
    }

    bool operator>(const CHashedString& other) const
    {
        return m_hash > other.m_hash;
    }

    operator uint32() const
    {
        return m_hash;
    }

protected:

    static uint32 CreateHash(const char* pStringToHash)
    {
        if (pStringToHash == NULL)
        {
            return INVALID_HASH;
        }
        if (pStringToHash[0] == '0' && pStringToHash[1] == 'x')  //if the given string already starts with 0x..., we assume that it is already a hashCode, stored in a string. we therefore simply convert it back
        {
            return strtoul(pStringToHash + 2, 0, 16);
        }
        return CCrc32::Compute(pStringToHash);
    }

    uint32 m_hash;

#if defined(STORE_UNHASHED_STRING)
    string m_TextCopy;
#endif
};

#endif  //_HASHED_STRING_H_