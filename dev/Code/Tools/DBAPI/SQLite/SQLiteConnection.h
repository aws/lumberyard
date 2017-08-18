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

#ifndef CRYINCLUDE_TOOLS_DBAPI_SQLITE_SQLITECONNECTION_H
#define CRYINCLUDE_TOOLS_DBAPI_SQLITE_SQLITECONNECTION_H
#pragma once


#include "IDBConnection.h"
#include "Util/DBUtil.h"


struct sqlite3;
struct sqlite3_stmt;
class CSQLiteConnection
    : public IDBConnection
{
public:
    CSQLiteConnection();

    virtual ~CSQLiteConnection();

    virtual unsigned long ClientVersion() const;

    virtual unsigned long ServerVersion() const;

    virtual bool Ping();


    virtual bool Commit();

    virtual bool Rollback();

    virtual bool GetAutoCommit() const;

    virtual bool SetAutoCommit(bool bAutoCommit);

    virtual IDBStatement*           CreateStatement();

    virtual IDBPreparedStatement*   CreatePreparedStatement(const char* Query);

    virtual IDBCallableStatement*   CreateCallableStatement(const char* Query);

    virtual bool                    DestroyStatement(IDBStatement* DBStatement);

    virtual EDBError                GetLastError() const;

    virtual const char*             GetRawErrorMessage() const;

    virtual void                    SetLastError(EDBError Error);

    virtual void                    SetSQLiteLastError(unsigned int SQLiteError, const char* ErrorMessage = NULL);

    virtual bool                    IsLocal() const { return true; }

    // additional interfaces
    bool Connect(const char* ConnectionString, const char* LocalPath);

    bool IsClosed() const { return m_Connection == NULL; }

    bool Close();

    sqlite3* GetRawHandle() const { return m_Connection; }

protected:
    sqlite3*            m_Connection;
    EDBError            m_LastError;
    StringStorage       m_RawErrorMessage;
    CDBStatementSet     m_DBStatementSet;
};

#endif // CRYINCLUDE_TOOLS_DBAPI_SQLITE_SQLITECONNECTION_H
