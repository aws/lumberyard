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

#pragma once

#include <AzCore/base.h>
#include "SQLiteConnection.h"
#include <tuple>

namespace AzToolsFramework
{
    namespace SQLite
    {
        //! Represents a binary data blob.  Needed so that Bind can accept a pointer and size as a single type
        struct SqlBlob
        {
            void* m_data;
            int m_dataSize;
        };

        //! Represents a single query parameter, where T is the type of the field
        template<typename T>
        struct SqlParam
        {
            explicit SqlParam(const char* parameterName) : m_parameterName(parameterName) {}

            const char* m_parameterName;
        };

        //////////////////////////////////////////////////////////////////////////

        namespace Internal
        {
            bool Bind(Statement* statement, int index, const AZ::Uuid& value);
            bool Bind(Statement* statement, int index, double value);
            bool Bind(Statement* statement, int index, AZ::s32 value);
            bool Bind(Statement* statement, int index, AZ::u32 value);
            bool Bind(Statement* statement, int index, const char* value);
            bool Bind(Statement* statement, int index, AZ::s64 value);
            bool Bind(Statement* statement, int index, AZ::u64 value);
            bool Bind(Statement* statement, int index, const SqlBlob& value);
        }

        template<typename... T>
        class SqlQuery
        {
        public:
            SqlQuery(const char* statementName, const char* statement, const char* logName, SqlParam<T>&&... parameters) :
                m_statementName(statementName),
                m_statement(statement),
                m_logName(logName),
                m_parameters(parameters...)
            {

            }

            //! Bind both prepares and binds the args - call it on an empty autoFinalizer and it will prepare
            //! the query for you and return a ready-to-go autoFinalizer that has a valid statment ready to 
            //! step()
            bool Bind(Connection& connection, StatementAutoFinalizer& autoFinalizer, const T&... args) const
            {
                // bind is meant to prepare the auto finalizer and prepare the connection, so assert if the
                // programmer has accidentally already bound it or prepared it first.
                AZ_Assert(!autoFinalizer.Get(), "Do not call Bind() on an autofinalizer that is already attached to a connection.");
                autoFinalizer = StatementAutoFinalizer(connection, m_statementName);
                Statement* statement = autoFinalizer.Get();

                if (statement == nullptr)
                {
                    AZ_Error(m_logName, false, "Could not find statement %s", m_statementName);
                    return false;
                }

                return BindInternal<0>(statement, args...);
            }

            //! BindAndStep will execute the given statement and then clean up afterwards.  It is for
            //! calls that perform some operation on the database rather than a query that you need the result of.
            bool BindAndStep(Connection& connection, const T&... args) const
            {
                StatementAutoFinalizer autoFinalizer;

                if (!Bind(connection, autoFinalizer, args...))
                {
                    return false;
                }

                if (autoFinalizer.Get()->Step() == Statement::SqlError)
                {
                    AZ_Warning(m_logName, false, "Failed to execute statement %s", m_statementName);
                    return false;
                }

                return true;
            }

        private:

            // Handles the 0-parameter case
            template<int TIndex>
            bool BindInternal(Statement*) const
            {
                return true;
            }

            template<int TIndex, typename T2>
            bool BindInternal(Statement* statement, const T2& value)  const
            {
                const SqlParam<T2>& sqlParam = std::get<TIndex>(m_parameters);

                int index = statement->GetNamedParamIdx(sqlParam.m_parameterName);

                if (!index)
                {
                    AZ_Error(m_logName, false, "Could not find the index for placeholder %s in statement %s ", sqlParam.m_parameterName, m_statementName);
                    return false;
                }

                Internal::Bind(statement, index, value);

                return true;
            }

            template<int TIndex, typename T2, typename... TArgs>
            bool BindInternal(Statement* statement, const T2& value, const TArgs&... args) const
            {
                return BindInternal<TIndex>(statement, value)
                    && BindInternal<TIndex + 1>(statement, args...);
            }

        public:
            const char* m_statementName;
            const char* m_statement;
            const char* m_logName;
            std::tuple<SqlParam<T>...> m_parameters;
        };

        template<typename TSqlQuery>
        void AddStatement(Connection* connection, const TSqlQuery& sqlQuery)
        {
            connection->AddStatement(sqlQuery.m_statementName, sqlQuery.m_statement);
        }

        template<typename... T>
        SqlQuery<T...> MakeSqlQuery(const char* statementName, const char* statement, const char* logName, SqlParam<T>&&... parameters)
        {
            return SqlQuery<T...>(statementName, statement, logName, AZStd::forward<SqlParam<T>>(parameters)...);
        }
    } // namespace SQLite
} // namespace AZFramework