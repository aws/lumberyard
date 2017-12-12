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
#include "SQLite/SQLiteConnection.h"

#include <AzCore/base.h>

#ifdef _WIN32
#define strnicmp _strnicmp
#endif

class DriverManagerImpl
{
public:
    static CSQLiteConnection* CreateSQLiteConnection()
    {
        CSQLiteConnection* NewConnection = new CSQLiteConnection();
        return NewConnection;
    }
    static void DestroyConnection(IDBConnection* DBConnection)
    {
        if (DBConnection != NULL)
        {
            delete DBConnection;
            DBConnection = NULL;
        }
    }

    static bool DoesDatabaseExist(IDBConnection* DBConnection, EDBType DBType, const char* DBName)
    {
        if (!DBConnection)
        {
            return false;
        }

        static const uint32 querySize = 1024;
        char strQuery[querySize];
        memset(strQuery, '\0', sizeof(strQuery));

        switch (DBType)
        {
        case eDT_SQLite:
            azsnprintf(strQuery, querySize, "%s", "SELECT * FROM sqlite_master");
            break;
        default:
            azsnprintf(strQuery, querySize, "SELECT `SCHEMA_NAME` FROM `INFORMATION_SCHEMA`.`SCHEMATA` WHERE `SCHEMA_NAME`=\"%s\"", DBName);
            break;
        }

        bool ret = false;
        IDBStatement* pStatement = DBConnection->CreateStatement();
        if (pStatement)
        {
            if (pStatement->Execute(strQuery))
            {
                IDBResultSet* results = pStatement->GetResultSet();
                if (results)
                {
                    if (results->Next())
                    {
                        ret = results->GetNumRows() > 0;
                    }

                    pStatement->CloseResultSet();
                }
            }
            DBConnection->DestroyStatement(pStatement);
        }
        return ret;
    }
};

IDBConnection* DriverManager::CreateConnection(const char* ConnectionString, const char* LocalPath)
{
    bool bConnect = false;
    IDBConnection* NewConnection = NULL;
    const char SQLiteProtocolName[] = "sqlite:";
    auto SQLiteProtocolLength = strlen(SQLiteProtocolName);

    if (strnicmp(ConnectionString, SQLiteProtocolName, SQLiteProtocolLength) == 0)
    {
        CSQLiteConnection* SQLiteConnection = DriverManagerImpl::CreateSQLiteConnection();
        bConnect = SQLiteConnection->Connect(ConnectionString + SQLiteProtocolLength, LocalPath);
        NewConnection = SQLiteConnection;
    }

    if (NewConnection != NULL && bConnect == false)
    {
        DriverManagerImpl::DestroyConnection(NewConnection);
        NewConnection = NULL;
    }
    return NewConnection;
}

IDBConnection* DriverManager::CreateConnection(EDBType DBType,
    const char* Host,
    unsigned short Port,
    const char* DefaultDBName,
    const char* UserID,
    const char* Password,
    const char* LocalPath)
{
    bool bConnect = false;
    IDBConnection* NewConnection = NULL;
    if (DBType == eDT_SQLite)
    {
        CSQLiteConnection* SQLiteConnection = DriverManagerImpl::CreateSQLiteConnection();
        bConnect = SQLiteConnection->Connect(DefaultDBName, LocalPath);
        NewConnection = SQLiteConnection;
    }

    if (NewConnection != NULL && bConnect == false)
    {
        DriverManagerImpl::DestroyConnection(NewConnection);
        NewConnection = NULL;
    }
    return NewConnection;
}

void DriverManager::DestroyConnection(IDBConnection* DBConnection)
{
    DriverManagerImpl::DestroyConnection(DBConnection);
}

bool DriverManager::DoesDatabaseExist(IDBConnection* pConnection, EDBType DBType, const char* DBName)
{
    return DriverManagerImpl::DoesDatabaseExist(pConnection, DBType, DBName);
}

struct ErrorMessage
{
    EDBError    Errno;
    const char* Message;
};

static ErrorMessage ErrorMessageTable[] =
{
    { eDE_SUCCESS,                      "eDE_SUCCESS" },
    { eDB_UNKNOWN,                      "eDB_UNKNOWN" },
    { eDB_OUT_OF_MEMORY,                "eDB_OUT_OF_MEMORY" },
    { eDB_INVALID_CONNECTION_STRING,    "eDB_INVALID_CONNECTION_STRING" },
    { eDB_NOT_CONNECTED,                "eDB_NOT_CONNECTED" },
    { eDB_NOT_SUPPORTED,                "eDB_NOT_SUPPORTED" },
    { eDB_INVALID_POINTER,              "eDB_INVALID_POINTER" },
    { eDB_INVALID_COLUMN_NAME,          "eDB_INVALID_COLUMN_NAME"},
    { eDB_CLOSED_RESULTSET,             "eDB_CLOSED_RESULTSET"},
    { eDB_CLOSED_STATEMENT,             "eDB_CLOSED_STATEMENT"},
    { eDB_INVALID_COLUMN_INDEX,         "eDB_INVALID_COLUMN_INDEX"},
    { eDB_INVALID_PARAM_INDEX,          "eDB_INVALID_PARAM_INDEX"},
    { eDB_VALUE_IS_NULL,                "eDB_VALUE_IS_NULL"},
    { eDB_VALUE_CANNOT_CONVERT,         "eDB_VALUE_CANNOT_CONVERT"},
    { eDB_RESULT_IS_NOT_BOUND,          "eDB_RESULT_IS_NOT_BOUND"},
    { eDB_INVALID_TYPE,                 "eDB_INVALID_TYPE"},
};


const char* DBErrorMessage(EDBError Errno)
{
    const int ErrorMessageTableSize = sizeof(ErrorMessageTable) / sizeof(ErrorMessageTable[0]);
    for (int i = 0; i < ErrorMessageTableSize; i++)
    {
        if (ErrorMessageTable[i].Errno == Errno)
        {
            return ErrorMessageTable[i].Message;
        }
    }
    return "Error number is invalid";
}

