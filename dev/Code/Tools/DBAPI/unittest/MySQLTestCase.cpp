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

// INSERT INTO mysql.db VALUES('localhost','googletest','googletest','Y','Y','Y','Y','Y','Y','Y','Y','Y','Y','Y','Y','Y','Y','Y','Y','Y','Y','Y');
const char* host = "localhost";
const char* db = "crytektest";
const char* user = "crytektest";
const char* password = "crytek";

struct MySQLConnector
{
    static IDBConnection* Connect()
    {
        return DriverManager::CreateConnection(eDT_MySQL, host, 0, db, user, password);
    }
};

struct MySQLTableNameGenerator
    : public DefaultTableNameGenerator
{
    std::string& GenerateName(const char* Category, const char* Prefix)
    {
        DefaultTableNameGenerator::GenerateName(Category, Prefix);
        // Mysql Table Length Limit is 64
        if (TableName.length() >= 64)
        {
            TableName.erase(0, TableName.length() - 64);
        }
        return TableName;
    }
};

typedef DBAPI_TEST_BASE<MySQLConnector> MYSQL_TEST_BASE;
typedef DBAPI_TEST_WITH_FIXED_TABLE<MySQLConnector, MySQLTableNameGenerator, DefaultSchemaQuery > MYSQL_TEST_WITH_FIXED_TABLE;
typedef DBAPI_TEST_WITH_TABLE_AND_DBSTATEMENT<MySQLConnector, MySQLTableNameGenerator, DefaultSchemaQuery > MYSQL_TEST_WITH_TABLE_AND_DBSTATEMENT;
typedef DBAPI_TEST_WITH_RANDOM_TABLENAME<MySQLConnector, MySQLTableNameGenerator> MYSQL_TEST_WITH_RANDOM_TABLENAME;

TEST(SUPPORT_DBAPI, MYSQL)
{
    IDBConnection* DBConnection = MySQLConnector::Connect();
    ASSERT_TRUE(DBConnection != NULL);
    ASSERT_GE(DBConnection->ClientVersion(), 0x00050000u);
    EXPECT_GE(DBConnection->ServerVersion(), 0x00050000u);
    DriverManager::DestroyConnection(DBConnection);
}

TEST(MYSQL_TEST, CONNECT_WITH_ERROR)
{
    IDBConnection* DBConnection = DriverManager::CreateConnection(eDT_MySQL, host, 0, db, NULL, NULL);
    ASSERT_TRUE(DBConnection == NULL);
}

TEST(MYSQL_TEST, CONNECT_WITH_CONNECTIONSTRING)
{
    std::string con;
    con = std::string("mysql://") + host + "/" + db + "?" + "user=" + user + "&" + "password=" + password;
    std::string con2;
    con2 = std::string("mysql://") + host + ":0/" + db + "?" + "user=" + user + "&" + "password=" + password;
    std::string con_error1;
    con_error1 = std::string("mysql://") + host + ":0/" + db + "&" + "user=" + user + "&" + "password=" + password;
    std::string con_error2;
    con_error2 = std::string("mysql://") + host + ":0/" + db + "?" + "user=" + user + "?" + "password=" + password;
    std::string con_error3("mysql:///");

    IDBConnection* ErrroDBConnection;
    ErrroDBConnection = DriverManager::CreateConnection(con_error1.c_str());
    ASSERT_TRUE(ErrroDBConnection == NULL);
    ErrroDBConnection = DriverManager::CreateConnection(con_error2.c_str());
    ASSERT_TRUE(ErrroDBConnection == NULL);
    ErrroDBConnection = DriverManager::CreateConnection(con_error3.c_str());
    ASSERT_TRUE(ErrroDBConnection == NULL);

    IDBConnection* DBConnection;
    DBConnection = DriverManager::CreateConnection(con.c_str());
    ASSERT_TRUE(DBConnection != NULL);
    DriverManager::DestroyConnection(DBConnection);
    DBConnection = DriverManager::CreateConnection(con2.c_str());
    ASSERT_TRUE(DBConnection != NULL);
    DriverManager::DestroyConnection(DBConnection);
}

TEST_F(MYSQL_TEST_BASE, Ping)
{
    ASSERT_TRUE(Connection->Ping());
}

TEST_F(MYSQL_TEST_BASE, CreateStatement)
{
    IDBStatement* DBStatement;
    DBStatement = Connection->CreateStatement();
    ASSERT_TRUE(DBStatement != NULL);
    Connection->DestroyStatement(DBStatement);
}

