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

#ifndef CRYINCLUDE_TOOLS_DBAPI_DBTYPES_H
#define CRYINCLUDE_TOOLS_DBAPI_DBTYPES_H
#pragma once


typedef signed char         int8;
typedef unsigned char       uint8;
typedef signed short        int16;
typedef unsigned short      uint16;
typedef signed int          int32;
typedef unsigned int        uint32;
typedef signed long long    int64;
typedef unsigned long long  uint64;
typedef unsigned char       byte;

enum EDBType
{
    eDT_NONE,
    eDT_Oracle,
    eDT_MSSQL,
    eDT_ODBC,
    eDT_SQLite,
};

enum EDBError
{
    eDE_SUCCESS = 0,
    eDB_UNKNOWN,
    eDB_OUT_OF_MEMORY,
    eDB_INVALID_CONNECTION_STRING,
    eDB_NOT_CONNECTED,
    eDB_NOT_SUPPORTED,
    eDB_INVALID_POINTER,
    eDB_INVALID_COLUMN_NAME,
    eDB_CLOSED_RESULTSET,
    eDB_CLOSED_STATEMENT,
    eDB_INVALID_COLUMN_INDEX,
    eDB_INVALID_PARAM_INDEX,
    eDB_VALUE_IS_NULL,
    eDB_VALUE_CANNOT_CONVERT,
    eDB_RESULT_IS_NOT_BOUND,
    eDB_INVALID_TYPE,
};

typedef unsigned int        QueryIDType;


#endif // CRYINCLUDE_TOOLS_DBAPI_DBTYPES_H
