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

#include <ScriptCanvas/Variable/VariableDatumBase.h>

namespace ScriptCanvas
{
    class VariableDatum
        : public VariableDatumBase
    {
    public:
        AZ_TYPE_INFO(VariableDatum, "{E0315386-069A-4061-AD68-733DCBE393DD}");
        AZ_CLASS_ALLOCATOR(VariableDatum, AZ::SystemAllocator, 0);
        static void Reflect(AZ::ReflectContext* context);

        VariableDatum();
        VariableDatum(const Datum& variableData);
        VariableDatum(Datum&& variableData);

        bool operator==(const VariableDatum& rhs) const;
        bool operator!=(const VariableDatum& rhs) const;

        AZ::Crc32 GetInputControlVisibility() const;
        void SetInputControlVisibility(const AZ::Crc32& inputControlVisibility);
        
        AZ::Crc32 GetVisibility() const;
        void SetVisibility(AZ::Crc32 visibility);

        // Temporary work around. Eventually we're going to want a bitmask so we can have multiple options here.
        // But the editor functionality isn't quite ready for this. So going to bias this towards maintaining a
        // consistent editor rather then consistent data.
        bool ExposeAsComponentInput() const { return m_exposeAsInput; }
        void SetExposeAsComponentInput(bool exposeAsInput) { m_exposeAsInput = exposeAsInput;  }

        void GenerateNewId();

    private:
        friend bool VariableDatumVersionConverter(AZ::SerializeContext&, AZ::SerializeContext::DataElementNode&);
        void OnExposureChanged();

        // Still need to make this a proper bitmask, once we have support for multiple
        // input/output attributes. For now, just going to assume it's only the single flag(which is is).
        bool m_exposeAsInput;
        AZ::Crc32 m_inputControlVisibility;
        AZ::Crc32 m_visibility;
    };

    struct VariableNameValuePair
    {
        AZ_TYPE_INFO(VariableNameValuePair, "{C1732C54-5E61-4D00-9A39-5B919CF2F8E7}");
        AZ_CLASS_ALLOCATOR(VariableNameValuePair, AZ::SystemAllocator, 0);
        static void Reflect(AZ::ReflectContext* context);
        AZStd::string m_varName;
        VariableDatum m_varDatum;

    private:
        AZStd::string GetDescriptionOverride();
    };
}