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
#include "DBAPIHelper.h"
#include "StringUtil.h"
#include "TestCaseUtil.h"

const char* FileName = "sqlitetest.db";

/// TODO mix FileName with inner DBName
struct SQLiteConnector
{
    static IDBConnection* Connect()
    {
        return DriverManager::CreateConnection(eDT_SQLite, NULL, NULL, FileName, NULL, NULL);
    }
};

struct SQLiteTableNameGenerator
    : public DefaultTableNameGenerator
{
    std::string& GenerateName(const char* Category, const char* Prefix)
    {
        DefaultTableNameGenerator::GenerateName(Category, Prefix);
        // Table names that begin with "sqlite_" are reserved for internal use.
        // It is an error to attempt to create a table with a name that starts with "sqlite_".
        if (_strnicmp(TableName.c_str(), "sqlite_", 7) == 0)
        {
            TableName.replace(0, 7, "SQ_LITE_");
        }
        return TableName;
    }
};

typedef DBAPI_TEST_BASE<SQLiteConnector> SQLITE_TEST_BASE;
typedef DBAPI_TEST_WITH_FIXED_TABLE<SQLiteConnector, SQLiteTableNameGenerator, DefaultSchemaQuery > SQLITE_TEST_WITH_FIXED_TABLE;
typedef DBAPI_TEST_WITH_TABLE_AND_DBSTATEMENT<SQLiteConnector, SQLiteTableNameGenerator, DefaultSchemaQuery > SQLITE_TEST_WITH_TABLE_AND_DBSTATEMENT;
typedef DBAPI_TEST_WITH_RANDOM_TABLENAME<SQLiteConnector, SQLiteTableNameGenerator> SQLITE_TEST_WITH_RANDOM_TABLENAME;

TEST(SUPPORT_DBAPI, SQLITE)
{
    IDBConnection* DBConnection = SQLiteConnector::Connect();
    ASSERT_TRUE(DBConnection != NULL);
    ASSERT_GE(DBConnection->ClientVersion(), 0x00030000u);
    EXPECT_GE(DBConnection->ServerVersion(), 0x00030000u);
    DriverManager::DestroyConnection(DBConnection);
}

TEST_F(SQLITE_TEST_BASE, Ping)
{
    ASSERT_TRUE(Connection->Ping());
}

TEST_F(SQLITE_TEST_BASE, CreateStatement)
{
    IDBStatement* DBStatement;
    DBStatement = Connection->CreateStatement();
    ASSERT_TRUE(DBStatement != NULL);
}

TEST_F(SQLITE_TEST_WITH_TABLE_AND_DBSTATEMENT, Insert)
{
    fm_string Query;
    Query.format("INSERT INTO %s VALUES(1, \'abcdefg\', 1.0)", TableName);
    ASSERT_TRUE(QueryStatement->Execute(*Query));

    Query.format("INSERT INTO %s VALUES(2, \'ùÓÙþ\', 2.0)", TableName);
    ASSERT_TRUE(QueryStatement->Execute(*Query));

    Query.format("INSERT INTO %s VALUES(5, \'ÇÑ±Û\', 5.0)", TableName);
    ASSERT_TRUE(QueryStatement->Execute(*Query));

    IDBResultSet* ResultSet = QueryStatement->GetResultSet();
    ASSERT_TRUE(ResultSet != NULL);
    ASSERT_FALSE(ResultSet->Next());
}

TEST_F(SQLITE_TEST_WITH_TABLE_AND_DBSTATEMENT, SelectOneRowOne)
{
    fm_string Query;
    Query.format("INSERT INTO %s VALUES(1, \'abcdefg\', 1.0)", TableName);
    ASSERT_TRUE(QueryStatement->Execute(*Query) != NULL);

    Query.format("SELECT * FROM %s", TableName);
    ASSERT_TRUE(QueryStatement->Execute(*Query) != NULL);

    IDBResultSet* ResultSet = QueryStatement->GetResultSet();
    ASSERT_TRUE(ResultSet != NULL);
    ASSERT_TRUE(ResultSet->Next());

    int IntValue;
    ASSERT_TRUE(ResultSet->GetInt32ByIndex(0, IntValue));
    EXPECT_EQ(1, IntValue);
}

