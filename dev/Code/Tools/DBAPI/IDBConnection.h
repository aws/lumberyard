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

#ifndef CRYINCLUDE_TOOLS_DBAPI_IDBCONNECTION_H
#define CRYINCLUDE_TOOLS_DBAPI_IDBCONNECTION_H
#pragma once


#include "DBTypes.h"

class IDBStatement;
class IDBPreparedStatement;
class IDBCallableStatement;
class IDBConnection
{
public:
    virtual ~IDBConnection() {}

    virtual unsigned long ClientVersion() const = 0;

    virtual unsigned long ServerVersion() const = 0;

    virtual bool Ping() = 0;

    virtual bool Commit() = 0;

    virtual bool Rollback() = 0;

    virtual bool GetAutoCommit() const = 0;

    virtual bool SetAutoCommit(bool bAutoCommit) = 0;

    virtual IDBStatement*           CreateStatement() = 0;

    virtual IDBPreparedStatement*   CreatePreparedStatement(const char* Query) = 0;

    virtual IDBCallableStatement*   CreateCallableStatement(const char* Query) = 0;

    virtual bool                    DestroyStatement(IDBStatement* DBStatement) = 0;

    virtual EDBError                GetLastError() const = 0;

    virtual const char*             GetRawErrorMessage() const = 0;

    virtual bool                    IsLocal() const = 0;
};

#endif // CRYINCLUDE_TOOLS_DBAPI_IDBCONNECTION_H
