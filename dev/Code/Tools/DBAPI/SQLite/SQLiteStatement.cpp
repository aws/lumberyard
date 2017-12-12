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
#include "SQLiteStatement.h"
#include "SQLiteConnection.h"
#include <SQLite/sqlite3.h>
#include "SQLiteResultSet.h"

CSQLiteStatement::CSQLiteStatement(CSQLiteConnection* Connection)
    : m_Connection(Connection)
    , m_UpdateCount(-1)
    , m_hStatement(NULL)
    , m_LastResult(SQLITE_OK)
    , m_ResultSet(NULL)
    , m_bGetResult(true)
{
    // TODO NULL Check
}

CSQLiteStatement::~CSQLiteStatement()
{
    Close();
}

bool CSQLiteStatement::Close()
{
    CloseResultSet();
    return Reset();
}

bool CSQLiteStatement::Reset()
{
    m_UpdateCount = -1;

    if (m_hStatement != NULL)
    {
        // http://www.sqlite.org/c3ref/finalize.html
        int Ret = sqlite3_finalize(m_hStatement);
        m_hStatement = NULL;

        if (Ret != SQLITE_OK)
        {
            m_Connection->SetSQLiteLastError(Ret);
            return false;
        }
        return true;
    }

    return false;
}

bool CSQLiteStatement::Execute(const char* Query)
{
    Reset();

    sqlite3* Handle = m_Connection->GetRawHandle();

    // http://www.sqlite.org/c3ref/prepare.html
    const char* zErrMsg = 0;
    int Ret = sqlite3_prepare_v2(Handle, Query, static_cast<unsigned long>(strlen(Query)), &m_hStatement, &zErrMsg);
    if (Ret != SQLITE_OK)
    {
        m_Connection->SetSQLiteLastError(Ret, zErrMsg);
        return false;
    }
    // http://www.sqlite.org/c3ref/step.html
    m_LastResult = sqlite3_step(m_hStatement);

    if (m_LastResult != SQLITE_DONE && m_LastResult != SQLITE_ROW)
    {
        Close();
        m_Connection->SetSQLiteLastError(m_LastResult);
        return false;
    }
    if (m_LastResult == SQLITE_DONE)
    {
        // http://www.sqlite.org/c3ref/changes.html
        m_UpdateCount = sqlite3_changes(Handle);
    }

    if (m_LastResult == SQLITE_ROW || m_UpdateCount <= 0)
    {
        m_bGetResult = false;
    }
    m_Connection->SetLastError(eDE_SUCCESS);
    return true;
}

IDBResultSet* CSQLiteStatement::GetResultSet(bool bUnbuffered)
{
    UNREFERENCED_PARAMETER(bUnbuffered);

    CloseResultSet();

    if (m_hStatement)
    {
        m_Connection->SetLastError(eDE_SUCCESS);
        m_ResultSet = new CSQLiteResultSet(m_Connection, this, m_hStatement, m_LastResult);
        m_bGetResult = true;
        return m_ResultSet;
    }
    m_Connection->SetLastError(eDB_CLOSED_STATEMENT);
    return NULL;
}

bool CSQLiteStatement::CloseResultSet()
{
    if (m_ResultSet != NULL)
    {
        delete m_ResultSet;
        m_ResultSet = NULL;
        m_Connection->SetLastError(eDE_SUCCESS);
        return true;
    }

    if (!m_bGetResult)
    {
        m_Connection->SetLastError(eDE_SUCCESS);
        return true;
    }
    m_Connection->SetLastError(eDB_CLOSED_RESULTSET);
    return false;
}

bool CSQLiteStatement::GetMoreResults()
{
    return false;
}

IDBConnection* CSQLiteStatement::GetConnection() const
{
    return m_Connection;
}

int64 CSQLiteStatement::GetUpdateCount() const
{
    return m_UpdateCount;
}

// http://www.sqlite.org/c3ref/last_insert_rowid.html
int64 CSQLiteStatement::GetAutoIncrementedValue() const
{
    sqlite3* Handle = m_Connection->GetRawHandle();
    if (Handle == NULL)
    {
        m_Connection->SetLastError(eDB_CLOSED_STATEMENT);
        return -1;
    }
    m_Connection->SetLastError(eDE_SUCCESS);
    int64 RetValue = sqlite3_last_insert_rowid(Handle);
    return RetValue;
}