TEST_F(SQLITE_TEST_WITH_TABLE_AND_DBSTATEMENT, SelectOneRowAll)
{
    fm_string Query;
    Query.format("INSERT INTO %s VALUES(1, \'abcdefg\', 1.0)", TableName);
    ASSERT_TRUE(QueryStatement->Execute(*Query) != NULL);

    Query.format("INSERT INTO %s VALUES(11, \'ùÓÙþ\', 11.0)", TableName);
    ASSERT_TRUE(QueryStatement->Execute(*Query) != NULL);

    Query.format("INSERT INTO %s VALUES(111, \'ÇÑ±Û\', 111.0)", TableName);
    ASSERT_TRUE(QueryStatement->Execute(*Query) != NULL);

    Query.format("SELECT stringValue, intValue, doubleValue FROM %s WHERE intValue = 111", TableName);
    ASSERT_TRUE(QueryStatement->Execute(*Query) != NULL);

    IDBResultSet* ResultSet = QueryStatement->GetResultSet();
    ASSERT_TRUE(ResultSet != NULL);
    ASSERT_TRUE(ResultSet->Next());

    const char* StringValue;
    ASSERT_TRUE(ResultSet->GetStringByIndex(0, StringValue));
    EXPECT_STREQ("ÇÑ±Û", StringValue);

    int IntValue;
    ASSERT_TRUE(ResultSet->GetInt32ByIndex(1, IntValue));
    EXPECT_EQ(111, IntValue);

    double DoubleValue;
    ASSERT_TRUE(ResultSet->GetDoubleByIndex(2, DoubleValue));
    EXPECT_EQ(111.0, DoubleValue);

    ASSERT_FALSE(ResultSet->GetDoubleByIndex(3, DoubleValue));
    ASSERT_TRUE(Connection->GetLastError() == eDB_INVALID_COLUMN_INDEX);
    ASSERT_FALSE(ResultSet->Next());
}

TEST_F(SQLITE_TEST_WITH_TABLE_AND_DBSTATEMENT, SelectOneByColumnName)
{
    fm_string Query;
    Query.format("INSERT INTO %s VALUES(1, \'abcdefg\', 1.0)", TableName);
    ASSERT_TRUE(QueryStatement->Execute(*Query) != NULL);

    Query.format("INSERT INTO %s VALUES(11, \'ùÓÙþ\', 11.0)", TableName);
    ASSERT_TRUE(QueryStatement->Execute(*Query) != NULL);

    Query.format("INSERT INTO %s VALUES(111, \'ÇÑ±Û\', 111.0)", TableName);
    ASSERT_TRUE(QueryStatement->Execute(*Query) != NULL);

    Query.format("SELECT stringValue, intValue, doubleValue FROM %s WHERE intValue = 11", TableName);
    ASSERT_TRUE(QueryStatement->Execute(*Query) != NULL);

    IDBResultSet* ResultSet = QueryStatement->GetResultSet();
    ASSERT_TRUE(ResultSet != NULL);
    ASSERT_TRUE(ResultSet->Next());

    const char* StringValue;
    ASSERT_TRUE(ResultSet->GetStringByName("stringValue", StringValue));
    EXPECT_STREQ("ùÓÙþ", StringValue);

    int IntValue;
    ASSERT_TRUE(ResultSet->GetInt32ByName("intValue", IntValue));
    EXPECT_EQ(11, IntValue);

    double DoubleValue;
    ASSERT_TRUE(ResultSet->GetDoubleByName("doubleValue", DoubleValue));
    EXPECT_EQ(11.0, DoubleValue);

    ASSERT_FALSE(ResultSet->GetStringByName("stringValue2", StringValue));
    ASSERT_TRUE(Connection->GetLastError() == eDB_INVALID_COLUMN_NAME);
    ASSERT_FALSE(ResultSet->Next());
}

