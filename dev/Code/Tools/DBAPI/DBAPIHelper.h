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

#ifndef CRYINCLUDE_TOOLS_DBAPI_DBAPIHELPER_H
#define CRYINCLUDE_TOOLS_DBAPI_DBAPIHELPER_H
#pragma once


#include "IDBStatement.h"
#include "IDBResultSet.h"

// //
// Releasing resources pointer for DBAPI resources.
// It transfers ownership on assignment (like std::auto_ptr)
// //

template <class T>
struct DBAPIPtr
{
    mutable T* p;

    DBAPIPtr()
        : p(0) {}
    DBAPIPtr(const DBAPIPtr& other) { p = other.p; other.p; }
    DBAPIPtr(T* other) { p = other; }
    ~DBAPIPtr() { Release(p); }

    operator T*() const {
        return p;
    }
    operator const T*() const {
        return p;
    }
    T& operator*() const { return *p; }
    T* operator->() const { return p; }
    DBAPIPtr& operator=(T* other) { Release(p); p = other; return *this; }
    DBAPIPtr& operator=(const DBAPIPtr& other) { Release(p); p = other.p; other.p; return *this; }
    operator bool() const {
        return p != NULL;
    }
    bool operator !() const { return p == NULL; }

    void Release(IDBConnection* p)
    {
        if (p)
        {
            DriverManager::DestroyConnection(p);
        }
    }

    void Release(IDBStatement* p)
    {
        if (p)
        {
            IDBConnection* pCon = p->GetConnection();

            if (pCon)
            {
                pCon->DestroyStatement(p);
            }
        }
    }

    void Release(IDBResultSet* p)
    {
        if (p)
        {
            IDBStatement* pStatement = p->GetStatement();

            if (pStatement)
            {
                pStatement->CloseResultSet();
            }
        }
    }
};

typedef DBAPIPtr<IDBConnection> IDBConnectionPtr;
typedef DBAPIPtr<IDBStatement> IDBStatementPtr;
typedef DBAPIPtr<IDBResultSet> IDBResultSetPtr;

// //
// Scoped helper for transaction management. Rollbacks on exit if not committed.
// //

template <class T>
struct TAutoTransaction
{
    T* m_pConnection;
    bool m_bCommitted;

    TAutoTransaction(T* pConnection)
    {
        m_pConnection = pConnection;
        m_bCommitted = false;
        pConnection->SetAutoCommit(false);
    }

    bool Commit()
    {
        if (!m_bCommitted)
        {
            bool bSuccess = m_pConnection->Commit();
            m_pConnection->SetAutoCommit(true);
            m_bCommitted = true;
            return bSuccess;
        }

        return true;
    }

    bool Rollback()
    {
        if (m_pConnection)
        {
            bool bSuccess = m_pConnection->Rollback();
            m_pConnection->SetAutoCommit(true);
            m_pConnection;
            return bSuccess;
        }

        return true;
    }

    ~TAutoTransaction()
    {
        if (m_pConnection && !m_bCommitted)
        {
            m_pConnection->Rollback();
            m_pConnection->SetAutoCommit(true);
        }
    }
};

namespace DBAPIHelper
{
    bool Exec(IDBPreparedStatement* Statement);

    template<typename T1>
    bool Exec(IDBPreparedStatement* Statement, T1 arg1)
    {
        bool Result = false;
        // if( Statement->ParamCount() == 1 )
        Result = SetParam(Statement, 0, arg1);
        if (Result)
        {
            Result = Statement->Execute();
        }
        return Result;
    }

    template<typename T1, typename T2>
    bool Exec(IDBPreparedStatement* Statement, T1 arg1, T2 arg2)
    {
        bool Result = false;
        // if( Statement->ParamCount() == 2 )
        Result = SetParam(Statement, 0, arg1) &&
            SetParam(Statement, 1, arg2);
        if (Result)
        {
            Result = Statement->Execute();
        }
        return Result;
    }

    template<typename T1, typename T2, typename T3>
    bool Exec(IDBPreparedStatement* Statement, T1 arg1, T2 arg2, T3 arg3)
    {
        bool Result = SetParam(Statement, 0, arg1) &&
            SetParam(Statement, 1, arg2) &&
            SetParam(Statement, 2, arg3);
        if (Result)
        {
            Result = Statement->Execute();
        }
        return Result;
    }

    template<typename T1, typename T2, typename T3, typename T4>
    bool Exec(IDBPreparedStatement* Statement, T1 arg1, T2 arg2, T3 arg3, T4 arg4)
    {
        bool Result = SetParam(Statement, 0, arg1) &&
            SetParam(Statement, 1, arg2) &&
            SetParam(Statement, 2, arg3) &&
            SetParam(Statement, 3, arg4);
        if (Result)
        {
            Result = Statement->Execute();
        }
        return Result;
    }

    template<typename T1, typename T2, typename T3, typename T4, typename T5>
    bool Exec(IDBPreparedStatement* Statement, T1 arg1, T2 arg2, T3 arg3, T4 arg4, T5 arg5)
    {
        bool Result = SetParam(Statement, 0, arg1) &&
            SetParam(Statement, 1, arg2) &&
            SetParam(Statement, 2, arg3) &&
            SetParam(Statement, 3, arg4) &&
            SetParam(Statement, 4, arg5);
        if (Result)
        {
            Result = Statement->Execute();
        }
        return Result;
    }

