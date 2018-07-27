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
#include "StdAfx.h"

#include <AzCore/Component/TransformBus.h>
#include <AzCore/Math/Spline.h>
#include <AzCore/Math/Vector2.h>
#include <AzCore/Math/Aabb.h>

#include <IEditor.h>
#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <AzToolsFramework/API/EditorVegetationRequestsBus.h>

#include <LmbrCentral/Shape/SplineComponentBus.h>

#include "EditorDefs.h"
#include "MathConversion.h"
#include "EditorUtils.h"
#include "SplineGeometry.h"
#include "VegetationMap.h"
#include "Terrain/Heightmap.h"
#include "VegetationObject.h"

#include <random>

namespace RoadsAndRivers
{
    namespace EditorUtils
    {
        using AzToolsFramework::EditorVegetation::EditorVegetationRequestsBus;

        struct SplineChunkAabbPoints
        {
            AZ::Vector2 m_min;
            AZ::Vector2 m_max;
        };

        /**
         * Returns AABB points in 2d space (xy plane) around piece of spline geometry, taking width modifications at the specified points into account
         * @param splineGeometry geometry to calculate points around
         * @param distanceBegin distance along the spline to sample first 2 points around
         * @param distanceAfterBegin distance from distanceBegin to sample second 2 points around
         * @param border geometry to generate point around
         * @return Points of 2d AABB that fully encompasses spline chunk from distanceBegin to distanceBegin+distanceAfterBegin, 
         * taking width and border at this points into account
         */
        SplineChunkAabbPoints GetEdgePointsOfSplineGeometryChunk(const SplineGeometry& splineGeometry, float distanceBegin, float distanceAfterBegin, float border)
        {
            AZ::Aabb aabb;
            aabb.SetNull();

            AZ::ConstSplinePtr spline = nullptr;
            LmbrCentral::SplineComponentRequestBus::EventResult(spline, splineGeometry.GetEntityId(), &LmbrCentral::SplineComponentRequests::GetSpline);

            AZ::Transform transform = AZ::Transform::CreateIdentity();
            AZ::TransformBus::EventResult(transform, splineGeometry.GetEntityId(), &AZ::TransformBus::Events::GetWorldTM);

            const float width01 = splineGeometry.GetWidthModifiers().GetWidthAt(distanceBegin) + border;
            const float width02 = splineGeometry.GetWidthModifiers().GetWidthAt(distanceBegin + distanceAfterBegin) + border;

            auto points01 = SplineGeometryMathUtils::GetPointsAroundSpline(width01, spline, distanceBegin);
            auto points02 = SplineGeometryMathUtils::GetPointsAroundSpline(width02, spline, distanceBegin + distanceAfterBegin);

            aabb.AddPoint(points01.first);
            aabb.AddPoint(points01.second);
            aabb.AddPoint(points02.first);
            aabb.AddPoint(points02.second);

            aabb.ApplyTransform(transform);

            return { AZ::Vector2(aabb.GetMin().GetX(), aabb.GetMin().GetY()), AZ::Vector2(aabb.GetMax().GetX(), aabb.GetMax().GetY()) };
        }

