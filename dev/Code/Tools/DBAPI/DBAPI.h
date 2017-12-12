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

#ifndef CRYINCLUDE_TOOLS_DBAPI_DBAPI_H
#define CRYINCLUDE_TOOLS_DBAPI_DBAPI_H
#pragma once


#include "DBTypes.h"
#include "IDBConnection.h"
#include "IDBStatement.h"
#include "IDBResultSet.h"
#include "CachedConnection.h"

class DriverManager
{
public:
    static IDBConnection*   CreateConnection(const char* ConnectionString, const char* LocalPath);
    static IDBConnection*   CreateConnection(EDBType DBType,
        const char* Host,
        unsigned short Port,
        const char* DefaultDBName,
        const char* UserID,
        const char* Password,
        const char* LocalPath);

    static void             DestroyConnection(IDBConnection* DBConnection);

    // for CCachedDBConnection
    static ICachedDBConnection* CreateCachedConnection(IDBConnection* DBConnection,
        IQueryMapper* PreparedStatemetQuery,
        IQueryMapper* CallableStatemetQuery);

    static void                 DestroyCachedConnection(ICachedDBConnection* CachedConnection);

    static ICachedDBConnection* CreateCachedConnection(const char* ConnectionString,
        IQueryMapper* PreparedStatemetQuery,
        IQueryMapper* CallableStatemetQuery,
        const char* LocalPath);

    static ICachedDBConnection* CreateCachedConnection(EDBType DBType,
        const char* Host,
        unsigned short Port,
        const char* DefaultDBName,
        const char* UserID,
        const char* Password,
        const char* LocalPath,
        IQueryMapper* PreparedStatemetQuery,
        IQueryMapper* CallableStatemetQuery);

    static bool DoesDatabaseExist(IDBConnection* DBConnection, EDBType DBType, const char* DBName);
};

const char* DBErrorMessage(EDBError Error);

#endif // CRYINCLUDE_TOOLS_DBAPI_DBAPI_H
