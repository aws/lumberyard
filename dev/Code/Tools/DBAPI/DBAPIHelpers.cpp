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
#include "DBAPIHelpers.h"
#include "DBAPI.h"

// //
// class CQueryHelper
// //

#define CHECK_CONNECTION             \
    if (!m_pConnection) {            \
        m_Error = eDB_NOT_CONNECTED; \
        return false;                \
    }

#define CHECK_PREPARED_STATEMENT \
    if (!m_pPrepStatement) {     \
        return false; }

#define CHECK_RESULT_SET \
    if (!m_pResultSet) { \
        return false; }

CQueryHelper::CQueryHelper(TDataSourceId datasourceId)
{
    // SGG Implement this when the DBMW is ready
    m_DataSourceId = datasourceId;
    m_bCloseConnection = false;
    m_pStatement = 0;
    m_pPrepStatement = 0;
    m_pResultStatement = 0;
    m_pResultSet = 0;
    m_bCommitted = false;
    m_Error = eDE_SUCCESS;
}

CQueryHelper::CQueryHelper(IDBConnection* pConnection)
{
    m_pConnection = pConnection;
    m_DataSourceId = 0;
    m_bCloseConnection = false;
    m_pStatement = 0;
    m_pPrepStatement = 0;
    m_pResultStatement = 0;
    m_pResultSet = 0;
    m_bCommitted = false;
    m_Error = eDE_SUCCESS;
}

CQueryHelper::CQueryHelper(const char* pcUrl)
{
    m_pConnection = DriverManager::CreateConnection(pcUrl);

    if (!m_pConnection)
    {
        m_Error = m_pConnection ? eDE_SUCCESS : eDB_NOT_CONNECTED;
    }

    m_DataSourceId = 0;
    m_bCloseConnection = true;
    m_pStatement = 0;
    m_pPrepStatement = 0;
    m_pResultStatement = 0;
    m_pResultSet = 0;
    m_bCommitted = false;
}

CQueryHelper::~CQueryHelper()
{
    if (m_pConnection && !m_bCommitted)
    {
        m_pConnection->Rollback();
        m_pConnection->SetAutoCommit(true);
    }

    if (m_pConnection)
    {
        if (m_bCloseConnection)
        {
            DriverManager::DestroyConnection(m_pConnection);
        }
        else
        {
            if (m_pStatement)
            {
                m_pConnection->DestroyStatement(m_pStatement);
            }

            if (m_pPrepStatement)
            {
                // SGG release the prepared statement when the DBMW is ready

                if (m_pResultSet)
                {
                    m_pResultSet->Close();
                }
            }
        }
    }

    if (m_DataSourceId)
    {
        // SGG release the connection to the datasource when the DBMW is ready
    }
}

bool CQueryHelper::Execute(const char* pcStatement)
{
    CHECK_CONNECTION

    if (!m_pStatement)
    {
        m_pStatement = m_pConnection->CreateStatement();

        if (!m_pStatement)
        {
            return false;
        }
    }

    bool bSuccess = m_pStatement->Execute(pcStatement);

    if (bSuccess)
    {
        m_pResultStatement = m_pStatement;

        if (m_pResultSet)
        {
            m_pResultSet->Close();
            m_pResultSet = 0;
        }
    }

    return bSuccess;
}

// Transaction management
bool CQueryHelper::BeginTransaction(bool bCommitPrevious)
{
    CHECK_CONNECTION

    if (!m_pConnection->GetAutoCommit())
    {
        if (bCommitPrevious)
        {
            m_pConnection->Commit();
        }
        else
        {
            m_pConnection->Rollback();
        }
    }

    return m_pConnection->SetAutoCommit(false);
}

bool CQueryHelper::Commit()
{
    CHECK_CONNECTION

    return m_pConnection->Commit();
}

bool CQueryHelper::Rollback()
{
    CHECK_CONNECTION

    return m_pConnection->Rollback();
}

bool CQueryHelper::Next()
{
    CHECK_CONNECTION

    if (m_pResultSet)
    {
        return m_pResultSet->Next();
    }
    else if (m_pResultStatement)
    {
        m_pResultSet = m_pResultStatement->GetResultSet();

        if (m_pResultSet)
        {
            return m_pResultSet->Next();
        }
    }

    return false;
}

IDBResultSet* CQueryHelper::GetCurrentResultSet() const
{
    return m_pResultSet;
}

bool CQueryHelper::SetNull(int nInd)
{
    CHECK_CONNECTION
        CHECK_PREPARED_STATEMENT

    return m_pPrepStatement->SetNullByIndex(nInd);
}

bool CQueryHelper::SetInt(int nInd, int8 nVal)
{
    CHECK_CONNECTION
        CHECK_PREPARED_STATEMENT

    return m_pPrepStatement->SetInt8ByIndex(nInd, nVal);
}

