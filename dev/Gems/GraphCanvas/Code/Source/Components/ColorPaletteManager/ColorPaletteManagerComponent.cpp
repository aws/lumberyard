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

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Math/Vector3.h>
#include <AzCore/Math/Vector4.h>

#include <Components/ColorPaletteManager/ColorPaletteManagerComponent.h>

#include <Styling/StyleHelper.h>

namespace GraphCanvas
{   
    /////////////////////////////////
    // ColorPaletteManagerComponent
    /////////////////////////////////
    void ColorPaletteManagerComponent::Reflect(AZ::ReflectContext* reflectContext)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(reflectContext);
        
        if (serializeContext)
        {
            serializeContext->Class<ColorPaletteManagerComponent>()
                ->Version(1)
            ;
        }
    }
    
    ColorPaletteManagerComponent::ColorPaletteManagerComponent()
    {
    }
    
    ColorPaletteManagerComponent::~ColorPaletteManagerComponent()
    {
        for (auto& mapPair : m_styleHelpers)
        {
            delete mapPair.second;
        }
    }
    
    void ColorPaletteManagerComponent::Init()
    {
    }
    
    void ColorPaletteManagerComponent::Activate()    
    {
        ColorPaletteManagerRequestBus::Handler::BusConnect(GetEntityId());
    }
    
    void ColorPaletteManagerComponent::Deactivate()
    {
        ColorPaletteManagerRequestBus::Handler::BusConnect(GetEntityId());
    }
    
    void ColorPaletteManagerComponent::RefreshColorPalette()
    {
        for (auto& mapPair : m_styleHelpers)
        {
            mapPair.second->SetScene(GetEntityId());
            mapPair.second->SetStyle(mapPair.first);
        }
    }
    
    const Styling::StyleHelper* ColorPaletteManagerComponent::FindColorPalette(const AZStd::string& paletteString)
    {
        return FindCreateStyleHelper(paletteString);
    }
    
    const Styling::StyleHelper* ColorPaletteManagerComponent::FindDataColorPalette(const AZ::Uuid& dataType)
    {
        return FindCreateStyleHelper(FindDataColorPaletteString(dataType));
    }
    
    AZStd::string ColorPaletteManagerComponent::FindDataColorPaletteString(const AZ::Uuid& uuid) const
    {
        if (uuid == AZ::AzTypeInfo<bool>::Uuid())
        {
            return "BooleanDataColorPalette";
        }
        else if (uuid == AZ::AzTypeInfo<AZStd::string>::Uuid())
        {
            return "StringDataColorPalette";
        }
        else if (uuid == AZ::AzTypeInfo<AZ::EntityId>::Uuid())
        {
            return "EntityIdDataColorPalette";
        }
        else if (uuid == AZ::AzTypeInfo<char>::Uuid()
            || uuid == AZ::AzTypeInfo<AZ::s8>::Uuid()
            || uuid == AZ::AzTypeInfo<short>::Uuid()
            || uuid == AZ::AzTypeInfo<int>::Uuid()
            || uuid == AZ::AzTypeInfo<long>::Uuid()
            || uuid == AZ::AzTypeInfo<AZ::s64>::Uuid()
            || uuid == AZ::AzTypeInfo<unsigned char>::Uuid()
            || uuid == AZ::AzTypeInfo<unsigned short>::Uuid()
            || uuid == AZ::AzTypeInfo<unsigned int>::Uuid()
            || uuid == AZ::AzTypeInfo<unsigned long>::Uuid()
            || uuid == AZ::AzTypeInfo<AZ::u64>::Uuid()
            || uuid == AZ::AzTypeInfo<float>::Uuid()
            || uuid == AZ::AzTypeInfo<double>::Uuid()
            || uuid == AZ::AzTypeInfo<AZ::VectorFloat>::Uuid()
            )
        {
            return "NumberDataColorPalette";
        }
        else if (uuid == AZ::AzTypeInfo<AZ::Vector2>::Uuid()
                || uuid == AZ::AzTypeInfo<AZ::Vector3>::Uuid()
                || uuid == AZ::AzTypeInfo<AZ::Vector4>::Uuid())
        {
            return "VectorDataColorPalette";
        }
        else if (uuid.IsNull())
        {
            return "UnknownDataColorPalette";
        }
        else
        {
            return "ObjectDataColorPalette";
            
        }
    }
    
    Styling::StyleHelper* ColorPaletteManagerComponent::FindCreateStyleHelper(const AZStd::string& paletteString)
    {
        Styling::StyleHelper* styleHelper = nullptr;
        
        auto styleHelperIter = m_styleHelpers.find(paletteString);
        
        if (styleHelperIter == m_styleHelpers.end())
        {
            styleHelper = aznew Styling::StyleHelper();
            styleHelper->SetScene(GetEntityId());
            styleHelper->SetStyle(paletteString);
            
            m_styleHelpers[paletteString] = styleHelper;
        }
        else
        {
            styleHelper = styleHelperIter->second;
        }
        
        return styleHelper;
    }
}