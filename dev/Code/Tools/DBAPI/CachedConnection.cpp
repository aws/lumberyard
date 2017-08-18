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
#include "DBAPI.h"
#include "CachedConnection.h"
#include "Util/DBUtil.h"

class CCachedDBConnection
    : public ICachedDBConnection
{
public:
    CCachedDBConnection(IDBConnection* DBConnection);

    virtual ~CCachedDBConnection();

    virtual bool BuildCache(IQueryMapper* PreparedStatementQueryMapper,
        IQueryMapper* CallableStatementQueryMapper);

    virtual IDBConnection* GetDBConnection() const;

    virtual unsigned long  ClientVersion() const;

    virtual unsigned long  ServerVersion() const;

    virtual bool           Ping();

    virtual bool           Commit();

    virtual bool           Rollback();

    virtual bool           GetAutoCommit() const;

    virtual bool           SetAutoCommit(bool bAutoCommit);

    virtual IDBStatement*  GetStatement();

    virtual IDBPreparedStatement* GetPreparedStatement(QueryIDType QueryID);

    virtual IDBCallableStatement* GetCallableStatement(QueryIDType QueryID);

    virtual EDBError       GetLastError() const;

    virtual const char*    GetRawErrorMessage() const;

protected:

    IDBConnection*                      m_DBConnection;
    IDBStatement*                       m_CachedDBStatement;
    std::vector<IDBPreparedStatement*>  m_CachedPreparedStatement;
    std::vector<IDBCallableStatement*>  m_CachedCallableStatement;
    StringStorage                       m_LastErrorString;
};

struct SQueryMapper
    : public IQueryMapper
{
public:
    SQueryMapper()
        : m_LastEmptyIndex(0) {}

    bool Register(QueryIDType QueryID, const char* QueryString);

    QueryIDType Register(const char* QueryString);

    const char* GetQuery(QueryIDType QueryID) const
    {
        if (QueryID < m_QueryStringArray.size())
        {
            return m_QueryStringArray[QueryID].ToString();
        }
        return NULL;
    }

    QueryIDType GetMaxQueryID() const
    {
        return static_cast<QueryIDType>(m_QueryStringArray.size()) - 1;
    }

protected:
    typedef std::vector< StringStorage > QueryStringArrayType;

    QueryIDType           m_LastEmptyIndex;
    QueryStringArrayType  m_QueryStringArray;
};

QueryIDType SQueryMapper::Register(const char* QueryString)
{
    QueryIDType QueryID = m_LastEmptyIndex;

    // incremental
    if (m_LastEmptyIndex == m_QueryStringArray.size())
    {
        m_QueryStringArray.push_back(QueryString);
        m_LastEmptyIndex++; // = m_QueryStringArray.size()
    }
    else
    {
        for (; QueryID < m_QueryStringArray.size(); QueryID++)
        {
            if (m_QueryStringArray[QueryID].Empty())
            {
                m_QueryStringArray[QueryID] = QueryString;
                m_LastEmptyIndex = QueryID + 1;
                break;
            }
        }

        // New
        if (QueryID == m_QueryStringArray.size())
        {
            m_QueryStringArray.push_back(QueryString);
            m_LastEmptyIndex = QueryID + 1; // = m_QueryStringArray.size()
        }
    }
    return QueryID;
}

bool SQueryMapper::Register(QueryIDType QueryID, const char* QueryString)
{
    if (QueryID < m_QueryStringArray.size() && !m_QueryStringArray[QueryID].Empty())
    {
        return false;
    }

    if (m_QueryStringArray.size() == QueryID)
    {
        m_QueryStringArray.push_back(QueryString);
    }
    else
    {
        while (m_QueryStringArray.size() < QueryID)
        {
            m_QueryStringArray.push_back(QueryStringArrayType::value_type());
        }

        m_QueryStringArray.push_back(QueryString);
    }
    return true;
}

SQueryMapperWrapper::SQueryMapperWrapper()
{
    m_QueryMapper = new SQueryMapper;
}

SQueryMapperWrapper::~SQueryMapperWrapper()
{
    delete m_QueryMapper;
}

IQueryMapper* SQueryMapperWrapper::GetQueryMapper() const
{
    return m_QueryMapper;
}


CCachedDBConnection::CCachedDBConnection(IDBConnection* DBConnection)
    : m_DBConnection(DBConnection)
{
    m_CachedDBStatement = m_DBConnection->CreateStatement();
}

CCachedDBConnection::~CCachedDBConnection()
{
    // do nothing

    //if( m_DBConnection )
    //    DriverManager::DestroyConnection(m_DBConnection);
}

unsigned long CCachedDBConnection::ClientVersion() const
{
    return m_DBConnection->ClientVersion();
}

unsigned long CCachedDBConnection::ServerVersion() const
{
    return m_DBConnection->ServerVersion();
}

bool CCachedDBConnection::Ping()
{
    return m_DBConnection->Ping();
}

bool CCachedDBConnection::Commit()
{
    return m_DBConnection->Commit();
}

bool CCachedDBConnection::Rollback()
{
    return m_DBConnection->Rollback();
}