CSQLitePreparedStatement::CSQLitePreparedStatement(CSQLiteConnection* Connection, const char* Query)
    : m_Connection(Connection)
    , m_ResultSet(NULL)
    , m_Query(Query)
    , m_LastResult(SQLITE_OK)
    , m_hStatement(NULL)
    , m_UpdateCount(-1)
    , m_ParamCount(0)
    , m_bToReset(false)
    , m_bGetResult(true)
{
}

CSQLitePreparedStatement::~CSQLitePreparedStatement()
{
    Close();
}

bool CSQLitePreparedStatement::Close()
{
    CloseResultSet();

    m_ParamCount = 0;
    m_UpdateCount = -1;

    if (m_hStatement == NULL)
    {
        return false;
    }

    // http://www.sqlite.org/c3ref/finalize.html
    int Ret = sqlite3_finalize(m_hStatement);
    m_hStatement = NULL;

    if (Ret != SQLITE_OK)
    {
        m_Connection->SetSQLiteLastError(Ret);
        return false;
    }
    return true;
}

bool CSQLitePreparedStatement::Prepare()
{
    if (m_Connection->IsClosed())
    {
        m_Connection->SetLastError(eDB_NOT_CONNECTED);
        return false;
    }

    sqlite3* Handle = m_Connection->GetRawHandle();
    // http://www.sqlite.org/c3ref/prepare.html
    const char* zErrMsg = 0;
    int Ret = sqlite3_prepare_v2(Handle, m_Query.ToString(), static_cast<unsigned long>(m_Query.Length()), &m_hStatement, &zErrMsg);
    if (Ret != SQLITE_OK)
    {
        m_Connection->SetSQLiteLastError(Ret, zErrMsg);
        return false;
    }
    // http://www.sqlite.org/c3ref/bind_parameter_count.html
    m_ParamCount = sqlite3_bind_parameter_count(m_hStatement);
    m_Connection->SetLastError(eDE_SUCCESS);
    return true;
}

bool CSQLitePreparedStatement::Execute(const char* Query)
{
    m_Connection->SetLastError(eDB_NOT_SUPPORTED);
    return false;
}

IDBResultSet* CSQLitePreparedStatement::GetResultSet(bool bUnbuffered)
{
    UNREFERENCED_PARAMETER(bUnbuffered);

    CloseResultSet();

    if (m_hStatement)
    {
        m_Connection->SetLastError(eDE_SUCCESS);
        m_ResultSet = new CSQLiteResultSet(m_Connection, this, m_hStatement, m_LastResult);
        return m_ResultSet;
    }
    m_Connection->SetLastError(eDB_CLOSED_STATEMENT);
    return NULL;
}

bool CSQLitePreparedStatement::CloseResultSet()
{
    if (m_ResultSet != NULL)
    {
        delete m_ResultSet;
        m_ResultSet = NULL;
        m_Connection->SetLastError(eDE_SUCCESS);
        return true;
    }
    m_Connection->SetLastError(eDB_CLOSED_RESULTSET);
    return false;
}

bool CSQLitePreparedStatement::GetMoreResults()
{
    m_Connection->SetLastError(eDE_SUCCESS);
    return false;
}

IDBConnection* CSQLitePreparedStatement::GetConnection() const
{
    m_Connection->SetLastError(eDE_SUCCESS);
    return m_Connection;
}

int64 CSQLitePreparedStatement::GetUpdateCount() const
{
    return m_UpdateCount;
}

#define CHECK_CLOSED_STATEMENT(hStatement)                \
    if (hStatement == NULL)                               \
    {                                                     \
        m_Connection->SetLastError(eDB_CLOSED_STATEMENT); \
        return false;                                     \
    }

#define CHECK_INVALID_PARAM_INDEX(Index, MaxIndex)                    \
    if (!(0 <= Index && Index < static_cast<unsigned int>(MaxIndex))) \
    {                                                                 \
        m_Connection->SetLastError(eDB_INVALID_PARAM_INDEX);          \
        return false;                                                 \
    }

#define CHECK_RESULT(Result)                      \
    if (Result != SQLITE_OK)                      \
    {                                             \
        m_Connection->SetSQLiteLastError(Result); \
        return false;                             \
    }