TEST_F(SQLITE_TEST_WITH_TABLE_AND_DBSTATEMENT, Update)
{
    fm_string Query;
    Query.format("INSERT INTO %s VALUES(1, \'abcdefg\', 1.0)", TableName);
    ASSERT_TRUE(QueryStatement->Execute(*Query) != NULL);

    Query.format("INSERT INTO %s VALUES(11, \'ùÓÙþ\', 11.0)", TableName);
    ASSERT_TRUE(QueryStatement->Execute(*Query) != NULL);

    Query.format("INSERT INTO %s VALUES(111, \'ÇÑ±Û\', 111.0)", TableName);
    ASSERT_TRUE(QueryStatement->Execute(*Query) != NULL);

    Query.format("INSERT INTO %s VALUES(111, \'ÇÑ±Û2\', 111.0)", TableName);
    ASSERT_TRUE(QueryStatement->Execute(*Query) != NULL);

    Query.format("INSERT INTO %s VALUES(111, \'ÇÑ±Û3\', 111.0)", TableName);
    ASSERT_TRUE(QueryStatement->Execute(*Query) != NULL);

    Query.format("UPDATE %s SET stringValue = \'Success\' WHERE intValue = 111", TableName);
    ASSERT_TRUE(QueryStatement->Execute(*Query));
    ASSERT_TRUE(QueryStatement->GetUpdateCount() == 3);

    Query.format("SELECT stringValue, intValue, doubleValue FROM %s WHERE intValue = 111", TableName);
    ASSERT_TRUE(QueryStatement->Execute(*Query) != NULL);

    IDBResultSet* ResultSet = QueryStatement->GetResultSet();
    ASSERT_TRUE(ResultSet != NULL);
    ASSERT_TRUE(ResultSet->Next());

    const char* StringValue;
    ASSERT_TRUE(ResultSet->GetStringByName("stringValue", StringValue));
    EXPECT_STREQ("Success", StringValue);

    // GetNumRows
    Query.format("SELECT * FROM %s ", TableName);

    ASSERT_TRUE(QueryStatement->Execute(*Query) != NULL);
    ResultSet = QueryStatement->GetResultSet(true);
    ASSERT_TRUE(ResultSet != NULL);
    ASSERT_TRUE(ResultSet->Next());
    EXPECT_NE(5, ResultSet->GetNumRows());

    ASSERT_TRUE(QueryStatement->Execute(*Query) != NULL);
    ResultSet = QueryStatement->GetResultSet();
    ASSERT_TRUE(ResultSet != NULL);
    ASSERT_TRUE(ResultSet->Next());
    ASSERT_TRUE(ResultSet->Next());
    ASSERT_TRUE(ResultSet->Next());
    ASSERT_TRUE(ResultSet->Next());
    ASSERT_TRUE(ResultSet->Next());
    ASSERT_FALSE(ResultSet->Next());
    EXPECT_EQ(5, ResultSet->GetNumRows());
}


TEST_F(SQLITE_TEST_WITH_TABLE_AND_DBSTATEMENT, PreparedStatement)
{
    fm_string Query;
    Query.format("INSERT INTO %s VALUES(?, ?, ?)", TableName);
    IDBPreparedStatement* Statement = Connection->CreatePreparedStatement(*Query);
    ASSERT_TRUE(Statement != NULL);
    ASSERT_STREQ(Statement->GetQuery(), *Query);

    Statement->SetInt16ByIndex(0, 222);
    Statement->SetStringByIndex(1, "Success", strlen("Success"));
    Statement->SetDoubleByIndex(2, 2.0);
    ASSERT_TRUE(Statement->Execute());
    ASSERT_TRUE(Statement->GetUpdateCount() == 1);
    Connection->DestroyStatement(Statement);
}

