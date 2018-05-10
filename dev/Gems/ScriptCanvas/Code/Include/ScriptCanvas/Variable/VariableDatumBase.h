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

#include <ScriptCanvas/Core/Datum.h>
#include <ScriptCanvas/Variable/VariableCore.h>

namespace ScriptCanvas
{
    class VariableDatumBase
    {
    public:
        AZ_TYPE_INFO(VariableDatumBase, "{93D2BD2B-1559-4968-B055-77736E06D3F2}");
        AZ_CLASS_ALLOCATOR(VariableDatumBase, AZ::SystemAllocator, 0);
        static void Reflect(AZ::ReflectContext* context);

        VariableDatumBase() = default;
        VariableDatumBase(const Datum& variableData);
        VariableDatumBase(Datum&& variableData);

        bool operator==(const VariableDatumBase& rhs) const;
        bool operator!=(const VariableDatumBase& rhs) const;

        const VariableId& GetId() const { return m_id; }
        const Datum& GetData() const { return m_data; }
        Datum& GetData() { return m_data; }

        template<typename DatumType, typename ValueType>
        bool SetValueAs(ValueType&& value)
        {
            if (auto dataValueType = m_data.ModAs<DatumType>())
            {
                (*dataValueType) = AZStd::forward<ValueType>(value);
                OnValueChanged();
                return true;
            }

            return false;
        }

    protected:
        void OnValueChanged();

        Datum m_data;
        VariableId m_id;
    };
}

namespace AZStd
{
    template<>
    struct hash<ScriptCanvas::VariableDatumBase>
    {
        size_t operator()(const ScriptCanvas::VariableDatumBase& key) const
        {
            return hash<ScriptCanvas::VariableId>()(key.GetId());
        }
    };
}
