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

#include <AzCore/Component/Component.h>

#include <Components/ColorPaletteManager/ColorPaletteManagerBus.h>

namespace GraphCanvas
{
    class ColorPaletteManagerComponent 
        : public AZ::Component
        , public ColorPaletteManagerRequestBus::Handler
    {
    public:
        AZ_COMPONENT(ColorPaletteManagerComponent, "{486B009F-632B-44F6-81C2-3838746190AE}");
        static void Reflect(AZ::ReflectContext* reflectContext);
        
        ColorPaletteManagerComponent();
        ~ColorPaletteManagerComponent() override;
        
        // Component
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
        {
            provided.push_back(ColorPaletteManagerServiceCrc);
        }

        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
        {
            incompatible.push_back(ColorPaletteManagerServiceCrc);
        }

        static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent)
        {
            (void)dependent;
        }

        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
        {
            (void)required;
        }

        void Init() override;
        void Activate() override;
        void Deactivate() override;
        ////
        
        // ColorPaletteRequestBus
        void RefreshColorPalette() override;
        
        const Styling::StyleHelper* FindColorPalette(const AZStd::string& paletteString) override;
        const Styling::StyleHelper* FindDataColorPalette(const AZ::Uuid& uuid) override;
        ////
        
    private:    
    
        AZStd::string FindDataColorPaletteString(const AZ::Uuid& uuid) const;
        Styling::StyleHelper* FindCreateStyleHelper(const AZStd::string& paletteString);

        AZStd::unordered_map< AZStd::string, Styling::StyleHelper* > m_styleHelpers;
    };
}