bool CCachedDBConnection::GetAutoCommit() const
{
    return m_DBConnection->GetAutoCommit();
}

bool CCachedDBConnection::SetAutoCommit(bool bAutoCommit)
{
    return m_DBConnection->GetAutoCommit();
}

IDBStatement* CCachedDBConnection::GetStatement()
{
    return m_CachedDBStatement;
}

IDBPreparedStatement* CCachedDBConnection::GetPreparedStatement(QueryIDType QueryID)
{
    IDBPreparedStatement* Statement = NULL;
    if (QueryID < static_cast<QueryIDType>(m_CachedPreparedStatement.size()))
    {
        Statement = m_CachedPreparedStatement[QueryID];
    }
    return Statement;
}

IDBCallableStatement* CCachedDBConnection::GetCallableStatement(QueryIDType QueryID)
{
    IDBCallableStatement* Statement = NULL;
    if (QueryID < static_cast<QueryIDType>(m_CachedCallableStatement.size()))
    {
        Statement = m_CachedCallableStatement[QueryID];
    }
    return Statement;
}

EDBError CCachedDBConnection::GetLastError() const
{
    return m_DBConnection->GetLastError();
}

const char* CCachedDBConnection::GetRawErrorMessage() const
{
    if (m_LastErrorString.Length() > 0)
    {
        return m_LastErrorString.ToString();
    }
    return m_DBConnection->GetRawErrorMessage();
}

IDBConnection* CCachedDBConnection::GetDBConnection() const
{
    return m_DBConnection;
}

bool CCachedDBConnection::BuildCache(IQueryMapper* PreparedStatementQueryMapper,
    IQueryMapper* CallableStatementQueryMapper)
{
    m_LastErrorString.Clear();
    if (PreparedStatementQueryMapper)
    {
        QueryIDType MaxQueryID = PreparedStatementQueryMapper->GetMaxQueryID();
        for (QueryIDType i = 0; i <= MaxQueryID; i++)
        {
            IDBPreparedStatement* PreparedStatement = NULL;
            const char* QueryStr = PreparedStatementQueryMapper->GetQuery(i);
            if (QueryStr)
            {
                PreparedStatement = m_DBConnection->CreatePreparedStatement(QueryStr);
                if (!PreparedStatement)
                {
                    m_LastErrorString = QueryStr;
                    return false;
                }
            }
            m_CachedPreparedStatement.push_back(PreparedStatement);
        }
    }

    if (CallableStatementQueryMapper)
    {
        QueryIDType MaxQueryID = CallableStatementQueryMapper->GetMaxQueryID();
        for (QueryIDType i = 0; i <= MaxQueryID; i++)
        {
            IDBCallableStatement* CallableStatement = NULL;
            const char* QueryStr = CallableStatementQueryMapper->GetQuery(i);
            if (QueryStr)
            {
                CallableStatement = m_DBConnection->CreateCallableStatement(QueryStr);
                if (!CallableStatement)
                {
                    m_LastErrorString = QueryStr;
                    return false;
                }
            }
            m_CachedCallableStatement.push_back(CallableStatement);
        }
    }
    return true;
}

ICachedDBConnection* DriverManager::CreateCachedConnection(IDBConnection* DBConnection,
    IQueryMapper* PreparedStatemetQuery,
    IQueryMapper* CallableStatemetQuery)
{
    ICachedDBConnection* Cache = NULL;
    if (DBConnection)
    {
        Cache = new CCachedDBConnection(DBConnection);
        if (!Cache->BuildCache(PreparedStatemetQuery, CallableStatemetQuery))
        {
            delete Cache;
            Cache = NULL;
        }
    }
    return Cache;
}

ICachedDBConnection* DriverManager::CreateCachedConnection(const char* ConnectionString,
    IQueryMapper* PreparedStatemetQuery,
    IQueryMapper* CallableStatemetQuery,
    const char* LocalPath)
{
    IDBConnection* DBConnection = DriverManager::CreateConnection(ConnectionString, LocalPath);
    return CreateCachedConnection(DBConnection, PreparedStatemetQuery, CallableStatemetQuery);
}

ICachedDBConnection* DriverManager::CreateCachedConnection(EDBType DBType,
    const char* Host,
    unsigned short Port,
    const char* DefaultDBName,
    const char* UserID,
    const char* Password,
    const char* LocalPath,
    IQueryMapper* PreparedStatemetQuery,
    IQueryMapper* CallableStatemetQuery)
{
    IDBConnection* DBConnection = DriverManager::CreateConnection(DBType, Host, Port, DefaultDBName, UserID, Password, LocalPath);
    return CreateCachedConnection(DBConnection, PreparedStatemetQuery, CallableStatemetQuery);
}

void DriverManager::DestroyCachedConnection(ICachedDBConnection* CachedConnection)
{
    if (CachedConnection)
    {
        IDBConnection* DBConnection = CachedConnection->GetDBConnection();
        if (DBConnection)
        {
            DriverManager::DestroyConnection(DBConnection);
        }
        delete CachedConnection;
    }
}