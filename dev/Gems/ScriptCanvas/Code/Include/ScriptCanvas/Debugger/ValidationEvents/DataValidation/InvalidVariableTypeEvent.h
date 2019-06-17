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

#include <AzCore/Component/EntityId.h>

#include <ScriptCanvas/Debugger/ValidationEvents/ValidationEvent.h>

#include <ScriptCanvas/Debugger/ValidationEvents/DataValidation/DataValidationIds.h>

namespace ScriptCanvas
{
    class InvalidVariableTypeEvent
        : public ValidationEvent
    {
    public:
        AZ_CLASS_ALLOCATOR(InvalidVariableTypeEvent, AZ::SystemAllocator, 0);
        AZ_RTTI(InvalidVariableTypeEvent, "{85670BDB-ED14-44BC-A7F1-FFF0D749EF72}", ValidationEvent);
        
        InvalidVariableTypeEvent(const VariableId& variableId)
            : ValidationEvent(ValidationSeverity::Error)
            , m_variableId(variableId)
        {
            SetDescription(AZStd::string::format("Variable with id %s has an invalid type.", variableId.ToString().c_str()));
        }
        
        bool CanAutoFix() const
        {
            return true;
        }
        
        AZStd::string GetIdentifier() const
        {
            return DataValidationIds::InvalidVariableTypeId;
        }
        
        AZ::Crc32 GetIdCrc() const
        {
            return DataValidationIds::InvalidVariableTypeCrc;
        }
        
        const VariableId& GetVariableId() const
        {
            return m_variableId;
        }

        AZStd::string_view GetTooltip() const
        {
            return "Invalid type for variable, auto fixing will remove all invalid variable nodes.";
        }
        
    private:
    
        VariableId m_variableId;
    };
}
