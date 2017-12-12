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

#include "stdafx.h"
#include <SQLite/sqlite3.h>
#include "SQLiteConnection.h"
#include "SQLiteStatement.h"
#include <AzCore/base.h>

CSQLiteConnection::CSQLiteConnection()
    : m_Connection(NULL)
    , m_LastError(eDE_SUCCESS)
{
}

CSQLiteConnection::~CSQLiteConnection()
{
    Close();
}

unsigned long CSQLiteConnection::ClientVersion() const
{
    int Version = sqlite3_libversion_number();
    return (Version / 1000000) << 16 | (Version / 1000) % 1000;
}

unsigned long CSQLiteConnection::ServerVersion() const
{
    int Version = sqlite3_libversion_number();
    return (Version / 1000000) << 16  | (Version / 1000) % 1000;
}

// sqlite://dirA/dirB/dbfile
// sqlite:/DRIVE:/dirA/dirB/dbfile
// sqlite:///COMPUTERNAME/shareA/dirB/dbfile
bool CSQLiteConnection::Connect(const char* ConnectionString, const char* LocalPath)
{
    const char* FileName = ConnectionString;
    if (FileName == NULL)
    {
        SetLastError(eDB_INVALID_CONNECTION_STRING);
        return false;
    }

    char fullpath[2048];
    azsnprintf(fullpath, AZ_ARRAY_SIZE(fullpath), "%s\\%s", LocalPath, ConnectionString);
    FileName = fullpath;

    if (*FileName == '/')
    {
        FileName++;
    }

    if (!IsClosed())
    {
        Close();
    }

    // http://www.sqlite.org/c3ref/open.html
    int Ret = sqlite3_open(FileName, &m_Connection);
    if (Ret != SQLITE_OK)
    {
        SetSQLiteLastError(Ret);
        m_Connection = NULL;
        return false;
    }

    SetLastError(eDE_SUCCESS);
    return true;
}

bool CSQLiteConnection::Ping()
{
    if (IsClosed())
    {
        SetLastError(eDB_NOT_CONNECTED);
        return false;
    }
    return true;
}

bool CSQLiteConnection::Close()
{
    m_DBStatementSet.DeleteAll();

    if (m_Connection != NULL)
    {
        // http://www.sqlite.org/c3ref/close.html
        int Ret =  sqlite3_close(m_Connection);
        m_Connection = NULL;
        return Ret == SQLITE_OK;
    }
    return false;
}

bool CSQLiteConnection::Commit()
{
    // Autocommit mode is on by default
    return false;
}

bool CSQLiteConnection::Rollback()
{
    // Autocommit mode is on by default
    return false;
}

bool CSQLiteConnection::GetAutoCommit() const
{
    //if( m_Connection )
    //{
    //    // http://www.sqlite.org/c3ref/get_autocommit.html
    //    int Ret = SQLiteAPI->sqlite3_get_autocommit(m_Connection);
    //    SetLastError(eDE_SUCCESS);
    //    return Ret != 0; // 0 means autocommit is disabled, maybe always true
    //}
    //SetLastError(eDB_NOT_CONNECTED);
    return true;
}

bool CSQLiteConnection::SetAutoCommit(bool bAutoCommit)
{
    // Autocommit mode is on by default
    // you can not change it.
    // if you want to turn off it, you should querys with "BEGIN"
    SetLastError(eDB_NOT_SUPPORTED);
    return false;
}

IDBStatement* CSQLiteConnection::CreateStatement()
{
    if (IsClosed())
    {
        SetLastError(eDB_NOT_CONNECTED);
        return NULL;
    }

    IDBStatement* Statement = new CSQLiteStatement(this);
    SetLastError(eDE_SUCCESS);
    m_DBStatementSet.Add(Statement);
    return Statement;
}

IDBPreparedStatement* CSQLiteConnection::CreatePreparedStatement(const char* Query)
{
    if (IsClosed())
    {
        SetLastError(eDB_NOT_CONNECTED);
        return NULL;
    }

    SetLastError(eDE_SUCCESS);
    CSQLitePreparedStatement* DBStatement = new CSQLitePreparedStatement(this, Query);
    if (!DBStatement->Prepare())
    {
        delete DBStatement;
        return NULL;
    }
    // SetLastError(eDE_SUCCESS); // maybe set errorcode in Prepare()
    m_DBStatementSet.Add(DBStatement);
    return DBStatement;
}