bool CQueryHelper::SetInt(int nInd, int16 nVal)
{
    CHECK_CONNECTION
        CHECK_PREPARED_STATEMENT

    return m_pPrepStatement->SetInt16ByIndex(nInd, nVal);
}

bool CQueryHelper::SetInt(int nInd, int32 nVal)
{
    CHECK_CONNECTION
        CHECK_PREPARED_STATEMENT

    return m_pPrepStatement->SetInt32ByIndex(nInd, nVal);
}

bool CQueryHelper::SetInt(int nInd, int64 nVal)
{
    CHECK_CONNECTION
        CHECK_PREPARED_STATEMENT

    return m_pPrepStatement->SetInt64ByIndex(nInd, nVal);
}

bool CQueryHelper::SetInt(int nInd, uint8 nVal)
{
    CHECK_CONNECTION
        CHECK_PREPARED_STATEMENT

    return m_pPrepStatement->SetUInt8ByIndex(nInd, nVal);
}

bool CQueryHelper::SetInt(int nInd, uint16 nVal)
{
    CHECK_CONNECTION
        CHECK_PREPARED_STATEMENT

    return m_pPrepStatement->SetUInt16ByIndex(nInd, nVal);
}

bool CQueryHelper::SetInt(int nInd, uint32 nVal)
{
    CHECK_CONNECTION
        CHECK_PREPARED_STATEMENT

    return m_pPrepStatement->SetUInt32ByIndex(nInd, nVal);
}

bool CQueryHelper::SetInt(int nInd, uint64 nVal)
{
    CHECK_CONNECTION
        CHECK_PREPARED_STATEMENT

    return m_pPrepStatement->SetUInt64ByIndex(nInd, nVal);
}

bool CQueryHelper::SetFloat(int nInd, float fVal)
{
    CHECK_CONNECTION
        CHECK_PREPARED_STATEMENT

    return m_pPrepStatement->SetFloatByIndex(nInd, fVal);
}

bool CQueryHelper::SetDouble(int nInd, double dVal)
{
    CHECK_CONNECTION
        CHECK_PREPARED_STATEMENT

    return m_pPrepStatement->SetDoubleByIndex(nInd, dVal);
}

bool CQueryHelper::SetString(int nInd, const char* pc)
{
    CHECK_CONNECTION
        CHECK_PREPARED_STATEMENT

    return m_pPrepStatement->SetStringByIndex(nInd, pc, strlen(pc));
}

bool CQueryHelper::SetBinary(int nInd, void* pData, int nSize)
{
    CHECK_CONNECTION
        CHECK_PREPARED_STATEMENT

    return m_pPrepStatement->SetBinaryByIndex(nInd, (const byte*) pData, nSize);
}

bool CQueryHelper::SetDate(int nInd, time_t tVal)
{
    CHECK_CONNECTION
        CHECK_PREPARED_STATEMENT

    return m_pPrepStatement->SetDateByIndex(nInd, tVal);
}

bool CQueryHelper::IsNull(int nInd) const
{
    CHECK_CONNECTION
        CHECK_RESULT_SET

    return m_pResultSet->IsNullByIndex(nInd);
}

bool CQueryHelper::GetInt(int nInd, int8& nVal) const
{
    CHECK_CONNECTION
        CHECK_RESULT_SET

    return m_pResultSet->GetInt8ByIndex(nInd, nVal);
}

bool CQueryHelper::GetInt(int nInd, int16& nVal) const
{
    CHECK_CONNECTION
        CHECK_RESULT_SET

    return m_pResultSet->GetInt16ByIndex(nInd, nVal);
}

bool CQueryHelper::GetInt(int nInd, int32& nVal) const
{
    CHECK_CONNECTION
        CHECK_RESULT_SET

    return m_pResultSet->GetInt32ByIndex(nInd, nVal);
}

bool CQueryHelper::GetInt(int nInd, int64& nVal) const
{
    CHECK_CONNECTION
        CHECK_RESULT_SET

    return m_pResultSet->GetInt64ByIndex(nInd, nVal);
}

bool CQueryHelper::GetInt(int nInd, uint8& nVal) const
{
    CHECK_CONNECTION
        CHECK_RESULT_SET

    return m_pResultSet->GetUInt8ByIndex(nInd, nVal);
}

bool CQueryHelper::GetInt(int nInd, uint16& nVal) const
{
    CHECK_CONNECTION
        CHECK_RESULT_SET

    return m_pResultSet->GetUInt16ByIndex(nInd, nVal);
}

bool CQueryHelper::GetInt(int nInd, uint32& nVal) const
{
    CHECK_CONNECTION
        CHECK_RESULT_SET

    return m_pResultSet->GetUInt32ByIndex(nInd, nVal);
}

bool CQueryHelper::GetInt(int nInd, uint64& nVal) const
{
    CHECK_CONNECTION
        CHECK_RESULT_SET

    return m_pResultSet->GetUInt64ByIndex(nInd, nVal);
}

