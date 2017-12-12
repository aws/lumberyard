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
#include "SQLiteResultSet.h"
#include "SQLiteConnection.h"
#include "SQLiteStatement.h"
#include <SQLite/sqlite3.h>

CSQLiteResultSet::CSQLiteResultSet(CSQLiteConnection* Connection, IDBStatement* Statement, sqlite3_stmt* hStatement, int LastMode)
    : m_Connection(Connection)
    , m_Statement(Statement)
    , m_hStatement(hStatement)
    , m_LastMode(LastMode)
    , m_IsFirstNext(true)
    , m_NumFields(0)
    , m_NumRows(0)
{
}

CSQLiteResultSet::~CSQLiteResultSet()
{
}

int CSQLiteResultSet::FindColumn(const char* ColumnName) const
{
    return m_NameToIndexMap.Find(ColumnName);
}

IDBStatement* CSQLiteResultSet::GetStatement() const
{
    return m_Statement;
}

int CSQLiteResultSet::GetColumnCount() const
{
    return m_NumFields;
}

int64 CSQLiteResultSet::GetNumRows()
{
    return m_NumRows;
    //m_Connection->SetLastError(eDB_NOT_SUPPORTED);
    //return -1;
}

bool CSQLiteResultSet::Next()
{
    m_Connection->SetLastError(eDE_SUCCESS);
    if (m_LastMode == SQLITE_DONE)
    {
        return false;
    }

    if (m_LastMode == SQLITE_ROW)
    {
        if (m_IsFirstNext)
        {
            // loading metadata
            // http://www.sqlite.org/c3ref/column_count.html
            m_NumFields = sqlite3_column_count(m_hStatement);
            if (m_NumFields > 0)
            {
                for (unsigned int i = 0; i < m_NumFields; i++)
                {
                    // http://www.sqlite.org/c3ref/column_name.html
                    const char* FieldName = sqlite3_column_name(m_hStatement, i);
                    m_NameToIndexMap.Add(FieldName, i);
                }
            }
            m_IsFirstNext = false;
            m_NumRows++;
            return true;
        }

        // http://www.sqlite.org/c3ref/step.html
        int m_LastMode = sqlite3_step(m_hStatement);
        if (m_LastMode == SQLITE_ROW)
        {
            m_NumRows++;
            return true;
        }
        if (m_LastMode == SQLITE_DONE)
        {
            return false;
        }
    }
    m_Connection->SetSQLiteLastError(m_LastMode);
    return false;


    //if( m_LastMode == SQLITE_DONE || m_LastMode == SQLITE_ROW)
    //{
    //    m_Connection->SetLastError(eDE_SUCCESS);
    //    if( m_LastMode == SQLITE_DONE )
    //    {
    //        return false;
    //    }
    //    else if( m_IsFirstNext )
    //    {
    //        // loading metadata
    //        // http://www.sqlite.org/c3ref/column_count.html
    //        m_NumFields = SQLiteAPI->sqlite3_column_count(m_hStatement);
    //        if( m_NumFields > 0 )
    //        {
    //            for( unsigned int i = 0; i < m_NumFields; i++ )
    //            {
    //                // http://www.sqlite.org/c3ref/column_name.html
    //                const char* FieldName = SQLiteAPI->sqlite3_column_name(m_hStatement, i);
    //                m_NameToIndexMap.Add(FieldName, i);
    //            }
    //        }
    //        m_IsFirstNext = false;
    //        m_NumRows++;
    //        return true;
    //    }

    //    // http://www.sqlite.org/c3ref/step.html
    //    int m_LastMode = SQLiteAPI->sqlite3_step(m_hStatement);

    //    if( m_LastMode == SQLITE_ROW || m_LastMode == SQLITE_DONE)
    //    {
    //        m_Connection->SetLastError(eDE_SUCCESS);
    //
    //        return m_LastMode == SQLITE_ROW;
    //    }
    //}

    //m_Connection->SetSQLiteLastError(m_LastMode);
    //return false;
}

bool CSQLiteResultSet::IsNullByIndex(unsigned int ColumnIndex) const
{
    CHECK_CLOSED_RESULTSET(false);
    CHECK_INVALID_INDEX(ColumnIndex, m_NumFields);

    m_Connection->SetLastError(eDE_SUCCESS);
    return sqlite3_column_type(m_hStatement, ColumnIndex) == SQLITE_NULL;
}

