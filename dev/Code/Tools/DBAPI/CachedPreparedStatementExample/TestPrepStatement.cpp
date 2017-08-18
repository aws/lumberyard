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

#include "platform.h"
#include "../DBAPI.h"
#include "../DBAPIHelper.h"
#include "QueryDef.h"
#include "CryString.h"

extern const char* host;
extern const char* db;
extern const char* user;
extern const char* password;

const char* GDBstring = NULL;

int CreateTestTable()
{
    IDBConnection* Connection = DriverManager::CreateConnection(eDT_MySQL, host, 0, db, user, password);
    IDBStatement* Statement = Connection->CreateStatement();

    if (!Statement->Execute("DROP TABLE IF EXISTS GDStatus"))
    {
        fprintf(stderr, "[error] Can't drop dummy_table\n");
        fprintf(stderr, "[error] %s\n", Connection->GetRawErrorMessage());
        return -101;
    }

    if (!Statement->Execute("DROP TABLE IF EXISTS GDData"))
    {
        fprintf(stderr, "[error] Can't drop dummy_table\n");
        fprintf(stderr, "[error] %s\n", Connection->GetRawErrorMessage());
        return -101;
    }

    if (!Statement->Execute("CREATE TABLE GDStatus (GD_LockedBy INT, Name VARCHAR(45))"))
    {
        fprintf(stderr, "[error] Can't create dummy_table\n");
        fprintf(stderr, "[error] %s\n", Connection->GetRawErrorMessage());
        return -101;
    }

    if (!Statement->Execute("CREATE TABLE GDData (ID INT, Name VARCHAR(45))"))
    {
        fprintf(stderr, "[error] Can't create dummy_table\n");
        fprintf(stderr, "[error] %s\n", Connection->GetRawErrorMessage());
        return -101;
    }
    Connection->DestroyStatement(Statement);

    IDBPreparedStatement* InsertStmt1 = Connection->CreatePreparedStatement("INSERT INTO GDStatus VALUES(?, ?)");
    IDBPreparedStatement* InsertStmt2 = Connection->CreatePreparedStatement("INSERT INTO GDData VALUES(?, ?)");
#ifdef _DEBUG
    _ASSERTE(InsertStmt1 && InsertStmt1);
#endif
    if (InsertStmt1 && InsertStmt2)
    {
        // Insert Dummy Data
        for (int i = 0; i < 100; i++)
        {
            //string Str;
            CryStringLocal Str;
            Str.Format("a%2ds", i);
            DBAPIHelper::Exec(InsertStmt1, (i % 2) + 1, Str.c_str());
            DBAPIHelper::Exec(InsertStmt2, i, Str.c_str());
        }

        Connection->DestroyStatement(InsertStmt1);
        Connection->DestroyStatement(InsertStmt2);
    }

    DriverManager::DestroyConnection(Connection);
    return 0;
}

void TestPrepStatement(ICachedDBConnection* pConnection, QueryIDType query)
{
    IDBPreparedStatement* pStatement = pConnection->GetPreparedStatement(query);
    bool bSuccess = DBAPIHelper::Exec(pStatement, "a12s");
    _ASSERTE(bSuccess);
    IDBResultSet* pResultSet = pStatement->GetResultSet();
    pResultSet->Next();
    int nLockedBy = 0;
    DBAPIHelper::Get(pResultSet, nLockedBy);
    pStatement->CloseResultSet();
}

int ExecTestQuery()
{
    IQueryMapper* pMapper = GetQueryMapper<SW_STATEMENTS>();

    ICachedDBConnection* pConnection =
        DriverManager::CreateCachedConnection(GDBstring, pMapper, pMapper);

    TestPrepStatement(pConnection, ST_1);
    TestPrepStatement(pConnection, ST_1);
    TestPrepStatement(pConnection, ST_3);
    TestPrepStatement(pConnection, ST_2);

    DriverManager::DestroyCachedConnection(pConnection);
    return 0;
}

int ExecTest()
{
    string dbstring;
    dbstring = string("mysql://") + host + "/" + db + "?" + "user=" + user + "&" + "password=" + password;
    GDBstring = dbstring.c_str();
    int Ret = CreateTestTable();
    if (Ret != 0)
    {
        return Ret;
    }

    return ExecTestQuery();
}

