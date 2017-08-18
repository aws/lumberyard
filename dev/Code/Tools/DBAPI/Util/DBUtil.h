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

#ifndef CRYINCLUDE_TOOLS_DBAPI_UTIL_DBUTIL_H
#define CRYINCLUDE_TOOLS_DBAPI_UTIL_DBUTIL_H
#pragma once

class StringStorage
{
public:
    StringStorage()
    {
    }

    StringStorage(const char* Str)
    {
        SetString(Str);
    }

    StringStorage& operator=(const char* Str)
    {
        SetString(Str);
        return *this;
    }

    const char* ToString() const
    {
        return Storage.empty() ? NULL : static_cast<const char*>(&Storage[0]);
    }

    size_t Length() const
    {
        return Storage.empty() ? 0 : Storage.size();
    }

    void Clear()
    {
        Storage.clear();
    }

    bool Empty() const
    {
        return Length() == 0;
    }

    void SetString(const char* Str)
    {
        if (Str == NULL)
        {
            Clear();
        }
        else
        {
            size_t Length = strlen(Str);
            std::vector<char> other(Str, Str + Length + 1);
            Storage.swap(other);
        }
    }

    friend class StringStorageCompareNoCase;
    class StringStorageCompareNoCase
        : public std::binary_function< const StringStorage&, const StringStorage&, bool>
    {
    public:
        bool operator()(const StringStorage&  Str1, const StringStorage& Str2) const
        {
            return std::lexicographical_compare(Str1.Storage.begin(), Str1.Storage.end(),
                Str2.Storage.begin(), Str2.Storage.end());
        }
    };
protected:
    std::vector<char>   Storage;
};

class CNameToIndexMap
{
public:
    int Find(const char* Name) const
    {
        NameToIndexMapType::const_iterator itr = m_NameToIndexMap.find(StringStorage(Name));

        if (itr != m_NameToIndexMap.end())
        {
            return itr->second;
        }
        return -1;
    }

    bool Add(const char* Name, int Index)
    {
        if (Find(Name) < 0)
        {
            m_NameToIndexMap.insert(std::make_pair(Name, Index));
            return true;
        }
        return false;
    }

    void Reset()
    {
        m_NameToIndexMap.clear();
    }
protected:
    typedef std::map< StringStorage, int, StringStorage::StringStorageCompareNoCase> NameToIndexMapType;
    NameToIndexMapType   m_NameToIndexMap;
};

#include "IDBStatement.h"
class CDBStatementSet
{
public:
    CDBStatementSet() {}
    ~CDBStatementSet()
    {
        DeleteAll();
    }

    bool Add(IDBStatement* Stmt)
    {
        DBStatemetSetType::iterator itr = m_DBStatementSet.find(Stmt);
        if (itr != m_DBStatementSet.end())
        {
            return false;
        }
        m_DBStatementSet.insert(Stmt);
        return true;
    }

    bool Delete(IDBStatement* Stmt)
    {
        DBStatemetSetType::iterator itr = m_DBStatementSet.find(Stmt);
        if (itr != m_DBStatementSet.end())
        {
            m_DBStatementSet.erase(itr);
            return true;
        }
        return false;
    }

    void DeleteAll()
    {
        DBStatemetSetType::iterator itr = m_DBStatementSet.begin();
        for (; itr != m_DBStatementSet.end(); ++itr)
        {
            delete *itr;
        }
        m_DBStatementSet.clear();
    }

protected:
    typedef std::set<IDBStatement*> DBStatemetSetType;
    DBStatemetSetType   m_DBStatementSet;
};

#define DEFINE_COLUMNBASED_FUNCTION_0(CLASS, FUNCTION)            \
    bool CLASS :: FUNCTION##ByName (const char* ColumnName) const \
    {                                                             \
        int Index = FindColumn(ColumnName);                       \
        if (Index < 0)                                            \
        {                                                         \
            m_Connection->SetLastError(eDB_INVALID_COLUMN_NAME);  \
            return false;                                         \
        }                                                         \
        return FUNCTION##ByIndex(Index);                          \
    }

#define DEFINE_COLUMNBASED_FUNCTION_1(CLASS, FUNCTION, TYPE1)                   \
    bool CLASS :: FUNCTION##ByName (const char* ColumnName, TYPE1 & Arg1) const \
    {                                                                           \
        int Index = FindColumn(ColumnName);                                     \
        if (Index < 0)                                                          \
        {                                                                       \
            m_Connection->SetLastError(eDB_INVALID_COLUMN_NAME);                \
            return false;                                                       \
        }                                                                       \
        return FUNCTION##ByIndex(Index, Arg1);                                  \
    }

#define DEFINE_COLUMNBASED_FUNCTION_2(CLASS, FUNCTION, TYPE1, TYPE2)                         \
    bool CLASS:: FUNCTION##ByName (const char* ColumnName, TYPE1 & Arg1, TYPE2 & Arg2) const \
    {                                                                                        \
        int Index = FindColumn(ColumnName);                                                  \
        if (Index < 0)                                                                       \
        {                                                                                    \
            m_Connection->SetLastError(eDB_INVALID_COLUMN_NAME);                             \
            return false;                                                                    \
        }                                                                                    \
        return FUNCTION##ByIndex(Index, Arg1, Arg2);                                         \
    }

#define CHECK_CLOSED_RESULTSET(ret)                       \
    if (IsClosed())                                       \
    {                                                     \
        m_Connection->SetLastError(eDB_CLOSED_RESULTSET); \
        return ret;                                       \
    }

#define CHECK_INVALID_INDEX(Index, Max)                       \
    if (!(Index < Max))                                       \
    {                                                         \
        m_Connection->SetLastError(eDB_INVALID_COLUMN_INDEX); \
        return false;                                         \
    }

#define CHECK_IS_NULL(Value)                           \
    if (!Value)                                        \
    {                                                  \
        m_Connection->SetLastError(eDB_VALUE_IS_NULL); \
        return false;                                  \
    }

#endif // CRYINCLUDE_TOOLS_DBAPI_UTIL_DBUTIL_H
