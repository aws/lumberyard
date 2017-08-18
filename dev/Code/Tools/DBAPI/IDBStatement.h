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

#ifndef CRYINCLUDE_TOOLS_DBAPI_IDBSTATEMENT_H
#define CRYINCLUDE_TOOLS_DBAPI_IDBSTATEMENT_H
#pragma once

#include "stdafx.h"

class IDBResultSet;
class IDBConnection;
class IDBStatement
{
public:
    virtual ~IDBStatement() {};

    virtual bool Execute(const char* Query) = 0;

    virtual IDBResultSet* GetResultSet(bool bUnbuffered = false) = 0;

    virtual bool CloseResultSet() = 0;

    virtual bool GetMoreResults() = 0;

    virtual IDBConnection* GetConnection() const = 0;

    virtual int64 GetUpdateCount() const = 0;

    virtual int64 GetAutoIncrementedValue() const = 0;
};

class IDBPreparedStatement
    : public IDBStatement
{
public:
    virtual bool Execute(const char* Query) = 0;

    virtual bool Execute() = 0;

    virtual const char* GetQuery() const = 0;

    virtual bool SetNullByIndex(unsigned int ParamIndex) = 0;

    virtual bool SetInt8ByIndex(unsigned int ParamIndex, int8 Value) = 0;

    virtual bool SetInt16ByIndex(unsigned int ParamIndex, int16 Value) = 0;

    virtual bool SetInt32ByIndex(unsigned int ParamIndex, int32 Value) = 0;

    virtual bool SetInt64ByIndex(unsigned int ParamIndex, int64 Value) = 0;

    virtual bool SetUInt8ByIndex(unsigned int ParamIndex, uint8 Value) = 0;

    virtual bool SetUInt16ByIndex(unsigned int ParamIndex, uint16 Value) = 0;

    virtual bool SetUInt32ByIndex(unsigned int ParamIndex, uint32 Value) = 0;

    virtual bool SetUInt64ByIndex(unsigned int ParamIndex, uint64 Value) = 0;

    virtual bool SetDateByIndex(unsigned int ParamIndex, time_t Value) = 0;

    virtual bool SetFloatByIndex(unsigned int ParamIndex, float Value) = 0;

    virtual bool SetDoubleByIndex(unsigned int ParamIndex, double Value) = 0;

    virtual bool SetStringByIndex(unsigned int ParamIndex,
        const char* Buffer,
        size_t Length) = 0;

    virtual bool SetBinaryByIndex(unsigned int ParamIndex,
        const byte* Buffer,
        size_t Length) = 0;

    virtual bool SetNull(unsigned int ParamIndex) { return SetNullByIndex(ParamIndex); }
    virtual bool SetInt(unsigned int ParamIndex, int8 Value) { return SetInt8ByIndex(ParamIndex, Value); }
    virtual bool SetInt(unsigned int ParamIndex, int16 Value) { return SetInt16ByIndex(ParamIndex, Value); }
    virtual bool SetInt(unsigned int ParamIndex, int32 Value) { return SetInt32ByIndex(ParamIndex, Value); }
    virtual bool SetInt(unsigned int ParamIndex, int64 Value) { return SetInt64ByIndex(ParamIndex, Value); }
    virtual bool SetInt(unsigned int ParamIndex, uint8 Value) { return SetUInt8ByIndex(ParamIndex, Value); }
    virtual bool SetInt(unsigned int ParamIndex, uint16 Value) { return SetUInt16ByIndex(ParamIndex, Value); }
    virtual bool SetInt(unsigned int ParamIndex, uint32 Value) { return SetUInt32ByIndex(ParamIndex, Value); }
    virtual bool SetInt(unsigned int ParamIndex, uint64 Value) { return SetUInt64ByIndex(ParamIndex, Value); }
    virtual bool SetDate(unsigned int ParamIndex, time_t Value) { return SetDateByIndex(ParamIndex, Value); }
    virtual bool SetFloat(unsigned int ParamIndex, float Value) { return SetFloatByIndex(ParamIndex, Value); }
    virtual bool SetDouble(unsigned int ParamIndex, double Value) { return SetDoubleByIndex(ParamIndex, Value); }
    virtual bool SetString(unsigned int ParamIndex, const char* Buffer) { return SetStringByIndex(ParamIndex, Buffer, strlen(Buffer)); }
    virtual bool SetData(unsigned int ParamIndex, const byte* Buffer, int Length) { return SetBinaryByIndex(ParamIndex, Buffer, Length); }
};

class IDBCallableStatement
    : public IDBPreparedStatement
{
public:
};

#endif // CRYINCLUDE_TOOLS_DBAPI_IDBSTATEMENT_H
