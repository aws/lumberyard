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

#ifndef CRYINCLUDE_TOOLS_DBAPI_SQLITE_SQLITERESULTSET_H
#define CRYINCLUDE_TOOLS_DBAPI_SQLITE_SQLITERESULTSET_H
#pragma once


#include "IDBResultSet.h"
#include "Util/DBUtil.h"

class CSQLiteConnection;
struct sqlite3_stmt;
class CSQLiteResultSet
    : public IDBResultSet
{
public:
    CSQLiteResultSet(CSQLiteConnection* Connection, IDBStatement* Statement, sqlite3_stmt* hStatement, int LastMode);

    virtual ~CSQLiteResultSet();

    virtual int FindColumn(const char* ColumnName) const;

    virtual IDBStatement* GetStatement() const;

    virtual int GetColumnCount() const;

    virtual int64 GetNumRows();

    virtual bool Next();

    virtual bool IsNullByIndex(unsigned int ColumnIndex) const;

    virtual bool GetInt8ByIndex(unsigned int ColumnIndex, int8& OutValue) const;

    virtual bool GetInt16ByIndex(unsigned int ColumnIndex, int16& OutValue) const;

    virtual bool GetInt32ByIndex(unsigned int ColumnIndex, int32& OutValue) const;

    virtual bool GetInt64ByIndex(unsigned int ColumnIndex, int64& OutValue) const;

    virtual bool GetUInt8ByIndex(unsigned int ColumnIndex, uint8& OutValue) const;

    virtual bool GetUInt16ByIndex(unsigned int ColumnIndex, uint16& OutValue) const;

    virtual bool GetUInt32ByIndex(unsigned int ColumnIndex, uint32& OutValue) const;

    virtual bool GetUInt64ByIndex(unsigned int ColumnIndex, uint64& OutValue) const;

    virtual bool GetDoubleByIndex(unsigned int ColumnIndex, double& OutValue) const;

    virtual bool GetFloatByIndex(unsigned int ColumnIndex, float& OutValue) const;

    virtual bool GetStringByIndex(unsigned int ColumnIndex, const char*& OutValue) const;

    virtual bool GetBinaryByIndex(unsigned int ColumnIndex, const byte*& OutValue, size_t& OutSize) const;

    virtual bool GetDateByIndex(unsigned int ColumnIndex, time_t& OutValue) const;

    virtual bool IsNullByName(const char* ColumnName) const;

    virtual bool GetInt8ByName(const char* ColumnName, int8& OutValue) const;

    virtual bool GetInt16ByName(const char* ColumnName, int16& OutValue) const;

    virtual bool GetInt32ByName(const char* ColumnName, int32& OutValue) const;

    virtual bool GetInt64ByName(const char* ColumnName, int64& OutValue) const;

    virtual bool GetUInt8ByName(const char* ColumnName, uint8& OutValue) const;

    virtual bool GetUInt16ByName(const char* ColumnName, uint16& OutValue) const;

    virtual bool GetUInt32ByName(const char* ColumnName, uint32& OutValue) const;

    virtual bool GetUInt64ByName(const char* ColumnName, uint64& OutValue) const;

    virtual bool GetFloatByName(const char* ColumnName, float& OutValue) const;

    virtual bool GetDoubleByName(const char* ColumnName, double& OutValue) const;

    virtual bool GetStringByName(const char* ColumnName, const char*& OutValue) const;

    virtual bool GetBinaryByName(const char* ColumnName, const byte*& OutValue, size_t& OutSize) const;

    virtual bool GetDateByName(const char* ColumnName, time_t& OutValue) const;

protected:

    bool Close();

    bool IsClosed() const { return m_hStatement == NULL; }

    CNameToIndexMap     m_NameToIndexMap;
    CSQLiteConnection*  m_Connection;
    IDBStatement*       m_Statement;
    sqlite3_stmt*       m_hStatement;
    int                 m_LastMode;
    bool                m_IsFirstNext;
    unsigned int        m_NumFields;
    int64               m_NumRows;
};

#endif // CRYINCLUDE_TOOLS_DBAPI_SQLITE_SQLITERESULTSET_H