IDBCallableStatement* CSQLiteConnection::CreateCallableStatement(const char* Query)
{
    return NULL;
}

bool CSQLiteConnection::DestroyStatement(IDBStatement* DBStatement)
{
    bool Ret = m_DBStatementSet.Delete(DBStatement);
    if (Ret)
    {
        delete DBStatement;
        SetLastError(eDE_SUCCESS);
        return true;
    }
    SetLastError(eDB_INVALID_POINTER);
    return false;
    ;
}

EDBError CSQLiteConnection::GetLastError() const
{
    return m_LastError;
}

const char* CSQLiteConnection::GetRawErrorMessage() const
{
    return m_RawErrorMessage.ToString();
}

struct SQLiteErrnoConvertingItem
{
    unsigned int    SQLiteError;
    EDBError        DBErrno;
};

// http://www.sqlite.org/c3ref/c_abort.html
static SQLiteErrnoConvertingItem SQLLiteErrnoConveringTable[] =
{
    {SQLITE_OK,         eDE_SUCCESS},
    {SQLITE_ERROR,      eDB_UNKNOWN},
    {SQLITE_INTERNAL,   eDB_UNKNOWN},
    {SQLITE_PERM,       eDB_UNKNOWN},
    {SQLITE_ABORT,      eDB_UNKNOWN},
    {SQLITE_BUSY,       eDB_UNKNOWN},
    {SQLITE_LOCKED,     eDB_UNKNOWN},
    {SQLITE_NOMEM,      eDB_UNKNOWN},
    {SQLITE_READONLY,   eDB_UNKNOWN},
    {SQLITE_INTERRUPT,  eDB_UNKNOWN},
    {SQLITE_IOERR,      eDB_UNKNOWN},
    {SQLITE_CORRUPT,    eDB_UNKNOWN},
    {SQLITE_NOTFOUND,   eDB_UNKNOWN},
    {SQLITE_FULL,       eDB_UNKNOWN},
    {SQLITE_CANTOPEN,   eDB_UNKNOWN},
    {SQLITE_PROTOCOL,   eDB_UNKNOWN},
    {SQLITE_EMPTY,      eDB_UNKNOWN},
    {SQLITE_SCHEMA,     eDB_UNKNOWN},
    {SQLITE_TOOBIG,     eDB_UNKNOWN},
    {SQLITE_CONSTRAINT, eDB_UNKNOWN},
    {SQLITE_MISMATCH,   eDB_UNKNOWN},
    {SQLITE_MISUSE,     eDB_UNKNOWN},
    {SQLITE_NOLFS,      eDB_UNKNOWN},
    {SQLITE_AUTH,       eDB_UNKNOWN},
    {SQLITE_FORMAT,     eDB_UNKNOWN},
    {SQLITE_RANGE,      eDB_UNKNOWN},
    {SQLITE_NOTADB,     eDB_UNKNOWN},
    {SQLITE_ROW,        eDB_UNKNOWN},
    {SQLITE_DONE,       eDB_UNKNOWN},
};

void CSQLiteConnection::SetLastError(EDBError Error)
{
    m_LastError = Error;
    m_RawErrorMessage.Clear();
}

void CSQLiteConnection::SetSQLiteLastError(unsigned int SQLiteError, const char* ErrorMessage)
{
    if (SQLITE_OK <= SQLiteError && SQLiteError <= SQLITE_DONE)
    {
        m_LastError = SQLLiteErrnoConveringTable[SQLiteError - SQLITE_OK].DBErrno;
    }
    else
    {
        m_LastError = eDB_UNKNOWN;
    }
    // http://www.sqlite.org/c3ref/errcode.html
    if (ErrorMessage != NULL)
    {
        m_RawErrorMessage = ErrorMessage;
    }
    else if (m_Connection != NULL)
    {
        m_RawErrorMessage = sqlite3_errmsg(m_Connection);
    }
#ifdef _DEBUG
    if (SQLiteError !=  SQLITE_OK)
    {
        fprintf(stderr, "[errno:%d] %s\n", SQLiteError, GetRawErrorMessage());
    }
#endif // _DEBUG
}