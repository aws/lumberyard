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

#ifndef CRYINCLUDE_CRYCOMMON_XMLATTRREADER_H
#define CRYINCLUDE_CRYCOMMON_XMLATTRREADER_H
#pragma once

template <typename T>
class CXMLAttrReader
{
    typedef std::pair<const char*, T> TRecord;
    std::vector<TRecord> m_dictionary;

public:
    void Reserve(const size_t elementCount)
    {
        m_dictionary.reserve(elementCount);
    }

    void Add(const char* szKey, T tValue) { m_dictionary.push_back(TRecord(szKey, tValue)); }

    bool Get(const XmlNodeRef& node, const char* szAttrName, T& tValue, bool bMandatory = false)
    {
        const char* szKey = node->getAttr(szAttrName);
        for (typename std::vector<TRecord>::iterator it = m_dictionary.begin(), end = m_dictionary.end(); it != end; ++it)
        {
            if (!azstricmp(it->first, szKey))
            {
                tValue = it->second;
                return true;
            }
        }

        if (bMandatory)
        {
            CryWarning(VALIDATOR_MODULE_UNKNOWN, VALIDATOR_WARNING, "Unable to get mandatory attribute '%s' of node '%s' at line %d.",
                szAttrName, node->getTag(), node->getLine());
        }

        return false;
    }

    const char* GetFirstRecordName(const T value)
    {
        for (typename std::vector<TRecord>::iterator it = m_dictionary.begin(), end = m_dictionary.end(); it != end; ++it)
        {
            if (it->second == value)
            {
                return it->first;
            }
        }

        return NULL;
    }

    const T* GetFirstValue(const char* name)
    {
        for (typename std::vector<TRecord>::iterator it = m_dictionary.begin(), end = m_dictionary.end(); it != end; ++it)
        {
            if (!azstricmp(it->first, name))
            {
                return &it->second;
            }
        }

        return NULL;
    }
};

#endif // CRYINCLUDE_CRYCOMMON_XMLATTRREADER_H