TEST_F(MYSQL_TEST_BASE, CreatePreparedStatement)
{
    IDBPreparedStatement* DBStatement;
    DBStatement = Connection->CreatePreparedStatement("");
    ASSERT_TRUE(DBStatement == NULL);
    DBStatement = Connection->CreatePreparedStatement("SHOW TABLES");
    ASSERT_TRUE(DBStatement != NULL);

    Connection->DestroyStatement(DBStatement);
}

TEST_F(MYSQL_TEST_WITH_TABLE_AND_DBSTATEMENT, Insert)
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

TEST_F(MYSQL_TEST_WITH_TABLE_AND_DBSTATEMENT, SelectOneRowOne)
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
    ASSERT_FALSE(ResultSet->Next());
}

TEST_F(MYSQL_TEST_WITH_TABLE_AND_DBSTATEMENT, SelectOneRowAll)
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

TEST_F(MYSQL_TEST_WITH_TABLE_AND_DBSTATEMENT, SelectOneByColumnName)
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
}

TEST_F(MYSQL_TEST_WITH_TABLE_AND_DBSTATEMENT, Update)
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
    EXPECT_EQ(5, ResultSet->GetNumRows());
}

TEST_F(MYSQL_TEST_WITH_TABLE_AND_DBSTATEMENT, PreparedStatement)
{
    fm_string Query;
    Query.format("INSERT INTO %s VALUES(?, ?, ?)", TableName);
    IDBPreparedStatement* Statement = Connection->CreatePreparedStatement(*Query);
    ASSERT_TRUE(Statement != NULL);
    ASSERT_STREQ(Statement->GetQuery(), *Query);

    const char* StrValue1 = "Success";
    Statement->SetInt32ByIndex(0, 2);
    Statement->SetStringByIndex(1, StrValue1, strlen(StrValue1));
    Statement->SetDoubleByIndex(2, 2.0);

    ASSERT_TRUE(Statement->Execute());
    ASSERT_TRUE(Statement->GetUpdateCount() == 1);
    Connection->DestroyStatement(Statement);
}