    template<typename T1>
    bool Get(IDBResultSet* ResultSet, T1& arg1)
    {
        bool Result = false;
        if (ResultSet->GetColumnCount() == 1)
        {
            Result = GetValue(ResultSet, 0, arg1);
        }
        return Result;
    }

    template<typename T1, typename T2>
    bool Get(IDBResultSet* ResultSet, T1& arg1, T2& arg2)
    {
        bool Result = false;
        if (ResultSet->GetColumnCount() == 2)
        {
            Result = GetValue(ResultSet, 0, arg1) &&
                GetValue(ResultSet, 1, arg2);
        }
        return Result;
    }

    template<typename T1, typename T2, typename T3>
    bool Get(IDBResultSet* ResultSet, T1& arg1, T2& arg2, T3& arg3)
    {
        bool Result = false;
        if (ResultSet->GetColumnCount() == 3)
        {
            Result = GetValue(ResultSet, 0, arg1) &&
                GetValue(ResultSet, 1, arg2) &&
                GetValue(ResultSet, 2, arg3);
        }
        return Result;
    }

    template<typename T1, typename T2, typename T3, typename T4>
    bool Get(IDBResultSet* ResultSet, T1& arg1, T2& arg2, T3& arg3, T4& arg4)
    {
        bool Result = false;
        if (ResultSet->GetColumnCount() == 4)
        {
            Result = GetValue(ResultSet, 0, arg1) &&
                GetValue(ResultSet, 1, arg2) &&
                GetValue(ResultSet, 2, arg3) &&
                GetValue(ResultSet, 3, arg4);
        }
        return Result;
    }

    template<typename T1, typename T2, typename T3, typename T4, typename T5>
    bool Get(IDBResultSet* ResultSet, T1& arg1, T2& arg2, T3& arg3, T4& arg4, T5& arg5)
    {
        bool Result = false;
        if (ResultSet->GetColumnCount() == 5)
        {
            Result = GetValue(ResultSet, 0, arg1) &&
                GetValue(ResultSet, 1, arg2) &&
                GetValue(ResultSet, 2, arg3) &&
                GetValue(ResultSet, 3, arg4) &&
                GetValue(ResultSet, 4, arg5);
        }
        return Result;
    }

    template<typename T>
    bool SetParam(IDBPreparedStatement* Statement, unsigned int Index, T Value);

    template<>
    bool SetParam(IDBPreparedStatement* Statement, unsigned int Index, int8 Value);
    template<>
    bool SetParam(IDBPreparedStatement* Statement, unsigned int Index, int16 Value);
    template<>
    bool SetParam(IDBPreparedStatement* Statement, unsigned int Index, int32 Value);
    template<>
    bool SetParam(IDBPreparedStatement* Statement, unsigned int Index, int64 Value);
    template<>
    bool SetParam(IDBPreparedStatement* Statement, unsigned int Index, uint8 Value);
    template<>
    bool SetParam(IDBPreparedStatement* Statement, unsigned int Index, uint16 Value);
    template<>
    bool SetParam(IDBPreparedStatement* Statement, unsigned int Index, uint32 Value);
    template<>
    bool SetParam(IDBPreparedStatement* Statement, unsigned int Index, uint64 Value);
    template<>
    bool SetParam(IDBPreparedStatement* Statement, unsigned int Index, float Value);
    template<>
    bool SetParam(IDBPreparedStatement* Statement, unsigned int Index, double Value);
    template<>
    bool SetParam(IDBPreparedStatement* Statement, unsigned int Index, char* Value);
    template<>
    bool SetParam(IDBPreparedStatement* Statement, unsigned int Index, const char* Value);

    template<typename T>
    bool GetValue(IDBResultSet* ResultSet, unsigned int Index, T& Value);

    template<>
    bool GetValue(IDBResultSet* ResultSet, unsigned int Index, int8& Value);
    template<>
    bool GetValue(IDBResultSet* ResultSet, unsigned int Index, int16& Value);
    template<>
    bool GetValue(IDBResultSet* ResultSet, unsigned int Index, int32& Value);
    template<>
    bool GetValue(IDBResultSet* ResultSet, unsigned int Index, int64& Value);
    template<>
    bool GetValue(IDBResultSet* ResultSet, unsigned int Index, uint8& Value);
    template<>
    bool GetValue(IDBResultSet* ResultSet, unsigned int Index, uint16& Value);
    template<>
    bool GetValue(IDBResultSet* ResultSet, unsigned int Index, uint32& Value);
    template<>
    bool GetValue(IDBResultSet* ResultSet, unsigned int Index, uint64& Value);
    template<>
    bool GetValue(IDBResultSet* ResultSet, unsigned int Index, float& Value);
    template<>
    bool GetValue(IDBResultSet* ResultSet, unsigned int Index, double& Value);
    template<>
    bool GetValue(IDBResultSet* ResultSet, unsigned int Index, const char*& Value);
};

#endif // CRYINCLUDE_TOOLS_DBAPI_DBAPIHELPER_H
