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

#ifndef CRYINCLUDE_TOOLS_DBAPI_UNITTEST_TESTCASEUTIL_H
#define CRYINCLUDE_TOOLS_DBAPI_UNITTEST_TESTCASEUTIL_H
#pragma once

#include "StringUtil.h"

class TEST_BASE
    : public ::testing::Test
{
public:
    TEST_BASE()
        : m_testInfo(testing::UnitTest::GetInstance()->current_test_info())
        , m_testCaseName(m_testInfo->test_case_name())
        , m_testName(m_testInfo->name())
    {
    }

protected:
    const ::testing::TestInfo* const    m_testInfo;
    const char*                         m_testCaseName;
    const char*                         m_testName;
};

template<class Connector>
class DBAPI_TEST_BASE
    : public TEST_BASE
{
public:
    DBAPI_TEST_BASE()
        : Connection(NULL)
    {
    }

    void SetUp()
    {
        Connection = Connector::Connect();
        ASSERT_TRUE(Connection != NULL);
    }

    void TearDown()
    {
        ASSERT_TRUE(Connection != NULL);
        DriverManager::DestroyConnection(Connection);
    }

protected:
    IDBConnection*   Connection;
};

class DefaultTableNameGenerator
{
public:
    std::string& GenerateName(const char* Category, const char* Prefix)
    {
        unsigned long UniqueCount = ::GetTickCount();

        fm_string Name;
        if (Category != NULL && Prefix != NULL)
        {
            Name.format("%s_%s_%d", Category, Prefix, UniqueCount);
        }
        else if (Prefix != NULL)
        {
            Name.format("%s_%d", Prefix, UniqueCount);
        }
        else if (Category != NULL)
        {
            Name.format("%s_%d", Category, UniqueCount);
        }
        else
        {
            Name.format("%d", UniqueCount);
        }
        TableName.assign(*Name);
        return TableName;
    }
    const char* GetName()
    {
        return TableName.c_str();
    }
protected:
    std::string TableName;
};

template<class Connector, class TableNameGenerator>
class DBAPI_TEST_WITH_RANDOM_TABLENAME
    : public DBAPI_TEST_BASE<Connector>
{
public:
    DBAPI_TEST_WITH_RANDOM_TABLENAME() {}

    void SetUp()
    {
        DBAPI_TEST_BASE::SetUp();
        m_TableNameGenerator.GenerateName(m_testCaseName, m_testName);
        TableName =  m_TableNameGenerator.GetName();
    }

    void TearDown()
    {
        DBAPI_TEST_BASE::TearDown();
    }

protected:
    TableNameGenerator  m_TableNameGenerator;
    const char*   TableName;
};

template<class Connector, class TableNameGenerator, class SchemaQuery>
class DBAPI_TEST_WITH_FIXED_TABLE
    : public DBAPI_TEST_WITH_RANDOM_TABLENAME<Connector, TableNameGenerator>
{
public:
    DBAPI_TEST_WITH_FIXED_TABLE()
        : m_DDLStatement(NULL)
    {
    }

    void SetUp()
    {
        DBAPI_TEST_WITH_RANDOM_TABLENAME::SetUp();

        m_DDLStatement = Connection->CreateStatement();
        ASSERT_TRUE(m_DDLStatement != NULL);
        fm_string CreateTableQuery;
        CreateTableQuery.format(SchemaQuery::GetCreateTableQuery(), TableName);
        ASSERT_TRUE(m_DDLStatement->Execute(*CreateTableQuery));
    }

    void TearDown()
    {
        ASSERT_TRUE(Connection != NULL);
        fm_string DropTableQuery;
        DropTableQuery.format(SchemaQuery::GetDropTableQuery(), TableName);
        ASSERT_TRUE(m_DDLStatement->Execute(*DropTableQuery));
        //ASSERT_TRUE(Connection->DestroyStatement(m_DDLStatement));
        Connection->DestroyStatement(m_DDLStatement);

        DBAPI_TEST_WITH_RANDOM_TABLENAME::TearDown();
    }

protected:
    IDBStatement*   m_DDLStatement;
};

template<class Connector, class TableNameGenerator, class SchemaQuery>
class DBAPI_TEST_WITH_TABLE_AND_DBSTATEMENT
    : public DBAPI_TEST_WITH_FIXED_TABLE<Connector, TableNameGenerator, SchemaQuery>
{
public:
    DBAPI_TEST_WITH_TABLE_AND_DBSTATEMENT()
    {
    }

    void SetUp()
    {
        DBAPI_TEST_WITH_FIXED_TABLE::SetUp();

        QueryStatement = Connection->CreateStatement();
        ASSERT_TRUE(QueryStatement != NULL);
    }

    void TearDown()
    {
        if (QueryStatement != NULL)
        {
            Connection->DestroyStatement(QueryStatement);
        }

        DBAPI_TEST_WITH_FIXED_TABLE::TearDown();
    }

protected:
    IDBStatement*   QueryStatement;
};

struct DefaultSchemaQuery
{
    static const char* GetCreateTableQuery()
    {
        return "CREATE TABLE %s (intValue int, stringValue varchar(45), doubleValue double);";
    }

    static const char* GetDropTableQuery()
    {
        return "DROP TABLE %s;";
    }
};

template<class Connector, class _CachedConnection, class _TableName, class SchemaQuery>
class DBAPI_TEST_CACHEDCONNECTION_WITH_TABLE
    : public DBAPI_TEST_BASE<Connector>
{
public:
    DBAPI_TEST_CACHEDCONNECTION_WITH_TABLE()
        : Connection(NULL)
        , TableName(NULL)
    {
    }

    void SetUp()
    {
        TEST_BASE::SetUp();

        IDBConnection* DBConnection = Connector::Connect();
        ASSERT_TRUE(DBConnection != NULL);

        TableName = _TableName::Get();
        IDBStatement* DDLStatement = DBConnection->CreateStatement();
        ASSERT_TRUE(DDLStatement != NULL);

        fm_string CreateTableQuery;
        CreateTableQuery.format("DROP TABLE IF EXISTS %s", TableName);
        ASSERT_TRUE(DDLStatement->Execute(*CreateTableQuery));
        CreateTableQuery.format(SchemaQuery::GetCreateTableQuery(), TableName);
        ASSERT_TRUE(DDLStatement->Execute(*CreateTableQuery));
        DBConnection->DestroyStatement(DDLStatement);

        Connection = _CachedConnection::Create(DBConnection);
    }

    void TearDown()
    {
        ASSERT_TRUE(Connection != NULL);
        DriverManager::DestroyCachedConnection(Connection);
        TEST_BASE::TearDown();
    }

protected:
    ICachedDBConnection*    Connection;
    const char*             TableName;
};

#endif // CRYINCLUDE_TOOLS_DBAPI_UNITTEST_TESTCASEUTIL_H