TEST_F(MYSQL_TEST_WITH_TABLE_AND_DBSTATEMENT, PreparedStatementMultiInput)
{
    fm_string Query;
    Query.format("INSERT INTO %s VALUES(?, ?, ?)", TableName);
    IDBPreparedStatement* Statement = Connection->CreatePreparedStatement(*Query);
    ASSERT_TRUE(Statement != NULL);

    const char* StrValue1 = "Success";
    Statement->SetInt32ByIndex(0, 222);
    Statement->SetStringByIndex(1, StrValue1, strlen(StrValue1));
    Statement->SetDoubleByIndex(2, 2.0);

    ASSERT_TRUE(Statement->Execute());
    ASSERT_TRUE(Statement->GetUpdateCount() == 1);

    const char* StrValue2 = "Hello";
    Statement->SetInt32ByIndex(0, 333);
    Statement->SetStringByIndex(1, StrValue2, strlen(StrValue2));
    Statement->SetDoubleByIndex(2, 3.0);

    ASSERT_TRUE(Statement->Execute());
    ASSERT_TRUE(Statement->GetUpdateCount() == 1);
    Connection->DestroyStatement(Statement);

    Query.format("SELECT * FROM %s WHERE IntValue=?", TableName);
    Statement = Connection->CreatePreparedStatement(*Query);
    ASSERT_TRUE(Statement != NULL);

    IDBResultSet* ResultSet;
    int OutIntValue;
    const char* OutStringValue;
    double OutDoubleValue;

    Statement->SetInt32ByIndex(0, 222);
    ASSERT_TRUE(Statement->Execute());
    ResultSet = Statement->GetResultSet();
    ASSERT_TRUE(ResultSet != NULL);
    ASSERT_TRUE(ResultSet->Next());
    ASSERT_TRUE(ResultSet->GetInt32ByIndex(0, OutIntValue));
    EXPECT_EQ(222, OutIntValue);
    ASSERT_TRUE(ResultSet->GetStringByIndex(1, OutStringValue));
    EXPECT_STREQ(StrValue1, OutStringValue);
    ASSERT_TRUE(ResultSet->GetDoubleByIndex(2, OutDoubleValue));
    EXPECT_EQ(2.0, OutDoubleValue);

    Statement->SetInt32ByIndex(0, 333);
    ASSERT_TRUE(Statement->Execute());
    ResultSet = Statement->GetResultSet();
    ASSERT_TRUE(ResultSet != NULL);
    ASSERT_TRUE(ResultSet->Next());
    ASSERT_TRUE(ResultSet->GetInt32ByIndex(0, OutIntValue));
    EXPECT_EQ(333, OutIntValue);
    ASSERT_TRUE(ResultSet->GetDoubleByIndex(2, OutDoubleValue));
    EXPECT_EQ(3.0, OutDoubleValue);
    ASSERT_TRUE(ResultSet->GetStringByIndex(1, OutStringValue));
    EXPECT_STREQ(StrValue2, OutStringValue);

    Statement->SetInt32ByIndex(0, 222);
    ASSERT_TRUE(Statement->Execute());
    ResultSet = Statement->GetResultSet();
    ASSERT_TRUE(ResultSet != NULL);
    ASSERT_TRUE(ResultSet->Next());
    ASSERT_TRUE(ResultSet->GetInt32ByIndex(0, OutIntValue));
    EXPECT_EQ(222, OutIntValue);
    ASSERT_TRUE(ResultSet->GetStringByIndex(1, OutStringValue));
    EXPECT_STREQ(StrValue1, OutStringValue);
    ASSERT_TRUE(ResultSet->GetDoubleByIndex(2, OutDoubleValue));
    EXPECT_EQ(2.0, OutDoubleValue);

    Statement->SetInt32ByIndex(0, 333);
    ASSERT_TRUE(Statement->Execute());
    ResultSet = Statement->GetResultSet();
    ASSERT_TRUE(ResultSet != NULL);
    ASSERT_TRUE(ResultSet->Next());
    ASSERT_TRUE(ResultSet->GetInt32ByIndex(0, OutIntValue));
    EXPECT_EQ(333, OutIntValue);
    ASSERT_TRUE(ResultSet->GetDoubleByIndex(2, OutDoubleValue));
    EXPECT_EQ(3.0, OutDoubleValue);
    ASSERT_TRUE(ResultSet->GetStringByIndex(1, OutStringValue));
    EXPECT_STREQ(StrValue2, OutStringValue);

    Connection->DestroyStatement(Statement);
}

TEST_F(MYSQL_TEST_WITH_TABLE_AND_DBSTATEMENT, PreparedStatementMultiInputWithInvalidBindings)
{
    fm_string Query;
    Query.format("INSERT INTO %s VALUES(?, ?, ?)", TableName);
    IDBPreparedStatement* Statement = Connection->CreatePreparedStatement(*Query);
    ASSERT_TRUE(Statement != NULL);

    // 1st
    const char* StrValue1 = "Success";
    Statement->SetInt32ByIndex(0, 222);
    Statement->SetStringByIndex(1, StrValue1, strlen(StrValue1));
    Statement->SetDoubleByIndex(2, 2.0);

    ASSERT_TRUE(Statement->Execute());
    ASSERT_TRUE(Statement->GetUpdateCount() == 1);

    // 2nd
    Statement->SetUInt64ByIndex(0, 0xffffffffff);
    Statement->SetStringByIndex(1, StrValue1, strlen(StrValue1));
    Statement->SetFloatByIndex(2, 3.0);

    ASSERT_FALSE(Statement->Execute());

    // 3rd
    Statement->SetInt32ByIndex(0, 32768);
    Statement->SetStringByIndex(2, StrValue1, strlen(StrValue1));
    Statement->SetFloatByIndex(1, 3.0);

    ASSERT_FALSE(Statement->Execute());

    // 4th
    Statement->SetInt64ByIndex(0, 65536);
    Statement->SetStringByIndex(1, StrValue1, strlen(StrValue1));
    Statement->SetFloatByIndex(2, 3.0);

    ASSERT_TRUE(Statement->Execute());
    ASSERT_TRUE(Statement->GetUpdateCount() == 1);

    // 5th
    Statement->SetInt64ByIndex(0, -1);
    Statement->SetStringByIndex(1, StrValue1, strlen(StrValue1));
    Statement->SetFloatByIndex(2, -3.0f);

    ASSERT_TRUE(Statement->Execute());
    ASSERT_TRUE(Statement->GetUpdateCount() == 1);

    Connection->DestroyStatement(Statement);

    Query.format("SELECT * FROM %s WHERE IntValue=?", TableName);
    Statement = Connection->CreatePreparedStatement(*Query);
    ASSERT_TRUE(Statement != NULL);
    Statement->SetInt32ByIndex(0, 65536);
    ASSERT_TRUE(Statement->Execute());

    IDBResultSet* ResultSet = Statement->GetResultSet();
    ASSERT_TRUE(ResultSet != NULL);
    ASSERT_TRUE(ResultSet->Next());

    int OutIntValue;
    ASSERT_TRUE(ResultSet->GetInt32ByIndex(0, OutIntValue));
    EXPECT_EQ(65536, OutIntValue);

    const char* OutStringValue;
    ASSERT_TRUE(ResultSet->GetStringByIndex(1, OutStringValue));
    EXPECT_STREQ(StrValue1, OutStringValue);

    double OutDoubleValue;
    ASSERT_TRUE(ResultSet->GetDoubleByIndex(2, OutDoubleValue));
    EXPECT_EQ(3.0, OutDoubleValue);

    Connection->DestroyStatement(Statement);

    // GetNumRows
    Query.format("SELECT * FROM %s ", TableName);

    ASSERT_TRUE(QueryStatement->Execute(*Query) != NULL);
    ResultSet = QueryStatement->GetResultSet(true);
    ASSERT_TRUE(ResultSet != NULL);
    ASSERT_TRUE(ResultSet->Next());
    EXPECT_NE(3, ResultSet->GetNumRows());

    ASSERT_TRUE(QueryStatement->Execute(*Query) != NULL);
    ResultSet = QueryStatement->GetResultSet();
    ASSERT_TRUE(ResultSet != NULL);
    ASSERT_TRUE(ResultSet->Next());
    EXPECT_EQ(3, ResultSet->GetNumRows());
}