#define CHECK_IS_CONVERTABLE(Index, ConvertableType)                 \
    if (sqlite3_column_type(m_hStatement, Index) != ConvertableType) \
    {                                                                \
        m_Connection->SetLastError(eDB_VALUE_CANNOT_CONVERT);        \
        return false;                                                \
    }

// ww.sqlite.org/capi3ref.html#sqlite3_column_blob
bool CSQLiteResultSet::GetInt8ByIndex(unsigned int ColumnIndex, int8& OutValue) const
{
    CHECK_CLOSED_RESULTSET(false);
    CHECK_INVALID_INDEX(ColumnIndex, m_NumFields);
    CHECK_IS_CONVERTABLE(ColumnIndex, SQLITE_INTEGER);

    m_Connection->SetLastError(eDE_SUCCESS);
    OutValue = sqlite3_column_int(m_hStatement, ColumnIndex);
    return true;
}

bool CSQLiteResultSet::GetInt16ByIndex(unsigned int ColumnIndex, int16& OutValue) const
{
    CHECK_CLOSED_RESULTSET(false);
    CHECK_INVALID_INDEX(ColumnIndex, m_NumFields);
    CHECK_IS_CONVERTABLE(ColumnIndex, SQLITE_INTEGER);

    m_Connection->SetLastError(eDE_SUCCESS);
    OutValue = sqlite3_column_int(m_hStatement, ColumnIndex);
    return true;
}

bool CSQLiteResultSet::GetInt32ByIndex(unsigned int ColumnIndex, int32& OutValue) const
{
    CHECK_CLOSED_RESULTSET(false);
    CHECK_INVALID_INDEX(ColumnIndex, m_NumFields);
    CHECK_IS_CONVERTABLE(ColumnIndex, SQLITE_INTEGER);

    m_Connection->SetLastError(eDE_SUCCESS);
    // http://www.sqlite.org/c3ref/column_blob.html
    OutValue = sqlite3_column_int(m_hStatement, ColumnIndex);
    return true;
}

bool CSQLiteResultSet::GetInt64ByIndex(unsigned int ColumnIndex, int64& OutValue) const
{
    CHECK_CLOSED_RESULTSET(false);
    CHECK_INVALID_INDEX(ColumnIndex, m_NumFields);
    CHECK_IS_CONVERTABLE(ColumnIndex, SQLITE_INTEGER);

    m_Connection->SetLastError(eDE_SUCCESS);
    OutValue = sqlite3_column_int64(m_hStatement, ColumnIndex);
    return true;
}

bool CSQLiteResultSet::GetUInt8ByIndex(unsigned int ColumnIndex, uint8& OutValue) const
{
    CHECK_CLOSED_RESULTSET(false);
    CHECK_INVALID_INDEX(ColumnIndex, m_NumFields);
    CHECK_IS_CONVERTABLE(ColumnIndex, SQLITE_INTEGER);

    m_Connection->SetLastError(eDE_SUCCESS);
    OutValue = sqlite3_column_int(m_hStatement, ColumnIndex);
    return true;
}

bool CSQLiteResultSet::GetUInt16ByIndex(unsigned int ColumnIndex, uint16& OutValue) const
{
    CHECK_CLOSED_RESULTSET(false);
    CHECK_INVALID_INDEX(ColumnIndex, m_NumFields);
    CHECK_IS_CONVERTABLE(ColumnIndex, SQLITE_INTEGER);

    m_Connection->SetLastError(eDE_SUCCESS);
    OutValue = sqlite3_column_int(m_hStatement, ColumnIndex);
    return true;
}

bool CSQLiteResultSet::GetUInt32ByIndex(unsigned int ColumnIndex, uint32& OutValue) const
{
    CHECK_CLOSED_RESULTSET(false);
    CHECK_INVALID_INDEX(ColumnIndex, m_NumFields);
    CHECK_IS_CONVERTABLE(ColumnIndex, SQLITE_INTEGER);

    m_Connection->SetLastError(eDE_SUCCESS);
    OutValue = sqlite3_column_int(m_hStatement, ColumnIndex);
    return true;
}

bool CSQLiteResultSet::GetUInt64ByIndex(unsigned int ColumnIndex, uint64& OutValue) const
{
    CHECK_CLOSED_RESULTSET(false);
    CHECK_INVALID_INDEX(ColumnIndex, m_NumFields);
    CHECK_IS_CONVERTABLE(ColumnIndex, SQLITE_INTEGER);

    m_Connection->SetLastError(eDE_SUCCESS);
    OutValue = sqlite3_column_int64(m_hStatement, ColumnIndex);
    return true;
}

