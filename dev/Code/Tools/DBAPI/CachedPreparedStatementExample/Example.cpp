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

#include "platform_impl.h"
#include "../DBAPI.h"
#include "../DBAPIHelper.h"
#include "QueryDef.h"

const char* host = "localhost";
const char* db = "crytektest";
const char* user = "crytektest";
const char* password = "crytek";

int CreateExampleTable()
{
    ICachedDBConnection* CachedConnection = DriverManager::CreateCachedConnection(eDT_MySQL,
            host, 0, db, user, password,
            NULL, NULL);

    IDBStatement* Statement = CachedConnection->GetStatement();

    if (!Statement->Execute("DROP TABLE IF EXISTS dummy_table"))
    {
        fprintf(stderr, "[error] Can't drop dummy_table\n");
        fprintf(stderr, "[error] %s\n", CachedConnection->GetRawErrorMessage());
        return -101;
    }

    if (!Statement->Execute("CREATE TABLE dummy_table (intValue int, stringValue varchar(45), doubleValue double)"))
    {
        fprintf(stderr, "[error] Can't create dummy_table\n");
        fprintf(stderr, "[error] %s\n", CachedConnection->GetRawErrorMessage());
        return -101;
    }

    DriverManager::DestroyCachedConnection(CachedConnection);
    return 0;
}

int ExecExampleQuery()
{
    int OutIntValue;
    const char* OutStringValue = NULL;
    double OutDoubleValue;
    IDBResultSet* ResultSet;
    ICachedDBConnection* CachedConnection = NULL;
    IDBPreparedStatement* Statement = NULL;
    IDBResultSet* ResultSt = NULL;

    IQueryMapper* QM = GetQueryMapper<TEST_EXAMPLE_PREPARED_QUERIES>();
    CachedConnection = DriverManager::CreateCachedConnection(eDT_MySQL,
            host, 0, db, user, password,
            QM,                             // GetQueryMapper<TEST_PREPARED_QUERIES>()
            NULL);

    if (CachedConnection == NULL)
    {
        fprintf(stderr, "[error] Can't create ICachedConnection\n");
        fprintf(stderr, "[error] %s\n", CachedConnection->GetRawErrorMessage());
        return -1;
    }
    Statement = CachedConnection->GetPreparedStatement(PINSERT1);
    if (Statement == NULL)
    {
        fprintf(stderr, "[error] Can't get IDBPreparedStatement(%s) from ICachedConnection\n", QM->GetQuery(PINSERT1));
        fprintf(stderr, "[error] %s\n", CachedConnection->GetRawErrorMessage());
        return -2;
    }
    if (!DBAPIHelper::Exec(Statement, 1, "success1", 1.0) ||
        !DBAPIHelper::Exec(Statement, 2, "success2", 2.0) ||
        !DBAPIHelper::Exec(Statement, 3, "success3", 3.0) ||
        !DBAPIHelper::Exec(Statement, 4, "success4", 4.0) ||
        !DBAPIHelper::Exec(Statement, 5, "success5", 5.0))
    {
        fprintf(stderr, "[error] Can't insert values to DBMS");
        fprintf(stderr, "[error] %s\n", CachedConnection->GetRawErrorMessage());
        return -3;
    }

    if (Statement->GetUpdateCount() != 1)
    {
        fprintf(stderr, "[error]  Can't insert values to DBMS");
        fprintf(stderr, "[error] %s\n", CachedConnection->GetRawErrorMessage());
        return -4;
    }

    Statement = CachedConnection->GetPreparedStatement(PSELECT_ALL);
    if (Statement == NULL)
    {
        fprintf(stderr, "[error] Can't get IDBPreparedStatement(%s) from ICachedConnection\n", QM->GetQuery(PSELECT_ALL));
        fprintf(stderr, "[error] %s\n", CachedConnection->GetRawErrorMessage());
        return -2;
    }
    for (int i = 1; i <= 4; i += 3)
    {
        fprintf(stdout, "IntValue=%d, %s\n", i, QM->GetQuery(PSELECT_ALL));
        fprintf(stdout, "====\n");
        if (!DBAPIHelper::Exec(Statement, i))
        {
            fprintf(stderr, "[error] Can't select values from IDBPreparedStatement(%s)\n", QM->GetQuery(PSELECT_ALL));
            fprintf(stderr, "[error] %s\n", CachedConnection->GetRawErrorMessage());
            return -3;
        }
#if _DEBUG
        _ASSERTE(_CrtCheckMemory());
#endif
        ResultSet = Statement->GetResultSet();
        if (ResultSet == NULL)
        {
            fprintf(stderr, "[error] Can't get ResultSet\n");
            fprintf(stderr, "[error] %s\n", CachedConnection->GetRawErrorMessage());
            return -5;
        }
        while (ResultSet->Next())
        {
            if (!DBAPIHelper::Get(ResultSet, OutIntValue, OutStringValue, OutDoubleValue))
            {
                fprintf(stderr, "[error] Can't get values from ResultSet\n");
                fprintf(stderr, "[error] %s\n", CachedConnection->GetRawErrorMessage());
                return -6;
            }
            fprintf(stdout, "Int:%d, String:%s, Double:%f\n", OutIntValue, OutStringValue, OutDoubleValue);
        }
        fprintf(stdout, "====\n");
        Statement->CloseResultSet();
    }

    DriverManager::DestroyCachedConnection(CachedConnection);
    return 0;
}

int ExecExample()
{
    int Ret = CreateExampleTable();
    if (Ret != 0)
    {
        return Ret;
    }

    return ExecExampleQuery();
}

extern int ExecTest();

int main(int argc, char* argv[])
{
    ExecExample();

    ExecTest();
}