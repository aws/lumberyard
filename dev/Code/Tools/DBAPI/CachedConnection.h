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

#ifndef CRYINCLUDE_TOOLS_DBAPI_CACHEDCONNECTION_H
#define CRYINCLUDE_TOOLS_DBAPI_CACHEDCONNECTION_H
#pragma once


#include "DBTypes.h"

// forward declaration
struct IQueryMapper;
class IDBConnection;
class IDBStatement;
class IDBPreparedStatement;
class IDBCallableStatement;

class ICachedDBConnection
{
public:
    virtual ~ICachedDBConnection() {}

    virtual bool BuildCache(IQueryMapper* PreparedStatementQueryMapper,
        IQueryMapper* CallableStatementQueryMapper) = 0;

    virtual IDBConnection* GetDBConnection() const = 0;

    virtual unsigned long  ClientVersion() const = 0;

    virtual unsigned long  ServerVersion() const = 0;

    virtual bool           Ping() = 0;

    virtual bool           Commit() = 0;

    virtual bool           Rollback() = 0;

    virtual bool           GetAutoCommit() const = 0;

    virtual bool           SetAutoCommit(bool bAutoCommit) = 0;

    virtual IDBStatement*  GetStatement() = 0;

    virtual IDBPreparedStatement*  GetPreparedStatement(QueryIDType QueryID) = 0;

    virtual IDBCallableStatement*  GetCallableStatement(QueryIDType QueryID) = 0;

    virtual EDBError       GetLastError() const = 0;

    virtual const char*    GetRawErrorMessage() const = 0;
};

struct IQueryMapper
{
public:
    virtual ~IQueryMapper() {}

    virtual bool Register(QueryIDType QueryID, const char* QueryString) = 0;

    virtual QueryIDType Register(const char* QueryString) = 0;

    virtual const char* GetQuery(QueryIDType QueryID) const = 0;

    virtual QueryIDType GetMaxQueryID() const = 0;
};

struct SQueryMapperWrapper
{
public:
    SQueryMapperWrapper();
    ~SQueryMapperWrapper();

    IQueryMapper* GetQueryMapper() const;

private:
    IQueryMapper* m_QueryMapper;
};

template<typename QueryGroup>
IQueryMapper* GetQueryMapper();


#define QUERY_DEF_IMPL_START(QUERY_GROUP)        \
    template<>                                   \
    IQueryMapper * GetQueryMapper<QUERY_GROUP>() \
    {                                            \
        static CryCriticalSection _Lock;         \
        AUTO_LOCK(_Lock);                        \
        static SQueryMapperWrapper _Wrapper;     \
        IQueryMapper* QueryMapper = _Wrapper.GetQueryMapper();

#define QUERY_DEF_IMPL_END() \
    return QueryMapper;      \
    }

#define QUERY_DEF_IMPL_REGISTER(QueryID, QueryStr) \
    QueryMapper->Register(QueryID, QueryStr);

#endif // CRYINCLUDE_TOOLS_DBAPI_CACHEDCONNECTION_H
