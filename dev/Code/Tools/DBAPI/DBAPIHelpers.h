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

#ifndef CRYINCLUDE_TOOLS_DBAPI_DBAPIHELPERS_H
#define CRYINCLUDE_TOOLS_DBAPI_DBAPIHELPERS_H
#pragma once


#include "DBTypes.h"
#include "IDBConnection.h"

class IDBStatement;
class IDBPreparedStatement;
class IDBResultSet;

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
#ifdef KDAB_MAC_PORT
            p->Close();
#endif
            DriverManager::DestroyConnection(p);
        }
    }

    void Release(IDBStatement* p)
    {
        if (p)
        {
#ifdef KDAB_MAC_PORT
            p->Close();
#endif
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
#ifdef KDAB_MAC_PORT
            p->Close();
#endif
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


// //
// Helper for executing queries and automatically managing transactions and resources.
// //

typedef int TDataSourceId;
typedef const byte* TStatementId;

class CQueryHelper
{
public:
    CQueryHelper(TDataSourceId datasourceId);
    CQueryHelper(IDBConnection* pConnection);
    CQueryHelper(const char* pcUrl);
    ~CQueryHelper();

    // Statements
    bool Execute(const char* pcStatement);

    // Prepared statements
    bool Prepare(const char* pcStatement);
    bool Execute();

    // Transaction management
    bool BeginTransaction(bool bCommitPrevious = true);
    bool Commit();
    bool Rollback();

    // Query result
    bool Next();
    IDBResultSet* GetCurrentResultSet() const;

    // Parameter binding
    bool SetNull(int nInd);
    bool SetInt(int nInd, int8 nVal);
    bool SetInt(int nInd, int16 nVal);
    bool SetInt(int nInd, int32 nVal);
    bool SetInt(int nInd, int64 nVal);
    bool SetInt(int nInd, uint8 nVal);
    bool SetInt(int nInd, uint16 nVal);
    bool SetInt(int nInd, uint32 nVal);
    bool SetInt(int nInd, uint64 nVal);
    bool SetFloat(int nInd, float fVal);
    bool SetDouble(int nInd, double dVal);
    bool SetString(int nInd, const char* pc);
    bool SetBinary(int nInd, void* pData, int nSize);
    bool SetDate(int nInd, time_t tVal);

    // Result binding by index
    bool IsNull(int nInd) const;
    bool GetInt(int nInd, int8& nVal) const;
    bool GetInt(int nInd, int16& nVal) const;
    bool GetInt(int nInd, int32& nVal) const;
    bool GetInt(int nInd, int64& nVal) const;
    bool GetInt(int nInd, uint8& nVal) const;
    bool GetInt(int nInd, uint16& nVal) const;
    bool GetInt(int nInd, uint32& nVal) const;
    bool GetInt(int nInd, uint64& nVal) const;
    bool GetFloat(int nInd, float& fVal) const;
    bool GetDouble(int nInd, double& dVal) const;
    bool GetString(int nInd, const char*& pc) const;
    bool GetBinary(int nInd, void*& pData, int& nSize) const;
    bool GetDate(int nInd, time_t& tVal) const;

    // Result binding by name
    bool IsNull(const char* pcCol) const;
    bool GetInt(const char* pcCol, int8& nVal) const;
    bool GetInt(const char* pcCol, int16& nVal) const;
    bool GetInt(const char* pcCol, int32& nVal) const;
    bool GetInt(const char* pcCol, int64& nVal) const;
    bool GetInt(const char* pcCol, uint8& nVal) const;
    bool GetInt(const char* pcCol, uint16& nVal) const;
    bool GetInt(const char* pcCol, uint32& nVal) const;
    bool GetInt(const char* pcCol, uint64& nVal) const;
    bool GetFloat(const char* pcCol, float& fVal) const;
    bool GetDouble(const char* pcCol, double& dVal) const;
    bool GetString(const char* pcCol, const char*& pc) const;
    bool GetBinary(const char* pcCol, void*& pData, int& nSize) const;
    bool GetDate(const char* pcCol, time_t& tVal) const;

    // Error handling
    EDBError GetLastError() const;
    const char* GetRawErrorMessage() const;

private:
    TDataSourceId m_DataSourceId;
    IDBConnection* m_pConnection;
    bool m_bCloseConnection;
    IDBStatement* m_pStatement;
    IDBPreparedStatement* m_pPrepStatement;
    IDBStatement* m_pResultStatement;
    IDBResultSet* m_pResultSet;
    bool m_bCommitted;
    mutable EDBError m_Error;
};

#endif // CRYINCLUDE_TOOLS_DBAPI_DBAPIHELPERS_H