        void EraseVegetation(const SplineGeometry& splineGeometry, float eraseVegetationWidth, float eraseVegetationVariance)
        {
            AZ_Assert(splineGeometry.GetEntityId().IsValid(), "[EditorUtils::EraseVegetation()] Entity id is invalid");

            IEditor* editor = nullptr;
            AzToolsFramework::EditorRequests::Bus::BroadcastResult(editor, &AzToolsFramework::EditorRequests::GetEditor);

            CVegetationMap* vegetationMap = editor->GetVegetationMap();
            if (vegetationMap == nullptr)
            {
                return;
            }

            AZ::Transform transform = AZ::Transform::CreateIdentity();
            AZ::TransformBus::EventResult(transform, splineGeometry.GetEntityId(), &AZ::TransformBus::Events::GetWorldTM);
            AZ::Transform invertedTransform = transform.GetInverseFull();

            AZ::ConstSplinePtr spline = nullptr;
            LmbrCentral::SplineComponentRequestBus::EventResult(spline, splineGeometry.GetEntityId(), &LmbrCentral::SplineComponentRequests::GetSpline);

            std::default_random_engine generator;
            generator.seed(static_cast<AZ::u32>(time(0)));

            AzToolsFramework::UndoSystem::URSequencePoint* undoOperation = nullptr;
            using AzToolsFramework::ToolsApplicationRequests;
            ToolsApplicationRequests::Bus::BroadcastResult(undoOperation, &ToolsApplicationRequests::BeginUndoBatch, "Erase vegetation");

            float currentDistanceAlongTheSpline = 0.0f;
            bool modified = false;

            const auto itData = SplineGeometryMathUtils::CaluclateIterationData(spline.get(), splineGeometry.GetSegmentLength());
            for (size_t i = 0; i < itData.m_chunksCount; ++i)
            {
                float maxEraseDist = eraseVegetationWidth + eraseVegetationVariance;
                auto points = GetEdgePointsOfSplineGeometryChunk(splineGeometry, currentDistanceAlongTheSpline, itData.m_realStepSize, maxEraseDist);
                currentDistanceAlongTheSpline += itData.m_realStepSize;

                AZStd::vector<CVegetationInstance*> vegetationInstances;
                EditorVegetationRequestsBus::EventResult(vegetationInstances, vegetationMap, &EditorVegetationRequestsBus::Events::GetObjectInstances, points.m_min, points.m_max);

                for (const auto& vegetation : vegetationInstances)
                {
                    AZ::Vector3 vegPosition = invertedTransform * LYVec3ToAZVec3(vegetation->pos);
                    auto nearestPositionQuery = spline->GetNearestAddressRay(vegPosition, AZ::Vector3::CreateAxisZ());

                    const SplineGeometryWidthModifier& widthModifiers = splineGeometry.GetWidthModifiers();
                    float preciseRequiredDistance = widthModifiers.GetWidthAt(spline->GetLength(nearestPositionQuery.m_splineAddress)) * 0.5f;
                    preciseRequiredDistance += eraseVegetationWidth;
                    preciseRequiredDistance = max(preciseRequiredDistance, 0.0f);
                    
                    float randomVariance = 0.0f;
                    if (eraseVegetationVariance > 0.0f)
                    {
                        std::normal_distribution<> randomVegetationDistribution(preciseRequiredDistance, eraseVegetationVariance / AZ::Constants::Pi);
                        randomVariance = randomVegetationDistribution(generator);
                    }

                    AZ::Vector3 nearestSplinePosition = spline->GetPosition(nearestPositionQuery.m_splineAddress);
                    float distanceXySqr = AZ::Vector2(nearestSplinePosition.GetX(), nearestSplinePosition.GetY()).GetDistanceSq(AZ::Vector2(vegPosition.GetX(), vegPosition.GetY()));

                    if (distanceXySqr < sqr(randomVariance) || distanceXySqr < sqr(preciseRequiredDistance))
                    {
                        EditorVegetationRequestsBus::Event(vegetationMap, &EditorVegetationRequestsBus::Events::DeleteObjectInstance, vegetation);
                        modified = true;
                    }
                }
            }

            ToolsApplicationRequests::Bus::Broadcast(&ToolsApplicationRequests::EndUndoBatch);
        }