TEST_F(SQLITE_TEST_WITH_TABLE_AND_DBSTATEMENT, PreparedStatementMultiInput)
{
    fm_string Query;
    Query.format("INSERT INTO %s VALUES(?, ?, ?)", TableName);
    IDBPreparedStatement* Statement = Connection->CreatePreparedStatement(*Query);
    ASSERT_TRUE(Statement != NULL);

    Statement->SetUInt8ByIndex(0, 222);
    Statement->SetStringByIndex(1, "Success", strlen("Success"));
    Statement->SetDoubleByIndex(2, 2.0);

    ASSERT_TRUE(Statement->Execute());
    ASSERT_TRUE(Statement->GetUpdateCount() == 1);

    Statement->SetUInt8ByIndex(0, 111);
    Statement->SetStringByIndex(1, "Hello", strlen("Hello"));
    Statement->SetDoubleByIndex(2, 3.0);

    ASSERT_TRUE(Statement->Execute());
    ASSERT_TRUE(Statement->GetUpdateCount() == 1);
    Connection->DestroyStatement(Statement);

    Query.format("SELECT * FROM %s WHERE IntValue=?", TableName);
    Statement = Connection->CreatePreparedStatement(*Query);
    ASSERT_TRUE(Statement != NULL);

    Statement->SetUInt8ByIndex(0, 222);
    ASSERT_TRUE(Statement->Execute());

    IDBResultSet* ResultSet = Statement->GetResultSet();
    ASSERT_TRUE(ResultSet != NULL);
    ASSERT_TRUE(ResultSet->Next());

    uint8 IntValue;
    ASSERT_TRUE(ResultSet->GetUInt8ByIndex(0, IntValue));
    EXPECT_EQ(222, IntValue);

    const char* StringValue;
    ASSERT_TRUE(ResultSet->GetStringByIndex(1, StringValue));
    EXPECT_STREQ("Success", StringValue);

    double DoubleValue;
    ASSERT_TRUE(ResultSet->GetDoubleByIndex(2, DoubleValue));
    EXPECT_EQ(2.0, DoubleValue);

    Statement->SetUInt8ByIndex(0, 111);
    ASSERT_TRUE(Statement->Execute());

    ResultSet = Statement->GetResultSet();
    ASSERT_TRUE(ResultSet != NULL);
    ASSERT_TRUE(ResultSet->Next());

    ASSERT_TRUE(ResultSet->GetUInt8ByIndex(0, IntValue));
    EXPECT_EQ(111, IntValue);

    ASSERT_TRUE(ResultSet->GetStringByIndex(1, StringValue));
    EXPECT_STREQ("Hello", StringValue);

    ASSERT_TRUE(ResultSet->GetDoubleByIndex(2, DoubleValue));
    EXPECT_EQ(3.0, DoubleValue);

    Connection->DestroyStatement(Statement);
}

TEST_F(SQLITE_TEST_WITH_TABLE_AND_DBSTATEMENT, PreparedStatementWithBinaryData)
{
    fm_string Query;
    Query.format("ALTER TABLE %s ADD bin blob;", TableName);
    ASSERT_TRUE(m_DDLStatement->Execute(*Query));
    // sqlite3 doesn't implement ALTER TABLE's somple opreation (DROP COLUMN, ALTER COLUMN)
    //Query.format("ALTER TABLE %s DROP doubleValue;", TableName);
    //ASSERT_TRUE(m_DDLStatement->Execute(*Query));

    Query.format("INSERT INTO %s VALUES(?, ?, ?, ?)", TableName);
    IDBPreparedStatement* Statement = Connection->CreatePreparedStatement(*Query);
    ASSERT_TRUE(Statement != NULL);

    const char* StrValue = "Success";
    unsigned char BinValue[] = {0x10, 0x20, 0x00, 0x30, 0x40};

    Statement->SetInt32ByIndex(0, 222);
    Statement->SetStringByIndex(1, StrValue, strlen(StrValue));
    Statement->SetDoubleByIndex(2, 0.001);
    Statement->SetBinaryByIndex(3, BinValue, 4); // last 0x40 is skipped

    ASSERT_TRUE(Statement->Execute());
    ASSERT_TRUE(Statement->GetUpdateCount() == 1);
    Connection->DestroyStatement(Statement);

    Query.format("SELECT * FROM %s WHERE IntValue=?", TableName);
    IDBPreparedStatement* ResultStatement = Connection->CreatePreparedStatement(*Query);
    ASSERT_TRUE(ResultStatement != NULL);
    Statement->SetInt32ByIndex(0, 222);
    ASSERT_TRUE(Statement->Execute());

    IDBResultSet* ResultSet = ResultStatement->GetResultSet();
    ASSERT_TRUE(ResultSet != NULL);
    ASSERT_TRUE(ResultSet->Next());

    int IntValue;
    ASSERT_TRUE(ResultSet->GetInt32ByIndex(0, IntValue));
    EXPECT_EQ(222, IntValue);

    const char* StringValue;
    ASSERT_TRUE(ResultSet->GetStringByIndex(1, StringValue));
    EXPECT_STREQ("Success", StringValue);

    double DoubleValue;
    ASSERT_TRUE(ResultSet->GetDoubleByIndex(2, DoubleValue));
    EXPECT_EQ(0.001, DoubleValue);

    const byte* CheckBinValue;
    size_t CheckBinValueLen = 0;

    ASSERT_TRUE(ResultSet->GetBinaryByIndex(3, CheckBinValue, CheckBinValueLen));
    EXPECT_EQ(4, CheckBinValueLen);
    EXPECT_EQ(0x10, CheckBinValue[0]);
    EXPECT_EQ(0x20, CheckBinValue[1]);
    EXPECT_EQ(0x00, CheckBinValue[2]);
    EXPECT_EQ(0x30, CheckBinValue[3]);
    Connection->DestroyStatement(ResultStatement);
}