TEST_F(MYSQL_TEST_WITH_TABLE_AND_DBSTATEMENT, PreparedStatementWithBinaryData)
{
    fm_string Query;
    Query.format("ALTER TABLE %s ADD bin blob", TableName);
    ASSERT_TRUE(m_DDLStatement->Execute(*Query));
    Query.format("ALTER TABLE %s DROP doubleValue", TableName);
    ASSERT_TRUE(m_DDLStatement->Execute(*Query));

    Query.format("INSERT INTO %s VALUES(?, ?, ?)", TableName);
    IDBPreparedStatement* Statement = Connection->CreatePreparedStatement(*Query);
    ASSERT_TRUE(Statement != NULL);

    const char* StrValue = "Success";
    unsigned char BinValue[] = {0x10, 0x20, 0x00, 0x30, 0x40};

    Statement->SetInt32ByIndex(0, 222);
    Statement->SetStringByIndex(1, StrValue, strlen(StrValue));
    Statement->SetBinaryByIndex(2, BinValue, 4);

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

    int OutIntValue;
    ASSERT_TRUE(ResultSet->GetInt32ByIndex(0, OutIntValue));
    EXPECT_EQ(222, OutIntValue);

    const char* OutStringValue;
    ASSERT_TRUE(ResultSet->GetStringByIndex(1, OutStringValue));
    EXPECT_STREQ(StrValue, OutStringValue);

    const byte* OutBinValue;
    size_t OutBinLen = 0;

    ASSERT_TRUE(ResultSet->GetBinaryByIndex(2, OutBinValue, OutBinLen));
    EXPECT_EQ(4, OutBinLen);
    EXPECT_EQ(0x10, OutBinValue[0]);
    EXPECT_EQ(0x20, OutBinValue[1]);
    EXPECT_EQ(0x00, OutBinValue[2]);
    EXPECT_EQ(0x30, OutBinValue[3]);
    Connection->DestroyStatement(ResultStatement);
}

// http://gtko.springnote.com/pages/3390193
const char* test_sp =
    //"DELIMITER $$\n"
    "CREATE PROCEDURE test_sp($param int)\n"
    "BEGIN\n"
    "    DECLARE $val int;\n"
    "    SET $val = ($param + 10)*2;\n"
    "    SELECT $val; \n"
    "END"
    //"$$\n"
;

