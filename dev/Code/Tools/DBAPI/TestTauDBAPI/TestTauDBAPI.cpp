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

const char* host = "devnet";
const char* db = "sw_a12s";
const char* user = "tau";
const char* password = "tau";
ICachedDBConnection* CachedConnection = NULL;
IQueryMapper* pMapper;
char chLog[2048];

void ConnectCachedToDB()
{
    pMapper = GetQueryMapper<SW_STATEMENTS>();
    assert(pMapper);

    CachedConnection = DriverManager::CreateCachedConnection(eDT_MySQL,
            host, 0, db, user, password,
            pMapper, // GetQueryMapper<SW_STATEMENTS>()
            NULL);

    if (CachedConnection == NULL)
    {
        fprintf(stderr, "[error] Can't create ICachedConnection\n");
        return;
    }
}

void DisconnectCachedFromDB()
{
    if (CachedConnection)
    {
        DriverManager::DestroyCachedConnection(CachedConnection);
    }
    CachedConnection = NULL;
}

int ExecPrepStmtExample(int st)
{
    if (!CachedConnection)
    {
        return -1;
    }

    _ASSERTE(_CrtCheckMemory());

    IDBPreparedStatement* pStatement = NULL;

    char chStatement[64];
    char chQuery[1024];
    switch (st)
    {
    case 0:
        pStatement = CachedConnection->GetPreparedStatement(ST_1);
        sprintf(chStatement, "ST_1");
        sprintf(chQuery, pMapper->GetQuery(ST_1));
        break;
    case 1:
        pStatement = CachedConnection->GetPreparedStatement(ST_2);
        sprintf(chStatement, "ST_2");
        sprintf(chQuery, pMapper->GetQuery(ST_2));
        break;
    case 2:
        pStatement = CachedConnection->GetPreparedStatement(ST_3);
        sprintf(chStatement, "ST_3");
        sprintf(chQuery, pMapper->GetQuery(ST_3));
        break;
    }

    sprintf(chLog, "Query: %d, \"%s\"\n", st, chQuery);
    OutputDebugString(chLog);

    _ASSERTE(_CrtCheckMemory());

    if (pStatement == NULL)
    {
        fprintf(stderr, "[error] Can't get IDBPreparedStatement(%d) from ICachedConnection\n", st);
        fprintf(stderr, "[error] %s\n", CachedConnection->GetRawErrorMessage());
        return -2;
    }

    if (!DBAPIHelper::Exec(pStatement, "a12s"))
    {
        fprintf(stderr, "%s query failed.\n", chStatement);
        return -3;
    }

    _ASSERTE(_CrtCheckMemory());

    //IDBResultSet *pResultSet = pStatement->GetResultSet();
    IDBResultSetPtr pResultSet = pStatement->GetResultSet();

    if (!pResultSet || !pResultSet->Next())
    {
        fprintf(stderr, "%s query returned 0 rows.\n", chStatement);
        return -4;
    }

    _ASSERTE(_CrtCheckMemory());

    int nLockedBy;
    int64 nHRID[3];
    if (!DBAPIHelper::Get(pResultSet, nLockedBy, nHRID[0], nHRID[1], nHRID[2]))
    {
        fprintf(stderr, "%s query returned 0 rows.\n", chStatement);
        return -5;
    }

    _ASSERTE(_CrtCheckMemory());

    return 0;
}

int main(int argc, char* argv[])
{
    ConnectCachedToDB();
    for (int j = 0; j < 1000; j++)
    {
        sprintf(chLog, "LOOP: %d\n", j);
        OutputDebugString(chLog);
        for (int i = 0; i < 3; i++)
        {
            if (ExecPrepStmtExample(i) < 0)
            {
                OutputDebugString("Prepared Stmt Execution Failed\n");
            }
        }
    }
    DisconnectCachedFromDB();
    return 0;
}