// http://www.sqlite.org/faq.html#q1
TEST_F(SQLITE_TEST_WITH_RANDOM_TABLENAME, GetLastId)
{
    const char* CreateQueryFormat =
        "CREATE TABLE %s"
        "("
        "id INTEGER PRIMARY KEY,"             // if id is null, it'll be AUTO_INCREMENTed.
        "name VARCHAR(256) NOT NULL"
        ")";

    fm_string Query;
    IDBStatement* CreateTableStmt = Connection->CreateStatement();
    Query.format("DROP TABLE IF EXISTS %s", TableName);
    ASSERT_TRUE(CreateTableStmt != NULL && CreateTableStmt->Execute(*Query));
    Query.format(CreateQueryFormat, TableName);
    ASSERT_TRUE(CreateTableStmt->Execute(*Query));
    ASSERT_TRUE(Connection->DestroyStatement(CreateTableStmt));

    IDBStatement* InsertStatement = Connection->CreateStatement();
    ASSERT_TRUE(InsertStatement != NULL);
    Query.format("INSERT INTO %s VALUES(NULL,'dummy_id')", TableName);
    ASSERT_TRUE(InsertStatement->Execute(*Query));
    ASSERT_EQ(1, InsertStatement->GetAutoIncrementedValue());
    ASSERT_TRUE(Connection->DestroyStatement(InsertStatement));

    fm_string SelectQuery;
    SelectQuery.format("SELECT * FROM %s ", TableName);
    IDBStatement* SelectStatement = Connection->CreateStatement();
    ASSERT_TRUE(SelectStatement != NULL);
    ASSERT_TRUE(SelectStatement->Execute(*SelectQuery));
    ASSERT_EQ(1, SelectStatement->GetAutoIncrementedValue()); // read from InsertStatement value
    ASSERT_TRUE(SelectStatement->CloseResultSet());

    InsertStatement = Connection->CreateStatement();
    ASSERT_TRUE(InsertStatement != NULL);
    Query.format("INSERT INTO %s VALUES(NULL,'dummy_id2')", TableName);
    ASSERT_TRUE(InsertStatement->Execute(*Query));
    ASSERT_EQ(2, InsertStatement->GetAutoIncrementedValue());

    ASSERT_TRUE(SelectStatement->Execute(*SelectQuery));
    ASSERT_EQ(2, SelectStatement->GetAutoIncrementedValue()); // read from InsertStatement value
    ASSERT_TRUE(SelectStatement->CloseResultSet());

    ASSERT_TRUE(Connection->DestroyStatement(InsertStatement));
    ASSERT_TRUE(Connection->DestroyStatement(SelectStatement));
}