// http://dev.mysql.md/doc/refman/6.0/en/c-api-prepared-call-statements.html
TEST_F(MYSQL_TEST_BASE, TESTStoreadProcedure)
{
    IDBStatement* CreateSP = Connection->CreateStatement();
    ASSERT_TRUE(CreateSP != NULL);
    ASSERT_TRUE(CreateSP->Execute("DROP PROCEDURE IF EXISTS test_sp"));
    ASSERT_TRUE(CreateSP->Execute(test_sp));
    Connection->DestroyStatement(CreateSP);

    IDBCallableStatement* Statement = Connection->CreateCallableStatement("Call test_sp(?);");
    ASSERT_TRUE(Statement != NULL);
    Statement->SetInt32ByIndex(0, 1);
    ASSERT_TRUE(Statement->Execute());

    IDBResultSet* ResultSet = Statement->GetResultSet();
    ASSERT_TRUE(ResultSet != NULL);
    ASSERT_TRUE(ResultSet->Next());

    int OutValue;
    ASSERT_TRUE(ResultSet->GetInt32ByIndex(0, OutValue));
    EXPECT_EQ(22, OutValue);
    bool MoreResult = Statement->GetMoreResults();
    if (MoreResult)
    {
        ResultSet = Statement->GetResultSet();
        ASSERT_TRUE(ResultSet != NULL);
    }

    Statement->SetInt32ByIndex(0, 2);
    ASSERT_TRUE(Statement->Execute());

    ResultSet = Statement->GetResultSet();
    ASSERT_TRUE(ResultSet != NULL);
    ASSERT_TRUE(ResultSet->Next());

    ASSERT_TRUE(ResultSet->GetInt32ByIndex(0, OutValue));
    EXPECT_EQ(24, OutValue);
    MoreResult = Statement->GetMoreResults();
    if (MoreResult)
    {
        ResultSet = Statement->GetResultSet();
        ASSERT_TRUE(ResultSet != NULL);
    }

    Connection->DestroyStatement(Statement);
}

TEST_F(MYSQL_TEST_BASE, TEST_OUT_OF_SYNC_ERROR)
{
    const char* CreateQuery =
        "CREATE TABLE id_generators"
        "("
        "value INT NOT NULL AUTO_INCREMENT,"
        "id VARCHAR(128) NOT NULL UNIQUE,"
        "PRIMARY KEY(value)"
        ")";

    IDBStatement* CreateTableStmt = Connection->CreateStatement();
    ASSERT_TRUE(CreateTableStmt != NULL && CreateTableStmt->Execute("DROP TABLE IF EXISTS id_generators"));
    ASSERT_TRUE(CreateTableStmt->Execute(CreateQuery));
    ASSERT_TRUE(CreateTableStmt->Execute("INSERT INTO id_generators (id) VALUE('mmonode_id')"));
    ASSERT_TRUE(Connection->DestroyStatement(CreateTableStmt));

    IDBPreparedStatement* pSelectStmt =
        Connection->CreatePreparedStatement("SELECT value FROM id_generators WHERE id = ?");
    ASSERT_TRUE(pSelectStmt != NULL);
    ASSERT_TRUE(pSelectStmt->SetStringByIndex(0, "mmonode_id", strlen("mmonode_id")));
    ASSERT_TRUE(pSelectStmt->Execute());
    IDBResultSet* pResult = pSelectStmt->GetResultSet();
    ASSERT_TRUE(pResult != NULL);
    int nVal = 0;
    ASSERT_TRUE(pResult->Next());
    ASSERT_TRUE(pResult->GetInt32ByName("value", nVal));

    ASSERT_TRUE(pSelectStmt->CloseResultSet());

    IDBPreparedStatement* pUpdateStmt =
        Connection->CreatePreparedStatement("UPDATE id_generators SET value = ? WHERE id = ?");
    ASSERT_TRUE(pUpdateStmt != NULL);

    Connection->DestroyStatement(pSelectStmt);
    Connection->DestroyStatement(pUpdateStmt);
}