bool CSQLitePreparedStatement::ResetBind()
{
    CHECK_CLOSED_STATEMENT(m_hStatement);

    // Reset
    int Ret = sqlite3_clear_bindings(m_hStatement);
    if (Ret != SQLITE_OK)
    {
        m_Connection->SetSQLiteLastError(Ret);
        return false;
    }

    Ret = sqlite3_reset(m_hStatement);
    if (Ret != SQLITE_OK)
    {
        m_Connection->SetSQLiteLastError(Ret);
        return false;
    }

    m_bToReset = false;
    return true;
}

bool CSQLitePreparedStatement::Execute()
{
    CHECK_CLOSED_STATEMENT(m_hStatement);

    sqlite3* Handle = m_Connection->GetRawHandle();

    // http://www.sqlite.org/c3ref/step.html
    m_LastResult = sqlite3_step(m_hStatement);
    if (m_LastResult != SQLITE_DONE && m_LastResult != SQLITE_ROW)
    {
        Close();
        m_Connection->SetSQLiteLastError(m_LastResult);
        return false;
    }

    if (m_LastResult == SQLITE_DONE)
    {
        // http://www.sqlite.org/c3ref/changes.html
        m_UpdateCount = sqlite3_changes(Handle);
    }

    m_bToReset = true;
    m_Connection->SetLastError(eDE_SUCCESS);
    return true;
}

const char* CSQLitePreparedStatement::GetQuery() const
{
    return m_Query.ToString();
}

#define CHECK_TO_RESET()            \
    if (m_bToReset && !ResetBind()) \
    {                               \
        return false;               \
    }

// http://www.sqlite.org/c3ref/bind_blob.html
bool CSQLitePreparedStatement::SetNullByIndex(unsigned int ParamIndex)
{
    CHECK_CLOSED_STATEMENT(m_hStatement);
    CHECK_INVALID_PARAM_INDEX(ParamIndex, m_ParamCount);
    CHECK_TO_RESET();

    int Ret = sqlite3_bind_null(m_hStatement, ParamIndex + 1);
    CHECK_RESULT(Ret);
    return true;
}

bool CSQLitePreparedStatement::SetInt8ByIndex(unsigned int ParamIndex, int8 Value)
{
    CHECK_CLOSED_STATEMENT(m_hStatement);
    CHECK_INVALID_PARAM_INDEX(ParamIndex, m_ParamCount);
    CHECK_TO_RESET();

    int Ret = sqlite3_bind_int(m_hStatement, ParamIndex + 1, Value);
    CHECK_RESULT(Ret);
    return true;
}

bool CSQLitePreparedStatement::SetInt16ByIndex(unsigned int ParamIndex, int16 Value)
{
    CHECK_CLOSED_STATEMENT(m_hStatement);
    CHECK_INVALID_PARAM_INDEX(ParamIndex, m_ParamCount);
    CHECK_TO_RESET();

    int Ret = sqlite3_bind_int(m_hStatement, ParamIndex + 1, Value);
    CHECK_RESULT(Ret);
    return true;
}

bool CSQLitePreparedStatement::SetInt32ByIndex(unsigned int ParamIndex, int32 Value)
{
    CHECK_CLOSED_STATEMENT(m_hStatement);
    CHECK_INVALID_PARAM_INDEX(ParamIndex, m_ParamCount);
    CHECK_TO_RESET();

    int Ret = sqlite3_bind_int(m_hStatement, ParamIndex + 1, Value);
    CHECK_RESULT(Ret);
    return true;
}

bool CSQLitePreparedStatement::SetInt64ByIndex(unsigned int ParamIndex, int64 Value)
{
    CHECK_CLOSED_STATEMENT(m_hStatement);
    CHECK_INVALID_PARAM_INDEX(ParamIndex, m_ParamCount);
    CHECK_TO_RESET();

    int Ret = sqlite3_bind_int64(m_hStatement, ParamIndex + 1, Value);
    CHECK_RESULT(Ret);
    return true;
}

bool CSQLitePreparedStatement::SetUInt8ByIndex(unsigned int ParamIndex, uint8 Value)
{
    CHECK_CLOSED_STATEMENT(m_hStatement);
    CHECK_INVALID_PARAM_INDEX(ParamIndex, m_ParamCount);
    CHECK_TO_RESET();

    int Ret = sqlite3_bind_int(m_hStatement, ParamIndex + 1, Value);
    CHECK_RESULT(Ret);
    return true;
}