TEST_F(SQLITE_TEST_WITH_TABLE_AND_DBSTATEMENT, GetLastId_Invalid)
{
    fm_string Query;
    Query.format("SELECT * FROM %s", TableName);
    IDBStatement* SelectStatement = Connection->CreateStatement();
    ASSERT_TRUE(SelectStatement != NULL);
    ASSERT_TRUE(SelectStatement->Execute(*Query));
    ASSERT_EQ(0, SelectStatement->GetAutoIncrementedValue()); // read from InsertStatement value
    ASSERT_TRUE(SelectStatement->CloseResultSet());
    ASSERT_TRUE(Connection->DestroyStatement(SelectStatement));
}


TEST_F(SQLITE_TEST_WITH_TABLE_AND_DBSTATEMENT, DBAPIHELPER)
{
    fm_string Query;
    IDBPreparedStatement* Statement = NULL;
    IDBResultSet* ResultSet = NULL;
    int32 OutIntValue = 0;
    const char* OutStrValue = NULL;
    double OutDoubleValue = 0.0f;

    // insert
    Query.format("INSERT INTO %s VALUES(?, ?, ?)", TableName);
    Statement = Connection->CreatePreparedStatement(*Query);
    ASSERT_TRUE(Statement != NULL);
    ASSERT_TRUE(DBAPIHelper::Exec(Statement, 1, "one", 1.0));
    ASSERT_TRUE(DBAPIHelper::Exec(Statement, 2, "two", 2.0));
    //ASSERT_FALSE(DBAPIHelper::Exec(Statement, "error", "zero", -1.0));
    ASSERT_TRUE(DBAPIHelper::Exec(Statement, 3, "three", 3.0));
    Connection->DestroyStatement(Statement);

    // select one row
    Query.format("SELECT * FROM %s WHERE IntValue=?", TableName);
    Statement = Connection->CreatePreparedStatement(*Query);
    ASSERT_TRUE(Statement != NULL);
    ASSERT_TRUE(DBAPIHelper::Exec(Statement, 2));
    ResultSet = Statement->GetResultSet();
    ASSERT_TRUE(ResultSet != NULL);

    while (ResultSet->Next())
    {
        if (!DBAPIHelper::Get(ResultSet, OutIntValue, OutStrValue, OutDoubleValue))
        {
            break;
        }
        fprintf(stdout, "IntValue:%d, StringValue:%s, DoubleValue:%f\n", OutIntValue, OutStrValue, OutDoubleValue);
    }
    Connection->DestroyStatement(Statement);

    QueryStatement = Connection->CreateStatement();
    ASSERT_TRUE(QueryStatement != NULL);
    Query.format("SELECT * FROM %s", TableName);
    ASSERT_TRUE(QueryStatement->Execute(*Query));
    ResultSet = QueryStatement->GetResultSet();
    ASSERT_TRUE(ResultSet != NULL);

    while (ResultSet->Next())
    {
        if (!DBAPIHelper::Get(ResultSet, OutIntValue, OutStrValue, OutDoubleValue))
        {
            break;
        }
        fprintf(stdout, "IntValue:%d, StringValue:%s, DoubleValue:%f\n", OutIntValue, OutStrValue, OutDoubleValue);
    }
}

#include "QueryDef.h"
struct SQLiteCachedConnection
{
    static ICachedDBConnection* Create(IDBConnection* Connection)
    {
        return DriverManager::CreateCachedConnection(Connection, GetQueryMapper<TEST_PREPARED_QUERIES>(), NULL);
    }
};
struct DummyTableName
{
    static const char* Get()
    {
        return "dummy_table";
    }
};

typedef DBAPI_TEST_CACHEDCONNECTION_WITH_TABLE<SQLiteConnector, SQLiteCachedConnection, DummyTableName, DefaultSchemaQuery> SQLITE_TEST_CACHEDCONNECTION_WITH_TABLE;

