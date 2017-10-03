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

#include "precompiled.h"

#include "PureData.h"

namespace ScriptCanvas
{
    const char* PureData::k_getThis("Get");
    const char* PureData::k_setThis("Set");
    
    void PureData::AddInputAndOutputTypeSlot(const Data::Type& type, const void* source)
    {
        const Data::Type copy(type);
        AddInputDatumSlot(k_setThis, "", AZStd::move(Data::Type(type)), source, Datum::eOriginality::Original);
        AddOutputTypeSlot(k_getThis, "", AZStd::move(Data::Type(copy)), OutputStorage::Optional);
    }

    void PureData::AddInputTypeAndOutputTypeSlot(const Data::Type& type)
    {
        const Data::Type copy(type);
        AddInputTypeSlot(k_setThis, "", AZStd::move(Data::Type(type)), InputTypeContract::DatumType);
        AddOutputTypeSlot(k_getThis, "", AZStd::move(Data::Type(copy)), OutputStorage::Optional);
    }

    void PureData::OnActivate()
    {
        PushThis();
    }

    void PureData::OnInputChanged(const Datum& input, const SlotId& id) 
    {
        OnOutputChanged(input);
    }

    AZStd::string_view PureData::GetInputDataName() const
    {
        return k_setThis;
    }

    AZStd::string_view PureData::GetOutputDataName() const
    {
        return k_getThis;
    }

    void PureData::Reflect(AZ::ReflectContext* reflectContext)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(reflectContext);
        if (serializeContext)
        {
            serializeContext->Class<PureData, Node>()
                ->Version(0)
                ;

            AZ::EditContext* editContext = serializeContext->GetEditContext();
            if (editContext)
            {
                editContext->Class<PureData>("PureData", "")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                    ;
            }
        }
    }
}
