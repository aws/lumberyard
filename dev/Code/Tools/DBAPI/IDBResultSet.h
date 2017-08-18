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

#ifndef CRYINCLUDE_TOOLS_DBAPI_IDBRESULTSET_H
#define CRYINCLUDE_TOOLS_DBAPI_IDBRESULTSET_H
#pragma once


class IDBStatement;
class IDBResultSet
{
public:
    virtual ~IDBResultSet() {}

    virtual int FindColumn(const char* ColumnName) const = 0;

    virtual IDBStatement* GetStatement() const = 0;

    // from ResultSetMetaData, not ResultSet
    virtual int GetColumnCount() const = 0;

    virtual int64 GetNumRows() = 0;

    virtual bool Next() = 0;

    virtual bool IsNullByIndex(unsigned int ColumnIndex) const = 0;

    virtual bool GetInt8ByIndex(unsigned int ColumnIndex, int8& OutValue) const = 0;

    virtual bool GetInt16ByIndex(unsigned int ColumnIndex, int16& OutValue) const = 0;

    virtual bool GetInt32ByIndex(unsigned int ColumnIndex, int32& OutValue) const = 0;

    virtual bool GetInt64ByIndex(unsigned int ColumnIndex, int64& OutValue) const = 0;

    virtual bool GetUInt8ByIndex(unsigned int ColumnIndex, uint8& OutValue) const = 0;

    virtual bool GetUInt16ByIndex(unsigned int ColumnIndex, uint16& OutValue) const = 0;

    virtual bool GetUInt32ByIndex(unsigned int ColumnIndex, uint32& OutValue) const = 0;

    virtual bool GetUInt64ByIndex(unsigned int ColumnIndex, uint64& OutValue) const = 0;

    virtual bool GetDoubleByIndex(unsigned int ColumnIndex, double& OutValue) const = 0;

    virtual bool GetFloatByIndex(unsigned int ColumnIndex, float& OutValue) const = 0;

    virtual bool GetStringByIndex(unsigned int ColumnIndex, const char*& OutValue) const = 0;

    virtual bool GetBinaryByIndex(unsigned int ColumnIndex, const byte*& OutValue, size_t& OutSize) const = 0;

    virtual bool GetDateByIndex(unsigned int ColumnIndex, time_t& OutValue) const = 0;

    virtual bool IsNullByName(const char* ColumnName) const = 0;

    virtual bool GetInt8ByName(const char* ColumnName, int8& OutValue) const = 0;

    virtual bool GetInt16ByName(const char* ColumnName, int16& OutValue) const = 0;

    virtual bool GetInt32ByName(const char* ColumnName, int32& OutValue) const = 0;

    virtual bool GetInt64ByName(const char* ColumnName, int64& OutValue) const = 0;

    virtual bool GetUInt8ByName(const char* ColumnName, uint8& OutValue) const = 0;

    virtual bool GetUInt16ByName(const char* ColumnName, uint16& OutValue) const = 0;

    virtual bool GetUInt32ByName(const char* ColumnName, uint32& OutValue) const = 0;

    virtual bool GetUInt64ByName(const char* ColumnName, uint64& OutValue) const = 0;

    virtual bool GetFloatByName(const char* ColumnName, float& OutValue) const = 0;

    virtual bool GetDoubleByName(const char* ColumnName, double& OutValue) const = 0;

    virtual bool GetStringByName(const char* ColumnName, const char*& OutValue) const = 0;

    virtual bool GetBinaryByName(const char* ColumnName, const byte*& OutValue, size_t& OutSize) const = 0;

    virtual bool GetDateByName(const char* ColumnName, time_t& OutValue) const = 0;
};

#endif // CRYINCLUDE_TOOLS_DBAPI_IDBRESULTSET_H