bool CSQLiteResultSet::GetDoubleByIndex(unsigned int ColumnIndex, double& OutValue) const
{
    CHECK_CLOSED_RESULTSET(false);
    CHECK_INVALID_INDEX(ColumnIndex, m_NumFields);
    CHECK_IS_CONVERTABLE(ColumnIndex, SQLITE_FLOAT);

    m_Connection->SetLastError(eDE_SUCCESS);
    OutValue = sqlite3_column_double(m_hStatement, ColumnIndex);
    return true;
}

bool CSQLiteResultSet::GetFloatByIndex(unsigned int ColumnIndex, float& OutValue) const
{
    CHECK_CLOSED_RESULTSET(false);
    CHECK_INVALID_INDEX(ColumnIndex, m_NumFields);
    CHECK_IS_CONVERTABLE(ColumnIndex, SQLITE_FLOAT);

    m_Connection->SetLastError(eDE_SUCCESS);
    // http://www.sqlite.org/c3ref/column_blob.html
    OutValue = static_cast<float>(sqlite3_column_double(m_hStatement, ColumnIndex));
    return true;
}

bool CSQLiteResultSet::GetStringByIndex(unsigned int ColumnIndex, const char*& OutValue) const
{
    CHECK_CLOSED_RESULTSET(false);
    CHECK_INVALID_INDEX(ColumnIndex, m_NumFields);
    CHECK_IS_CONVERTABLE(ColumnIndex, SQLITE_TEXT);

    const unsigned char* Str = sqlite3_column_text(m_hStatement, ColumnIndex);
    int size = sqlite3_column_bytes(m_hStatement, ColumnIndex);
    m_Connection->SetLastError(eDE_SUCCESS);
    OutValue = static_cast<const char*>(static_cast<const void*>(Str));
    return true;
}

bool CSQLiteResultSet::GetBinaryByIndex(unsigned int ColumnIndex, const byte*& OutValue, size_t& OutSize) const
{
    CHECK_CLOSED_RESULTSET(false);
    CHECK_INVALID_INDEX(ColumnIndex, m_NumFields);
    CHECK_IS_CONVERTABLE(ColumnIndex, SQLITE_BLOB);

    const void* Value = sqlite3_column_blob(m_hStatement, ColumnIndex);
    int size = sqlite3_column_bytes(m_hStatement, ColumnIndex);
    m_Connection->SetLastError(eDE_SUCCESS);
    OutValue = static_cast<const byte*>(Value);
    OutSize = size;
    return true;
}

bool CSQLiteResultSet::GetDateByIndex(unsigned int ColumnIndex, time_t& OutValue) const
{
    CHECK_CLOSED_RESULTSET(false);
    CHECK_INVALID_INDEX(ColumnIndex, m_NumFields);

    m_Connection->SetLastError(eDB_NOT_SUPPORTED);
    return false;
}

DEFINE_COLUMNBASED_FUNCTION_0(CSQLiteResultSet, IsNull);
DEFINE_COLUMNBASED_FUNCTION_1(CSQLiteResultSet, GetInt8, int8);
DEFINE_COLUMNBASED_FUNCTION_1(CSQLiteResultSet, GetInt16, int16);
DEFINE_COLUMNBASED_FUNCTION_1(CSQLiteResultSet, GetInt32, int32);
DEFINE_COLUMNBASED_FUNCTION_1(CSQLiteResultSet, GetInt64, int64);
DEFINE_COLUMNBASED_FUNCTION_1(CSQLiteResultSet, GetUInt8, uint8);
DEFINE_COLUMNBASED_FUNCTION_1(CSQLiteResultSet, GetUInt16, uint16);
DEFINE_COLUMNBASED_FUNCTION_1(CSQLiteResultSet, GetUInt32, uint32);
DEFINE_COLUMNBASED_FUNCTION_1(CSQLiteResultSet, GetUInt64, uint64);
DEFINE_COLUMNBASED_FUNCTION_1(CSQLiteResultSet, GetFloat, float);
DEFINE_COLUMNBASED_FUNCTION_1(CSQLiteResultSet, GetDouble, double);
DEFINE_COLUMNBASED_FUNCTION_1(CSQLiteResultSet, GetDate, time_t);
DEFINE_COLUMNBASED_FUNCTION_1(CSQLiteResultSet, GetString, const char*);
DEFINE_COLUMNBASED_FUNCTION_2(CSQLiteResultSet, GetBinary, const byte*, size_t);