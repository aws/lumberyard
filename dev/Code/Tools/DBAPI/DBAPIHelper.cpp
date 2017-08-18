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
#include "DBAPIHelper.h"
#include "IDBStatement.h"

namespace DBAPIHelper
{
    bool Exec(IDBPreparedStatement* Statement)
    {
        bool Result = Statement->Execute();
        return Result;
    }

    template<>
    bool SetParam(IDBPreparedStatement* Statement, unsigned int Index, int8 Value)
    {
        return Statement->SetInt8ByIndex(Index, Value);
    }

    template<>
    bool SetParam(IDBPreparedStatement* Statement, unsigned int Index, int16 Value)
    {
        return Statement->SetInt16ByIndex(Index, Value);
    }

    template<>
    bool SetParam(IDBPreparedStatement* Statement, unsigned int Index, int32 Value)
    {
        return Statement->SetInt32ByIndex(Index, Value);
    }

    template<>
    bool SetParam(IDBPreparedStatement* Statement, unsigned int Index, int64 Value)
    {
        return Statement->SetInt64ByIndex(Index, Value);
    }

    template<>
    bool SetParam(IDBPreparedStatement* Statement, unsigned int Index, uint8 Value)
    {
        return Statement->SetUInt8ByIndex(Index, Value);
    }

    template<>
    bool SetParam(IDBPreparedStatement* Statement, unsigned int Index, uint16 Value)
    {
        return Statement->SetUInt16ByIndex(Index, Value);
    }

    template<>
    bool SetParam(IDBPreparedStatement* Statement, unsigned int Index, uint32 Value)
    {
        return Statement->SetUInt32ByIndex(Index, Value);
    }

    template<>
    bool SetParam(IDBPreparedStatement* Statement, unsigned int Index, uint64 Value)
    {
        return Statement->SetUInt64ByIndex(Index, Value);
    }

    template<>
    bool SetParam(IDBPreparedStatement* Statement, unsigned int Index, float Value)
    {
        return Statement->SetFloatByIndex(Index, Value);
    }

    template<>
    bool SetParam(IDBPreparedStatement* Statement, unsigned int Index, double Value)
    {
        return Statement->SetDoubleByIndex(Index, Value);
    }

    template<>
    bool SetParam(IDBPreparedStatement* Statement, unsigned int Index, char* Value)
    {
        return Statement->SetStringByIndex(Index, Value, strlen(Value));
    }

    template<>
    bool SetParam(IDBPreparedStatement* Statement, unsigned int Index, const char* Value)
    {
        return Statement->SetStringByIndex(Index, Value, strlen(Value));
    }

    template<>
    bool GetValue(IDBResultSet* ResultSet, unsigned int Index, int8& Value)
    {
        return ResultSet->GetInt8ByIndex(Index, Value);
    }

    template<>
    bool GetValue(IDBResultSet* ResultSet, unsigned int Index, int16& Value)
    {
        return ResultSet->GetInt16ByIndex(Index, Value);
    }

    template<>
    bool GetValue(IDBResultSet* ResultSet, unsigned int Index, int32& Value)
    {
        return ResultSet->GetInt32ByIndex(Index, Value);
    }

    template<>
    bool GetValue(IDBResultSet* ResultSet, unsigned int Index, int64& Value)
    {
        return ResultSet->GetInt64ByIndex(Index, Value);
    }

    template<>
    bool GetValue(IDBResultSet* ResultSet, unsigned int Index, uint8& Value)
    {
        return ResultSet->GetUInt8ByIndex(Index, Value);
    }

    template<>
    bool GetValue(IDBResultSet* ResultSet, unsigned int Index, uint16& Value)
    {
        return ResultSet->GetUInt16ByIndex(Index, Value);
    }

    template<>
    bool GetValue(IDBResultSet* ResultSet, unsigned int Index, uint32& Value)
    {
        return ResultSet->GetUInt32ByIndex(Index, Value);
    }

    template<>
    bool GetValue(IDBResultSet* ResultSet, unsigned int Index, uint64& Value)
    {
        return ResultSet->GetUInt64ByIndex(Index, Value);
    }

    template<>
    bool GetValue(IDBResultSet* ResultSet, unsigned int Index, float& Value)
    {
        return ResultSet->GetFloatByIndex(Index, Value);
    }

    template<>
    bool GetValue(IDBResultSet* ResultSet, unsigned int Index, double& Value)
    {
        return ResultSet->GetDoubleByIndex(Index, Value);
    }

    template<>
    bool GetValue(IDBResultSet* ResultSet, unsigned int Index, const char*& Value)
    {
        return ResultSet->GetStringByIndex(Index, Value);
    }
}