TEST_F(SQLITE_TEST_CACHEDCONNECTION_WITH_TABLE, CachedConnection_CREATE)
{
    IDBStatement* S1 = Connection->GetStatement();
    IDBStatement* S2 = Connection->GetStatement();
    ASSERT_EQ(S1, S2);
    ASSERT_TRUE(S1->Execute("SELECT 111;"));
    ASSERT_TRUE(S2->Execute("SELECT 222;"));
}

TEST_F(SQLITE_TEST_CACHEDCONNECTION_WITH_TABLE, CachedConnection_GetPreparedStatement)
{
    IDBResultSet* ResultSet;
    int64 OutIntValue;
    const char* OutStringValue;
    IDBPreparedStatement* S1 = Connection->GetPreparedStatement(PSELECT0);
    IDBPreparedStatement* S2 = Connection->GetPreparedStatement(PSELECT1);
    ASSERT_NE(S1, S2);

    ASSERT_TRUE(S1->Execute());
    ResultSet = S1->GetResultSet();
    ASSERT_TRUE(ResultSet != NULL);

    ASSERT_TRUE(ResultSet->Next());
    ASSERT_TRUE(DBAPIHelper::Get(ResultSet, OutIntValue, OutStringValue));
    EXPECT_EQ(0, OutIntValue);
    EXPECT_STREQ("0", OutStringValue);

    ASSERT_TRUE(S2->Execute());
    ResultSet = S2->GetResultSet();
    ASSERT_TRUE(ResultSet != NULL);
    ASSERT_TRUE(ResultSet->Next());
    ASSERT_TRUE(DBAPIHelper::Get(ResultSet, OutIntValue, OutStringValue));
    EXPECT_EQ(10, OutIntValue);
    EXPECT_STREQ("20", OutStringValue);
}

TEST_F(SQLITE_TEST_CACHEDCONNECTION_WITH_TABLE, CachedConnection_PreparedStatement)
{
    IDBPreparedStatement* Statement = NULL;
    IDBResultSet* ResultSet = NULL;
    int OutIntValue;
    const char* OutStrValue = NULL;
    double OutDoubleValue;
    Statement = Connection->GetPreparedStatement(PINSERT1);
    ASSERT_TRUE(Statement != NULL);
    ASSERT_TRUE(DBAPIHelper::Exec(Statement, 1, "success1", 1.0));
    ASSERT_TRUE(DBAPIHelper::Exec(Statement, 2, "success2", 2.0));
    ASSERT_TRUE(DBAPIHelper::Exec(Statement, 3, "success3", 3.0));
    ASSERT_TRUE(DBAPIHelper::Exec(Statement, 4, "success4", 4.0));
    ASSERT_TRUE(DBAPIHelper::Exec(Statement, 5, "success5", 5.0));
    ASSERT_TRUE(Statement->GetUpdateCount() == 1);

    Statement = Connection->GetPreparedStatement(PSELECT_ALL);
    ASSERT_TRUE(Statement != NULL);
    ASSERT_TRUE(DBAPIHelper::Exec(Statement, 2));
    ResultSet = Statement->GetResultSet();
    ASSERT_TRUE(ResultSet != NULL);
    ASSERT_TRUE(ResultSet->Next());
    ASSERT_TRUE(DBAPIHelper::Get(ResultSet, OutIntValue, OutStrValue, OutDoubleValue));
    EXPECT_EQ(3, OutIntValue);
    EXPECT_STREQ("success3", OutStrValue);
    EXPECT_EQ(3.0, OutDoubleValue);
    ASSERT_TRUE(ResultSet->Next());
    ASSERT_TRUE(DBAPIHelper::Get(ResultSet, OutIntValue, OutStrValue, OutDoubleValue));
    EXPECT_EQ(4, OutIntValue);
    EXPECT_STREQ("success4", OutStrValue);
    EXPECT_EQ(4.0, OutDoubleValue);
    ASSERT_TRUE(ResultSet->Next());
    ASSERT_TRUE(DBAPIHelper::Get(ResultSet, OutIntValue, OutStrValue, OutDoubleValue));
    EXPECT_EQ(5, OutIntValue);
    EXPECT_STREQ("success5", OutStrValue);
    EXPECT_EQ(5.0, OutDoubleValue);
}