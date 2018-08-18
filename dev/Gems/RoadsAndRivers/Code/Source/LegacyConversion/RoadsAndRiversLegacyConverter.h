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

#include <LegacyEntityConversion/LegacyEntityConversionBus.h>

class CRoadObject;

namespace RoadsAndRivers
{
    class RoadsAndRiversConverter : public AZ::LegacyConversion::LegacyConversionEventBus::Handler
    {
    public:
        AZ_CLASS_ALLOCATOR(RoadsAndRiversConverter, AZ::SystemAllocator, 0);

        RoadsAndRiversConverter();
        virtual ~RoadsAndRiversConverter();

        // LegacyConversionEventBus::Handler
        AZ::LegacyConversion::LegacyConversionResult ConvertEntity(CBaseObject* entityToConvert) override;
        bool BeforeConversionBegins() override;
        bool AfterConversionEnds() override;

    private:
        static AZ::LegacyConversion::LegacyConversionResult ConvertRoad(CBaseObject* entityToConvert);
        static AZ::LegacyConversion::LegacyConversionResult ConvertRiver(CBaseObject* entityToConvert);

        static bool ConvertCommonProperties(AZ::Entity* entity, const char* componentId, CRoadObject& legacySpline);
        static void TransformSplineNodes(AZ::Entity* entity, const char* componentId, const CRoadObject& legacySpline);
    };
}
