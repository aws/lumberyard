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

#ifndef CRYINCLUDE_TOOLS_DBAPI_SQLITE_SQLITESTATEMENT_H
#define CRYINCLUDE_TOOLS_DBAPI_SQLITE_SQLITESTATEMENT_H
#pragma once

#include "IDBStatement.h"
#include "Util/DBUtil.h"

class CSQLiteConnection;
class CSQLiteResultSet;
struct sqlite3_stmt;

class CSQLiteStatement
    : public IDBStatement
{
public:
    CSQLiteStatement(CSQLiteConnection* Connection);

    virtual ~CSQLiteStatement();

    virtual bool Close();

    virtual bool Execute(const char* Query);

    virtual IDBResultSet* GetResultSet(bool bUnbuffered = false);

    virtual bool CloseResultSet();

    virtual bool GetMoreResults();

    virtual IDBConnection* GetConnection() const;

    virtual int64  GetUpdateCount() const;

    virtual int64 GetAutoIncrementedValue() const;

protected:
    bool Reset();
    void ReleaseResultSet();

    int64                m_UpdateCount;
    CSQLiteConnection*   m_Connection;
    CSQLiteResultSet*    m_ResultSet;
    sqlite3_stmt*        m_hStatement;
    int                  m_LastResult;
    bool                 m_bGetResult;
};

class CSQLitePreparedStatement
    : public IDBPreparedStatement
{
public:
    CSQLitePreparedStatement(CSQLiteConnection* Connection, const char* Query);

    virtual ~CSQLitePreparedStatement();

    virtual bool Execute(const char* Query);

    virtual IDBResultSet* GetResultSet(bool bUnbuffered = false);

    virtual bool CloseResultSet();

    virtual bool GetMoreResults();

    virtual IDBConnection* GetConnection() const;

    virtual int64 GetUpdateCount() const;

    virtual bool Execute();

    virtual const char* GetQuery() const;

    virtual bool SetNullByIndex(unsigned int ParamIndex);

    virtual bool SetInt8ByIndex(unsigned int ParamIndex, int8 Value);

    virtual bool SetInt16ByIndex(unsigned int ParamIndex, int16 Value);

    virtual bool SetInt32ByIndex(unsigned int ParamIndex, int32 Value);

    virtual bool SetInt64ByIndex(unsigned int ParamIndex, int64 Value);

    virtual bool SetUInt8ByIndex(unsigned int ParamIndex, uint8 Value);

    virtual bool SetUInt16ByIndex(unsigned int ParamIndex, uint16 Value);

    virtual bool SetUInt32ByIndex(unsigned int ParamIndex, uint32 Value);

    virtual bool SetUInt64ByIndex(unsigned int ParamIndex, uint64 Value);

    virtual bool SetDateByIndex(unsigned int ParamIndex, time_t Value);

    virtual bool SetFloatByIndex(unsigned int ParamIndex, float Value);

    virtual bool SetDoubleByIndex(unsigned int ParamIndex, double Value);

    virtual bool SetStringByIndex(unsigned int ParamIndex,
        const char* Buffer,
        size_t Length);

    virtual bool SetBinaryByIndex(unsigned int ParamIndex,
        const byte* Buffer,
        size_t Length);

    virtual int64 GetAutoIncrementedValue() const;

    // additional function
    virtual bool Prepare();

protected:
    bool Close();
    bool ResetBind();
    void ReleaseResultSet();

    int64                m_UpdateCount;
    CSQLiteConnection*   m_Connection;
    CSQLiteResultSet*    m_ResultSet;
    sqlite3_stmt*        m_hStatement;
    int                  m_LastResult;
    StringStorage        m_Query;
    int                  m_ParamCount;
    bool                 m_bToReset;
    bool                 m_bGetResult;
};


#endif // CRYINCLUDE_TOOLS_DBAPI_SQLITE_SQLITESTATEMENT_H