        void AlignHeightMap(const SplineGeometry& splineGeometry, AlignHeightmapParams params)
        {
            IEditor* editor = nullptr;
            AzToolsFramework::EditorRequests::Bus::BroadcastResult(editor, &AzToolsFramework::EditorRequests::GetEditor);

            AZ::ConstSplinePtr spline = nullptr;
            LmbrCentral::SplineComponentRequestBus::EventResult(spline, splineGeometry.GetEntityId(), &LmbrCentral::SplineComponentRequests::GetSpline);
            const SplineGeometryWidthModifier& widthModifiers = splineGeometry.GetWidthModifiers();

            AZ::Transform transform = AZ::Transform::CreateIdentity();
            AZ::TransformBus::EventResult(transform, splineGeometry.GetEntityId(), &AZ::TransformBus::Events::GetWorldTM);
            AZ::Transform localFromWorldTransform = transform.GetInverseFull();

            CHeightmap* heightmap = editor->GetHeightmap();
            int cellSize = heightmap->GetUnitSize();

            AZ::Aabb splineAabb;
            splineAabb.SetNull();
            spline->GetAabb(splineAabb, transform);

            float maxAlignDist = params.terrainBorderWidth + widthModifiers.GetMaximumWidth();
            splineAabb.Expand(AZ::Vector3(maxAlignDist, maxAlignDist, maxAlignDist));

            auto getHeightmapCoord = [cellSize](float worldCoord) { return static_cast<int>(worldCoord / cellSize); };

            // Heightmap is rotated compared to world coordinates.
            int minY = getHeightmapCoord(splineAabb.GetMin().GetX());
            int minX = getHeightmapCoord(splineAabb.GetMin().GetY());
            int maxY = getHeightmapCoord(splineAabb.GetMax().GetX()) + 1;
            int maxX = getHeightmapCoord(splineAabb.GetMax().GetY()) + 1;

            int width = maxX - minX;
            int height = maxY - minY;

            heightmap->RecordAzUndoBatchTerrainModify(minX, minY, width + 1, height + 1);

            for (int y = minY; y <= maxY && y < heightmap->GetHeight(); ++y)
            {
                for (int x = minX; x <= maxX && x < heightmap->GetWidth(); ++x)
                {
                    AZ::Vector3 worldCellPosition { y * cellSize, x * cellSize, 0.0f };
                    AZ::Vector3 localCellPosition = localFromWorldTransform * worldCellPosition;

                    auto nearestPositionAddress = spline->GetNearestAddressRay(localCellPosition, AZ::Vector3::CreateAxisZ()).m_splineAddress;

                    if (params.dontExceedSplineLength)
                    {
                        float clampedDistance = spline->GetLength(nearestPositionAddress);
                        float splineLength = spline->GetSplineLength();

                        clampedDistance = AZ::GetClamp(clampedDistance, widthModifiers.GetWidthAt(0.0f) * 0.5f, splineLength - widthModifiers.GetWidthAt(splineLength) * 0.5f);
                        nearestPositionAddress = spline->GetAddressByDistance(clampedDistance);
                    }

                    auto localNearestPoint = spline->GetPosition(nearestPositionAddress);
                    auto worldNearestPoint = transform * localNearestPoint;

                    float halfGeomWidth = widthModifiers.GetWidthAt(spline->GetLength(nearestPositionAddress)) * 0.5f + cellSize * 0.5f;
                    float adjustedWidth = halfGeomWidth + params.riverBedWidth;

                    float distanceProjection = AZ::Vector2(localNearestPoint.GetX(), localNearestPoint.GetY()).GetDistance(AZ::Vector2(localCellPosition.GetX(), localCellPosition.GetY()));
                    if (distanceProjection < adjustedWidth)
                    {
                        float k = distanceProjection / adjustedWidth;
                        k = AZ::GetClamp(k, 0.0f, 1.0f);
                        k = 1.0f - (cos(k * AZ::Constants::Pi) + 1.0f) / 2;

                        float riverSurface = worldNearestPoint.GetZ() + params.embankment;
                        float worldRiverBedDepth = static_cast<float>(worldNearestPoint.GetZ() - params.riverBedDepth);
                        float blendedDepth = k * riverSurface + (1.0f - k) * worldRiverBedDepth;

                        heightmap->SetXY(x, y, AZ::GetClamp(blendedDepth, 0.0f, heightmap->GetMaxHeight()));
                    }
                    else 
                    {
                        float k = (distanceProjection - adjustedWidth) / params.terrainBorderWidth;
                        k = AZ::GetClamp(k, 0.0f, 1.0f);

                        float riverSurface = static_cast<float>(worldNearestPoint.GetZ()) + params.embankment;

                        // mapping linear 0-1 range using cos-like easing function to form a nice terrain fall-off on the road sides
                        // k = 0: point is just at the road edge, k = 1: point is at 'borderWidth' distance from the road edge
                        k = 1.0f - (cos(k * AZ::Constants::Pi) + 1.0f) / 2;
                        float z = heightmap->GetXY(x, y);

                        // blending spline height with surrounding terrain height; 
                        // k = 0: match spline height, k = 1: match original terrain height
                        z = k * z + (1.0f - k) * riverSurface;

                        heightmap->SetXY(x, y, AZ::GetClamp(z, 0.0f, heightmap->GetMaxHeight()));
                    }
                }
            }

            heightmap->UpdateEngineTerrain(minX, minY, AZ::GetMax(width, height), 0.0f, true, false);
        }
    }
}
