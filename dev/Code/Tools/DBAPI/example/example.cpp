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

bool StatementTest()
{
    IDBConnection* conn = DriverManager::CreateConnection(eDT_MySQL, "localhost", 0, "crytektest", "crytektest", "crytek");
    if (NULL == conn)
    {
        return false;
    }

    IDBStatement* stmt = conn->CreateStatement();
    if (NULL == stmt)
    {
        return false;
    }

    string tableName;
    tableName.Format("simle_test_%d", ::GetTickCount());

    string query;

    // create table
    query.Format("CREATE TABLE %s (intVal INT, floatVal FLOAT, stringVal VARCHAR(10))", tableName.c_str());
    stmt->Execute(query.c_str());

    // insert rows
    query.Format("INSERT INTO %s VALUES (1, 23, \'saadfaf\')", tableName.c_str());
    stmt->Execute(query.c_str());

    query.Format("INSERT INTO %s VALUES (3, 34.23434, \'gsdfsdf\')", tableName.c_str());
    stmt->Execute(query.c_str());

    query.Format("INSERT INTO %s VALUES (44, 1.34, \'jljljioj\')", tableName.c_str());
    stmt->Execute(query.c_str());

    // select rows
    query.Format("SELECT * FROM %s", tableName.c_str());
    stmt->Execute(query.c_str());

    IDBResultSet* rs = stmt->GetResultSet();
    while (true == rs->Next())
    {
        int intVal;
        float floatVal;
        const char* stringVal;

        // zero-based column index
        rs->GetInt32ByIndex(0, intVal);
        rs->GetFloatByIndex(1, floatVal);
        rs->GetStringByIndex(2, stringVal);

        printf("int = %d, float = %f, string = %s\n", intVal, floatVal, stringVal);
    }

    // drop table
    query.Format("DROP TABLE %s", tableName.c_str());
    stmt->Execute(query.c_str());

    conn->DestroyStatement(stmt);

    DriverManager::DestroyConnection(conn);

    return true;
}

bool PreparedStatementTest()
{
    IDBConnection* conn = DriverManager::CreateConnection(eDT_MySQL, "localhost", 0, "crytektest", "crytektest", "crytek");
    if (NULL == conn)
    {
        return false;
    }

    IDBStatement* stmt = conn->CreateStatement();
    if (NULL == stmt)
    {
        return false;
    }

    string tableName;
    tableName.Format("simle_test_%d", ::GetTickCount());

    string query;

    // create table
    query.Format("CREATE TABLE %s (intVal INT, floatVal FLOAT, stringVal VARCHAR(10))", tableName.c_str());
    stmt->Execute(query.c_str());

    // prepare queries
    query.Format("SELECT * FROM %s WHERE intVal = ?", tableName.c_str());
    IDBPreparedStatement* pstmtSelectOne = conn->CreatePreparedStatement(query.c_str());

    // insert rows
    query.Format("INSERT INTO %s VALUES (?, ?, ?)", tableName.c_str());
    IDBPreparedStatement* pstmtInsert = conn->CreatePreparedStatement(query.c_str());

    int intVal;
    float floatVal;
    char stringVal[10];
    size_t stringLength;

    pstmtInsert->SetInt32ByIndex(0, 1);
    pstmtInsert->SetFloatByIndex(1, 23.0f);
    pstmtInsert->SetStringByIndex(2, "afafsdfs", strlen("afafsdfs"));

    pstmtInsert->Execute();

    pstmtInsert->SetInt32ByIndex(0, 32);
    pstmtInsert->SetFloatByIndex(1, 34.34f);
    pstmtInsert->SetStringByIndex(2, "jlijhio", strlen("jlijhio"));

    pstmtInsert->Execute();

    pstmtInsert->SetInt32ByIndex(0, 8);
    pstmtInsert->SetFloatByIndex(1, 2.3f);
    pstmtInsert->SetStringByIndex(2, "youo", strlen("youo"));

    pstmtInsert->Execute();

    /*
    query.Format("INSERT INTO %s VALUES (1, 23, \'\')", tableName.c_str());
    stmt->Execute(query.c_str());

    query.Format("INSERT INTO %s VALUES (3, 34.23434, \'gsdfsdf\')", tableName.c_str());
    stmt->Execute(query.c_str());

    query.Format("INSERT INTO %s VALUES (44, 1.34, \'jljljioj\')", tableName.c_str());
    stmt->Execute(query.c_str());
    */

    // select rows
    query.Format("SELECT * FROM %s", tableName.c_str());
    IDBPreparedStatement* pstmtSelectAll = conn->CreatePreparedStatement(query.c_str());
    pstmtSelectAll->Execute();

    IDBResultSet* rsSelectAll = pstmtSelectAll->GetResultSet();
    while (true == rsSelectAll->Next())
    {
        int intVal;
        float floatVal;
        const char* stringVal;

        // zero-based column index
        rsSelectAll->GetInt32ByIndex(0, intVal);
        rsSelectAll->GetFloatByIndex(1, floatVal);
        rsSelectAll->GetStringByIndex(2, stringVal);

        printf("int = %d, float = %f, string = %s\n", intVal, floatVal, stringVal);
    }

    // drop table
    query.Format("DROP TABLE %s", tableName.c_str());
    stmt->Execute(query.c_str());

    conn->DestroyStatement(stmt);

    DriverManager::DestroyConnection(conn);

    return true;
}

int main(int argc, char** argv)
{
    StatementTest();
    PreparedStatementTest();

    return EXIT_SUCCESS;
}