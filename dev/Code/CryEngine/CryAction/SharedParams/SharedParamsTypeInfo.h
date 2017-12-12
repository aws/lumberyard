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

// Description : Shared parameters type information.


#ifndef CRYINCLUDE_CRYACTION_SHAREDPARAMS_SHAREDPARAMSTYPEINFO_H
#define CRYINCLUDE_CRYACTION_SHAREDPARAMS_SHAREDPARAMSTYPEINFO_H
#pragma once

#include <StringUtils.h>

#ifdef _DEBUG
#define DEBUG_SHARED_PARAMS_TYPE_INFO   1
#else
#define DEBUG_SHARED_PARAMS_TYPE_INFO   0
#endif //_DEBUG

////////////////////////////////////////////////////////////////////////////////////////////////////
// Shared parameters type information class.
////////////////////////////////////////////////////////////////////////////////////////////////////
class CSharedParamsTypeInfo
{
public:

    CSharedParamsTypeInfo(size_t size, const char* pName, const char* pFileName, uint32 line)
        : m_size(size)
    {
        if (pName)
        {
            const size_t length = strlen(pName);
            size_t pos = 0;

            if (length > sizeof(m_name) - 1)
            {
                pos = length - (sizeof(m_name) - 1);
            }

            cry_strcpy(m_name, pName + pos);
        }
        else
        {
            m_name[0] = '\0';
        }

        if (pFileName)
        {
            const size_t length = strlen(pFileName);
            size_t pos = 0;

            if (length > sizeof(m_fileName) - 1)
            {
                pos = length - (sizeof(m_fileName) - 1);
            }

            cry_strcpy(m_fileName, pFileName + pos);
        }
        else
        {
            m_fileName[0] = '\0';
        }

        m_line = line;

        CryFixedStringT<256> temp;

        temp.Format("%d%s%s%d", size, m_name, m_fileName, m_line);

        m_uniqueId = CryStringUtils::CalculateHash(temp.c_str());
    }

    inline size_t GetSize() const
    {
        return m_size;
    }

    inline const char* GetName() const
    {
        return m_name;
    }

    inline const char* GetFileName() const
    {
        return m_fileName;
    }

    inline uint32 GetLine() const
    {
        return m_line;
    }

    inline uint32 GetUniqueId() const
    {
        return m_uniqueId;
    }

    inline bool operator == (const CSharedParamsTypeInfo& right) const
    {
#if DEBUG_SHARED_PARAMS_TYPE_INFO
        if (m_uniqueId == right.m_uniqueId)
        {
            CRY_ASSERT(m_size == right.m_size);

            CRY_ASSERT(!strcmp(m_name, right.m_name));

            CRY_ASSERT(!strcmp(m_fileName, right.m_fileName));

            CRY_ASSERT(m_line == right.m_line);
        }
#endif //DEBUG_SHARED_PARAMS_TYPE_INFO

        return m_uniqueId == right.m_uniqueId;
    }

    inline bool operator != (const CSharedParamsTypeInfo& right) const
    {
#if DEBUG_SHARED_PARAMS_TYPE_INFO
        if (m_uniqueId == right.m_uniqueId)
        {
            CRY_ASSERT(m_size == right.m_size);

            CRY_ASSERT(!strcmp(m_name, right.m_name));

            CRY_ASSERT(!strcmp(m_fileName, right.m_fileName));

            CRY_ASSERT(m_line == right.m_line);
        }
#endif //DEBUG_SHARED_PARAMS_TYPE_INFO

        return m_uniqueId != right.m_uniqueId;
    }

private:

    size_t m_size;

    char m_name[64];
    char m_fileName[64];

    uint32 m_line;
    uint32 m_uniqueId;
};

#undef DEBUG_SHARED_PARAMS_TYPE_INFO

#endif // CRYINCLUDE_CRYACTION_SHAREDPARAMS_SHAREDPARAMSTYPEINFO_H