bool CQueryHelper::GetFloat(int nInd, float& fVal) const
{
    CHECK_CONNECTION
        CHECK_RESULT_SET

    return m_pResultSet->GetFloatByIndex(nInd, fVal);
}

bool CQueryHelper::GetDouble(int nInd, double& dVal) const
{
    CHECK_CONNECTION
        CHECK_RESULT_SET

    return m_pResultSet->GetDoubleByIndex(nInd, dVal);
}

bool CQueryHelper::GetString(int nInd, const char*& pc) const
{
    CHECK_CONNECTION
        CHECK_RESULT_SET

    return m_pResultSet->GetStringByIndex(nInd, pc);
}

bool CQueryHelper::GetBinary(int nInd, void*& pData, int& nSize) const
{
    CHECK_CONNECTION
    CHECK_RESULT_SET

    size_t st = 0;
    bool bSuccess = m_pResultSet->GetBinaryByIndex(nInd, (const byte*&) pData, st);
    nSize = st;
    return bSuccess;
}

bool CQueryHelper::GetDate(int nInd, time_t& tVal) const
{
    CHECK_CONNECTION
        CHECK_RESULT_SET

    return m_pResultSet->GetDateByIndex(nInd, tVal);
}

bool CQueryHelper::IsNull(const char* pcCol) const
{
    CHECK_CONNECTION
        CHECK_RESULT_SET

    return m_pResultSet->IsNullByName(pcCol);
}

bool CQueryHelper::GetInt(const char* pcCol, int8& nVal) const
{
    CHECK_CONNECTION
        CHECK_RESULT_SET

    return m_pResultSet->GetInt8ByName(pcCol, nVal);
}

bool CQueryHelper::GetInt(const char* pcCol, int16& nVal) const
{
    CHECK_CONNECTION
        CHECK_RESULT_SET

    return m_pResultSet->GetInt16ByName(pcCol, nVal);
}

bool CQueryHelper::GetInt(const char* pcCol, int32& nVal) const
{
    CHECK_CONNECTION
        CHECK_RESULT_SET

    return m_pResultSet->GetInt32ByName(pcCol, nVal);
}
bool CQueryHelper::GetInt(const char* pcCol, int64& nVal) const
{
    CHECK_CONNECTION
        CHECK_RESULT_SET

    return m_pResultSet->GetInt64ByName(pcCol, nVal);
}

bool CQueryHelper::GetInt(const char* pcCol, uint8& nVal) const
{
    CHECK_CONNECTION
        CHECK_RESULT_SET

    return m_pResultSet->GetUInt8ByName(pcCol, nVal);
}

bool CQueryHelper::GetInt(const char* pcCol, uint16& nVal) const
{
    CHECK_CONNECTION
        CHECK_RESULT_SET

    return m_pResultSet->GetUInt16ByName(pcCol, nVal);
}

bool CQueryHelper::GetInt(const char* pcCol, uint32& nVal) const
{
    CHECK_CONNECTION
        CHECK_RESULT_SET

    return m_pResultSet->GetUInt32ByName(pcCol, nVal);
}

bool CQueryHelper::GetInt(const char* pcCol, uint64& nVal) const
{
    CHECK_CONNECTION
        CHECK_RESULT_SET

    return m_pResultSet->GetUInt64ByName(pcCol, nVal);
}

bool CQueryHelper::GetFloat(const char* pcCol, float& fVal) const
{
    CHECK_CONNECTION
        CHECK_RESULT_SET

    return m_pResultSet->GetFloatByName(pcCol, fVal);
}

bool CQueryHelper::GetDouble(const char* pcCol, double& dVal) const
{
    CHECK_CONNECTION
        CHECK_RESULT_SET

    return m_pResultSet->GetDoubleByName(pcCol, dVal);
}

bool CQueryHelper::GetString(const char* pcCol, const char*& pc) const
{
    CHECK_CONNECTION
        CHECK_RESULT_SET

    return m_pResultSet->GetStringByName(pcCol, pc);
}

bool CQueryHelper::GetBinary(const char* pcCol, void*& pData, int& nSize) const
{
    CHECK_CONNECTION
    CHECK_RESULT_SET

    size_t st = 0;
    bool bSuccess = m_pResultSet->GetBinaryByName(pcCol, (const byte*&) pData, st);
    nSize = st;
    return bSuccess;
}

bool CQueryHelper::GetDate(const char* pcCol, time_t& tVal) const
{
    CHECK_CONNECTION
        CHECK_RESULT_SET

    return m_pResultSet->GetDateByName(pcCol, tVal);
}

EDBError CQueryHelper::GetLastError() const
{
    if (m_pConnection)
    {
        return m_pConnection->GetLastError();
    }
    else
    {
        return m_Error;
    }
}

const char* CQueryHelper::GetRawErrorMessage() const
{
    if (m_pConnection)
    {
        return m_pConnection->GetRawErrorMessage();
    }
    else
    {
        return DBErrorMessage(m_Error);
    }
}