TEST_F(MYSQL_TEST_WITH_RANDOM_TABLENAME, GetLastId)
{
    const char* CreateQueryFormat =
        "CREATE TABLE %s"
        "("
        "id INTEGER AUTO_INCREMENT PRIMARY KEY,"
        "name VARCHAR(128) NOT NULL UNIQUE"
        ")";

    fm_string Query;
    IDBStatement* CreateTableStmt = Connection->CreateStatement();
    Query.format("DROP TABLE IF EXISTS %s", TableName);
    ASSERT_TRUE(CreateTableStmt != NULL && CreateTableStmt->Execute(*Query));
    Query.format(CreateQueryFormat, TableName);
    ASSERT_TRUE(CreateTableStmt->Execute(*Query));
    // check invalid autoincremented value
    uint64 AutoIncrementedValue = CreateTableStmt->GetAutoIncrementedValue();
    ASSERT_TRUE(Connection->DestroyStatement(CreateTableStmt));

    IDBStatement* InsertStatement = Connection->CreateStatement();
    ASSERT_TRUE(InsertStatement != NULL);
    Query.format("INSERT INTO %s (name) VALUES('dummy_id')", TableName);
    ASSERT_TRUE(InsertStatement->Execute(*Query));
    ASSERT_TRUE(InsertStatement->GetAutoIncrementedValue() == 1);
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

TEST_F(MYSQL_TEST_WITH_TABLE_AND_DBSTATEMENT, GetLastId_Invalid)
{
    fm_string Query;
    Query.format("SELECT * FROM %s ", TableName);
    IDBStatement* SelectStatement = Connection->CreateStatement();
    ASSERT_TRUE(SelectStatement != NULL);
    ASSERT_TRUE(SelectStatement->Execute(*Query));
    ASSERT_EQ(0, SelectStatement->GetAutoIncrementedValue()); // read from InsertStatement value
    ASSERT_TRUE(SelectStatement->CloseResultSet());
    ASSERT_TRUE(Connection->DestroyStatement(SelectStatement));
}

TEST_F(MYSQL_TEST_WITH_TABLE_AND_DBSTATEMENT, DBAPIHELPER)
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
    ASSERT_FALSE(DBAPIHelper::Exec(Statement, "error", "zero", -1.0));
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

///////////////
#include "QueryDef.h"
///////////////

TEST(QUERYMAPPER, CREATE)
{
    IQueryMapper* QM1 = NULL;
    IQueryMapper* QM2 = NULL;
    QM1 = GetQueryMapper<TEST_PREPARED_QUERIES>();
    QM2 = GetQueryMapper<TEST_CALLABLE_QUERIES>();
    ASSERT_NE(QM1, QM2);
}

struct MySQLCachedConnection
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

typedef DBAPI_TEST_CACHEDCONNECTION_WITH_TABLE<MySQLConnector, MySQLCachedConnection, DummyTableName, DefaultSchemaQuery> MYSQL_TEST_CACHEDCONNECTION_WITH_TABLE;

TEST_F(MYSQL_TEST_CACHEDCONNECTION_WITH_TABLE, CachedConnection_CREATE)
{
    IDBStatement* S1 = Connection->GetStatement();
    IDBStatement* S2 = Connection->GetStatement();
    ASSERT_EQ(S1, S2);
    ASSERT_TRUE(S1->Execute("SELECT 111;"));
    ASSERT_TRUE(S2->Execute("SELECT 222;"));
}

TEST_F(MYSQL_TEST_CACHEDCONNECTION_WITH_TABLE, CachedConnection_GetPreparedStatement)
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

TEST(MYSQL_TEST_CACHEDCONNECTION, CachedConnection_GetCallableStatement)
{
    IDBResultSet* ResultSet;
    int64 OutIntValue;
    const char* OutStringValue;

    IDBConnection* Connection = MySQLConnector::Connect();
    ICachedDBConnection* CachedConnection = DriverManager::CreateCachedConnection(Connection,
            GetQueryMapper<TEST_PREPARED_QUERIES>(),
            GetQueryMapper<TEST_CALLABLE_QUERIES>());
    ASSERT_TRUE(CachedConnection != NULL);
    IDBPreparedStatement* S1 = CachedConnection->GetCallableStatement(CSELECT0);
    IDBPreparedStatement* S2 = CachedConnection->GetCallableStatement(CSELECT1);
    ASSERT_NE(S1, S2);

    ASSERT_TRUE(S1->Execute());
    ResultSet = S1->GetResultSet();
    ASSERT_TRUE(ResultSet != NULL);

    ASSERT_TRUE(ResultSet->Next());
    ASSERT_TRUE(DBAPIHelper::Get(ResultSet, OutIntValue));
    EXPECT_EQ(100, OutIntValue);

    ASSERT_TRUE(S2->Execute());
    ResultSet = S2->GetResultSet();
    ASSERT_TRUE(ResultSet != NULL);
    ASSERT_TRUE(ResultSet->Next());
    ASSERT_TRUE(DBAPIHelper::Get(ResultSet, OutStringValue));
    EXPECT_STREQ("100", OutStringValue);

    DriverManager::DestroyCachedConnection(CachedConnection);
}

TEST(MYSQL_TEST_CACHEDCONNECTION, CreateCachedConnection)
{
    std::string con;
    int64 OutIntValue;
    const char* OutStringValue;
    IDBResultSet* ResultSet;
    ICachedDBConnection* CachedConnection;
    IDBPreparedStatement* S1;

    con = std::string("mysql://") + host + "/" + db + "?" + "user=" + user + "&" + "password=" + password;
    CachedConnection = DriverManager::CreateCachedConnection(con.c_str(),
            GetQueryMapper<TEST_PREPARED_QUERIES>(),
            NULL);
    S1 = CachedConnection->GetPreparedStatement(PSELECT0);
    ASSERT_TRUE(S1->Execute());
    ResultSet = S1->GetResultSet();
    ASSERT_TRUE(ResultSet != NULL);
    ASSERT_TRUE(ResultSet->Next());
    ASSERT_TRUE(DBAPIHelper::Get(ResultSet, OutIntValue, OutStringValue));
    EXPECT_EQ(0, OutIntValue);
    EXPECT_STREQ("0", OutStringValue);
    DriverManager::DestroyCachedConnection(CachedConnection);

    CachedConnection = DriverManager::CreateCachedConnection(eDT_MySQL,
            host, 0, db, user, password,
            GetQueryMapper<TEST_PREPARED_QUERIES>(),
            NULL);
    S1 = CachedConnection->GetPreparedStatement(PSELECT1);
    ASSERT_TRUE(S1->Execute());
    ResultSet = S1->GetResultSet();
    ASSERT_TRUE(ResultSet != NULL);
    ASSERT_TRUE(ResultSet->Next());
    ASSERT_TRUE(DBAPIHelper::Get(ResultSet, OutIntValue, OutStringValue));
    EXPECT_EQ(10, OutIntValue);
    EXPECT_STREQ("20", OutStringValue);
    DriverManager::DestroyCachedConnection(CachedConnection);
}

TEST_F(MYSQL_TEST_CACHEDCONNECTION_WITH_TABLE, CachedConnection_PreparedStatement)
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

TEST_F(MYSQL_TEST_CACHEDCONNECTION_WITH_TABLE, NotGetResultTest)
{
    IDBPreparedStatement* Statement = NULL;
    IDBResultSet* ResultSet = NULL;
    Statement = Connection->GetPreparedStatement(PINSERT1);
    ASSERT_TRUE(Statement != NULL);
    ASSERT_TRUE(DBAPIHelper::Exec(Statement, 1, "success1", 1.0));
    ASSERT_TRUE(DBAPIHelper::Exec(Statement, 2, "success2", 2.0));
    ASSERT_TRUE(DBAPIHelper::Exec(Statement, 3, "success3", 3.0));
    ASSERT_TRUE(DBAPIHelper::Exec(Statement, 4, "success4", 4.0));
    ASSERT_TRUE(DBAPIHelper::Exec(Statement, 5, "success5", 5.0));
    ASSERT_TRUE(Statement->GetUpdateCount() == 1);

    Statement = Connection->GetPreparedStatement(PSELECT);
    ASSERT_TRUE(Statement != NULL);
    for (int i = 0; i < 10; i++)
    {
        ASSERT_TRUE(DBAPIHelper::Exec(Statement, 1));
        ASSERT_TRUE(DBAPIHelper::Exec(Statement, 2));
        ResultSet = Statement->GetResultSet();
        Statement->CloseResultSet();
        ASSERT_TRUE(DBAPIHelper::Exec(Statement, 3));
        ASSERT_TRUE(DBAPIHelper::Exec(Statement, 4));
        ResultSet = Statement->GetResultSet();
        ASSERT_TRUE(DBAPIHelper::Exec(Statement, 5));
        Statement->CloseResultSet();
    }
}

TEST_F(MYSQL_TEST_CACHEDCONNECTION_WITH_TABLE, NotGetResultTest2)
{
    IDBStatement* Statement = NULL;
    IDBPreparedStatement* PreparedStatement = NULL;
    IDBResultSet* ResultSet = NULL;
    PreparedStatement = Connection->GetPreparedStatement(PINSERT1);
    ASSERT_TRUE(PreparedStatement != NULL);

    ASSERT_TRUE(DBAPIHelper::Exec(PreparedStatement, 1, "success1", 1.0));
    ASSERT_TRUE(PreparedStatement->GetUpdateCount() == 1);
    ASSERT_TRUE(DBAPIHelper::Exec(PreparedStatement, 2, "success2", 2.0));
    ASSERT_TRUE(PreparedStatement->GetUpdateCount() == 1);
    ASSERT_TRUE(DBAPIHelper::Exec(PreparedStatement, 3, "success3", 3.0));
    ASSERT_TRUE(PreparedStatement->GetUpdateCount() == 1);
    ASSERT_TRUE(DBAPIHelper::Exec(PreparedStatement, 4, "success4", 4.0));
    ASSERT_TRUE(PreparedStatement->GetUpdateCount() == 1);
    ASSERT_TRUE(DBAPIHelper::Exec(PreparedStatement, 5, "success5", 5.0));
    ASSERT_TRUE(PreparedStatement->GetUpdateCount() == 1);

    for (int i = 0; i < 10; i++)
    {
        Statement = Connection->GetStatement();
        ASSERT_TRUE(Statement != NULL);
        Statement->Execute("SELECT * FROM dummy_table WHERE IntValue = 1");
        Statement->Execute("SELECT * FROM dummy_table WHERE IntValue = 2");
        ResultSet = Statement->GetResultSet();
        Statement->CloseResultSet();
        Statement->Execute("SELECT * FROM dummy_table WHERE IntValue = 3");
        Statement->Execute("SELECT * FROM dummy_table WHERE IntValue = 4");
        ResultSet = Statement->GetResultSet();
        Statement->Execute("SELECT * FROM dummy_table WHERE IntValue = 5");
    }
}

const char* test_sp2 =
    //"DELIMITER $$\n"
    "CREATE PROCEDURE test_sp2($param int)\n"
    "BEGIN\n"
    "    DECLARE $val int;\n"
    "    SET $val = $param * 100;\n"
    "    IF $param = 1\n"
    "    THEN\n"
    "       SELECT $val;\n"
    "    ELSE\n"
    "       SET $val = $val * 2;\n"
    "       SELECT $val, $param;\n"
    "    END IF; \n"
    "END"
    //"$$\n"
;

TEST_F(MYSQL_TEST_CACHEDCONNECTION_WITH_TABLE, TESTStoreadProcedureDiffrentResult)
{
    IDBConnection* Connection = MySQLConnector::Connect();
    IDBStatement* CreateSP = Connection->CreateStatement();
    ASSERT_TRUE(CreateSP != NULL);
    ASSERT_TRUE(CreateSP->Execute("DROP PROCEDURE IF EXISTS test_sp2"));
    ASSERT_TRUE(CreateSP->Execute(test_sp2));
    Connection->DestroyStatement(CreateSP);

    IDBCallableStatement* Statement = NULL;
    IDBResultSet* ResultSet = NULL;
    int OutIntValue;

    ICachedDBConnection* CachedConnection = DriverManager::CreateCachedConnection(Connection,
            NULL,
            GetQueryMapper<TEST_CALLABLE_QUERIES>());
    ASSERT_TRUE(CachedConnection != NULL);
    Statement = CachedConnection->GetCallableStatement(TESTSP2);
    ASSERT_TRUE(Statement != NULL);
    for (int i = 0; i < 10; i++)
    {
        ASSERT_TRUE(DBAPIHelper::Exec(Statement, 1));
        ResultSet = Statement->GetResultSet();
        ASSERT_TRUE(ResultSet != NULL);
        ASSERT_TRUE(ResultSet->Next());
        ASSERT_TRUE(DBAPIHelper::Get(ResultSet, OutIntValue));
        ASSERT_EQ(100, OutIntValue);
        ASSERT_TRUE(Statement->CloseResultSet());
    }

    ASSERT_TRUE(DBAPIHelper::Exec(Statement, 2));
    ResultSet = Statement->GetResultSet();
    ASSERT_TRUE(ResultSet != NULL);
    ASSERT_TRUE(ResultSet->Next());
    int ParamInt;
    ASSERT_TRUE(DBAPIHelper::Get(ResultSet, OutIntValue, ParamInt));
    ASSERT_EQ(400, OutIntValue);
    ASSERT_EQ(2, ParamInt);
    ASSERT_TRUE(Statement->CloseResultSet());

    DriverManager::DestroyCachedConnection(CachedConnection);
}