bool CSQLitePreparedStatement::SetUInt16ByIndex(unsigned int ParamIndex, uint16 Value)
{
    CHECK_CLOSED_STATEMENT(m_hStatement);
    CHECK_INVALID_PARAM_INDEX(ParamIndex, m_ParamCount);
    CHECK_TO_RESET();

    int Ret = sqlite3_bind_int(m_hStatement, ParamIndex + 1, Value);
    CHECK_RESULT(Ret);
    return true;
}

bool CSQLitePreparedStatement::SetUInt32ByIndex(unsigned int ParamIndex, uint32 Value)
{
    CHECK_CLOSED_STATEMENT(m_hStatement);
    CHECK_INVALID_PARAM_INDEX(ParamIndex, m_ParamCount);
    CHECK_TO_RESET();

    int Ret = sqlite3_bind_int(m_hStatement, ParamIndex + 1, Value);
    CHECK_RESULT(Ret);
    return true;
}

bool CSQLitePreparedStatement::SetUInt64ByIndex(unsigned int ParamIndex, uint64 Value)
{
    CHECK_CLOSED_STATEMENT(m_hStatement);
    CHECK_INVALID_PARAM_INDEX(ParamIndex, m_ParamCount);
    CHECK_TO_RESET();

    int Ret = sqlite3_bind_int64(m_hStatement, ParamIndex + 1, Value);
    CHECK_RESULT(Ret);
    return true;
}

bool CSQLitePreparedStatement::SetDateByIndex(unsigned int ParamIndex, time_t Value)
{
    CHECK_CLOSED_STATEMENT(m_hStatement);
    CHECK_INVALID_PARAM_INDEX(ParamIndex, m_ParamCount);
    CHECK_TO_RESET();

    m_Connection->SetLastError(eDB_NOT_SUPPORTED);
    return false;
}

bool CSQLitePreparedStatement::SetFloatByIndex(unsigned int ParamIndex, float Value)
{
    CHECK_CLOSED_STATEMENT(m_hStatement);
    CHECK_INVALID_PARAM_INDEX(ParamIndex, m_ParamCount);
    CHECK_TO_RESET();

    int Ret = sqlite3_bind_double(m_hStatement, ParamIndex + 1, Value);
    CHECK_RESULT(Ret);
    return true;
}

bool CSQLitePreparedStatement::SetDoubleByIndex(unsigned int ParamIndex, double Value)
{
    CHECK_CLOSED_STATEMENT(m_hStatement);
    CHECK_INVALID_PARAM_INDEX(ParamIndex, m_ParamCount);
    CHECK_TO_RESET();

    int Ret = sqlite3_bind_double(m_hStatement, ParamIndex + 1, Value);
    CHECK_RESULT(Ret);
    return true;
}

bool CSQLitePreparedStatement::SetStringByIndex(unsigned int ParamIndex,
    const char* Buffer,
    size_t Length)
{
    CHECK_CLOSED_STATEMENT(m_hStatement);
    CHECK_INVALID_PARAM_INDEX(ParamIndex, m_ParamCount);
    CHECK_TO_RESET();

    int Ret = sqlite3_bind_text(m_hStatement, ParamIndex + 1, Buffer,
            static_cast<unsigned long>(Length), SQLITE_STATIC);
    CHECK_RESULT(Ret);
    return true;
}

bool CSQLitePreparedStatement::SetBinaryByIndex(unsigned int ParamIndex,
    const byte* Buffer,
    size_t Length)
{
    CHECK_CLOSED_STATEMENT(m_hStatement);
    CHECK_INVALID_PARAM_INDEX(ParamIndex, m_ParamCount);
    CHECK_TO_RESET();

    int Ret = sqlite3_bind_blob(m_hStatement, ParamIndex + 1, Buffer,
            static_cast<unsigned long>(Length), SQLITE_STATIC);
    CHECK_RESULT(Ret);
    return true;
}

int64 CSQLitePreparedStatement::GetAutoIncrementedValue() const
{
    sqlite3* Handle = m_Connection->GetRawHandle();
    if (Handle == NULL)
    {
        m_Connection->SetLastError(eDB_CLOSED_STATEMENT);
        return -1;
    }
    m_Connection->SetLastError(eDE_SUCCESS);
    return sqlite3_last_insert_rowid(Handle);
}