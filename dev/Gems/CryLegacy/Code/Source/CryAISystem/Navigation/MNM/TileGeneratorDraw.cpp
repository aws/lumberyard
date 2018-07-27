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

#include "CryLegacy_precompiled.h"
#include "TileGenerator.h"
#include "IAIDebugRenderer.h"

namespace MNM
{
    template<typename TyFilter>
    void DrawSpanGrid(const Vec3 volumeMin, const Vec3 voxelSize, const CompactSpanGrid& grid, const TyFilter& filter, const ColorB& color) PREFAST_SUPPRESS_WARNING(6262)
    {
        const size_t width = grid.GetWidth();
        const size_t height = grid.GetHeight();
        const size_t gridSize = width * height;

        IRenderAuxGeom* renderAuxGeom = gEnv->pRenderer->GetIRenderAuxGeom();
        size_t aabbCount = 0;

        const size_t MaxAABB = 1024;
        AABB aabbs[MaxAABB];

        SAuxGeomRenderFlags oldFlags = renderAuxGeom->GetRenderFlags();
        SAuxGeomRenderFlags renderFlags(oldFlags);

        renderFlags.SetAlphaBlendMode(e_AlphaBlended);
        renderFlags.SetDepthWriteFlag(e_DepthWriteOff);

        for (size_t y = 0; y < height; ++y)
        {
            const float minY = volumeMin.y + y * voxelSize.y;

            for (size_t x = 0; x < width; ++x)
            {
                const float minX = volumeMin.x + x * voxelSize.x;
                const float minZ = volumeMin.z;

                if (const CompactSpanGrid::Cell cell = grid.GetCell(x, y))
                {
                    size_t count = cell.count;

                    for (size_t s = 0; s < count; ++s)
                    {
                        const CompactSpanGrid::Span& span = grid.GetSpan(cell.index + s);

                        if (filter(grid, cell.index + s))
                        {
                            aabbs[aabbCount++] = AABB(Vec3(minX, minY, minZ + span.bottom * voxelSize.z),
                                    Vec3(minX + voxelSize.x, minY + voxelSize.y, minZ + (span.bottom + span.height) * voxelSize.z));

                            if (aabbCount == MaxAABB)
                            {
                                renderAuxGeom->DrawAABBs(aabbs, aabbCount, true, color, eBBD_Faceted);
                                aabbCount = 0;
                            }
                        }
                    }
                }
            }
        }

        if (aabbCount)
        {
            renderAuxGeom->DrawAABBs(aabbs, aabbCount, true, color, eBBD_Faceted);
            aabbCount = 0;
        }

        renderAuxGeom->SetRenderFlags(oldFlags);
    }

    template<typename TyFilter, typename TyColorPicker>
    void DrawSpanGrid(const Vec3 volumeMin, const Vec3 voxelSize, const CompactSpanGrid& grid, const TyFilter& filter,
        const TyColorPicker& color)
    {
        const size_t width = grid.GetWidth();
        const size_t height = grid.GetHeight();
        const size_t gridSize = width * height;

        IRenderAuxGeom* renderAuxGeom = gEnv->pRenderer->GetIRenderAuxGeom();

        SAuxGeomRenderFlags oldFlags = renderAuxGeom->GetRenderFlags();
        SAuxGeomRenderFlags renderFlags(oldFlags);

        renderFlags.SetAlphaBlendMode(e_AlphaBlended);
        renderFlags.SetDepthWriteFlag(e_DepthWriteOff);

        for (size_t y = 0; y < height; ++y)
        {
            const float minY = volumeMin.y + y * voxelSize.y;

            for (size_t x = 0; x < width; ++x)
            {
                const float minX = volumeMin.x + x * voxelSize.x;
                const float minZ = volumeMin.z;

                if (const CompactSpanGrid::Cell cell = grid.GetCell(x, y))
                {
                    size_t count = cell.count;

                    for (size_t s = 0; s < count; ++s)
                    {
                        const CompactSpanGrid::Span& span = grid.GetSpan(cell.index + s);

                        if (filter(grid, cell.index + s))
                        {
                            AABB aabb(Vec3(minX, minY, minZ + span.bottom * voxelSize.z),
                                Vec3(minX + voxelSize.x, minY + voxelSize.y, minZ + (span.bottom + span.height) * voxelSize.z));

                            renderAuxGeom->DrawAABB(aabb, true, color(grid, cell.index + s), eBBD_Faceted);
                        }
                    }
                }
            }
        }

        renderAuxGeom->SetRenderFlags(oldFlags);
    }


    template<typename TyFilter, typename TyColorPicker>
    void DrawSpanGridTop(const Vec3 volumeMin, const Vec3 voxelSize, const CompactSpanGrid& grid, const TyFilter& filter,
        const TyColorPicker& color)
    {
        const size_t width = grid.GetWidth();
        const size_t height = grid.GetHeight();
        const size_t gridSize = width * height;

        IRenderAuxGeom* renderAuxGeom = gEnv->pRenderer->GetIRenderAuxGeom();

        SAuxGeomRenderFlags oldFlags = renderAuxGeom->GetRenderFlags();
        SAuxGeomRenderFlags renderFlags(oldFlags);

        renderFlags.SetAlphaBlendMode(e_AlphaBlended);
        renderFlags.SetDepthWriteFlag(e_DepthWriteOff);

        for (size_t y = 0; y < height; ++y)
        {
            const float minY = volumeMin.y + y * voxelSize.y;

            for (size_t x = 0; x < width; ++x)
            {
                const float minX = volumeMin.x + x * voxelSize.x;
                const float minZ = volumeMin.z;

                if (const CompactSpanGrid::Cell cell = grid.GetCell(x, y))
                {
                    size_t count = cell.count;

                    for (size_t s = 0; s < count; ++s)
                    {
                        const CompactSpanGrid::Span& span = grid.GetSpan(cell.index + s);

                        if (filter(grid, cell.index + s))
                        {
                            ColorB vcolor = color(grid, cell.index + s);

                            Vec3 v0 = Vec3(minX, minY, minZ + (span.bottom + span.height) * voxelSize.z);
                            Vec3 v1 = Vec3(v0.x, v0.y + voxelSize.y, v0.z);
                            Vec3 v2 = Vec3(v0.x + voxelSize.x, v0.y + voxelSize.y, v0.z);
                            Vec3 v3 = Vec3(v0.x + voxelSize.x, v0.y, v0.z);

                            renderAuxGeom->DrawTriangle(v0, vcolor, v2, vcolor, v1, vcolor);
                            renderAuxGeom->DrawTriangle(v0, vcolor, v3, vcolor, v2, vcolor);
                        }
                    }
                }
            }
        }

        renderAuxGeom->SetRenderFlags(oldFlags);
    }

    ColorB ColorFromNumber(size_t n)
    {
        return ColorB(64 + (n * 64) % 192, 64 + ((n * 32) + 64) % 192, 64 + (n * 24) % 192, 255);
    }

    struct AllPassFilter
    {
        bool operator()(const CompactSpanGrid& grid, size_t i) const
        {
            return true;
        }
    };

    struct NotWalkableFilter
    {
        bool operator()(const CompactSpanGrid& grid, size_t i) const
        {
            const CompactSpanGrid::Span& span = grid.GetSpan(i);
            return (span.flags & TileGenerator::NotWalkable) != 0;
        }
    };

    struct WalkableFilter
    {
        bool operator()(const CompactSpanGrid& grid, size_t i) const
        {
            const CompactSpanGrid::Span& span = grid.GetSpan(i);
            return (span.flags & TileGenerator::NotWalkable) == 0;
        }
    };

    struct BoundaryFilter
    {
        bool operator()(const CompactSpanGrid& grid, size_t i) const
        {
            const CompactSpanGrid::Span& span = grid.GetSpan(i);
            return (span.flags & TileGenerator::TileBoundary) != 0;
        }
    };

    struct RawColorPicker
    {
        RawColorPicker(const ColorB& _normal, const ColorB& _backface)
            : normal(_normal)
            , backface(_backface)
        {
        }

        ColorB operator()(const CompactSpanGrid& grid, size_t i) const
        {
            const CompactSpanGrid::Span& span = grid.GetSpan(i);
            if (!span.backface)
            {
                return normal;
            }
            return backface;
        }
    private:
        ColorB normal;
        ColorB backface;
    };


    struct DistanceColorPicker
    {
        DistanceColorPicker(const uint16* _distances)
            : distances(_distances)
        {
        }

        ColorB operator()(const CompactSpanGrid& grid, size_t i) const
        {
            uint8 distance = (uint8)min<uint16>(distances[i] * 4, 255);
            return ColorB(distance, distance, distance, 255);
        }
    private:
        const uint16* distances;
    };

    struct LabelColor
    {
        LabelColor()
        {
        }

        ColorB operator()(size_t r) const
        {
            if (r < TileGenerator::NoLabel)
            {
                return ColorFromNumber(r);
            }
            else if (r == TileGenerator::NoLabel)
            {
                return Col_DarkGray;
            }
            else if ((r& TileGenerator::BorderLabelV) == TileGenerator::BorderLabelV)
            {
                return Col_DarkSlateBlue;
            }
            else if ((r& TileGenerator::BorderLabelH) == TileGenerator::BorderLabelH)
            {
                return Col_DarkOliveGreen;
            }
            else if (r & TileGenerator::ExternalContour)
            {
                return Col_DarkSlateBlue * 0.5f + Col_DarkOliveGreen * 0.5f;
            }
            else if (r & TileGenerator::InternalContour)
            {
                return Col_DarkSlateBlue * 0.25f + Col_DarkOliveGreen * 0.75f;
            }
            else
            {
                return Col_DarkSlateBlue * 0.5f + Col_DarkOliveGreen * 0.5f;
            }

            return Col_VioletRed;
        }
    };

    struct LabelColorPicker
    {
        LabelColorPicker(const uint16* _labels)
            : labels(_labels)
        {
        }

        ColorB operator()(const CompactSpanGrid& grid, size_t i) const
        {
            return LabelColor()(labels[i]);
        }
    private:
        const uint16* labels;
    };


    void TileGenerator::Draw(DrawMode mode) const
    {
        if (!m_spanGrid.GetSpanCount())
        {
            return;
        }

        const size_t border = BorderSizeH();
        const size_t borderV = BorderSizeV();
        const Vec3 origin = Vec3(
                m_params.origin.x - m_params.voxelSize.x * border,
                m_params.origin.y - m_params.voxelSize.y * border,
                m_params.origin.z - m_params.voxelSize.z * borderV);

        const float blinking = 0.25f + 0.75f * fabs_tpl(sin_tpl(gEnv->pTimer->GetCurrTime() * gf_PI));
        const ColorB red = (Col_Red * blinking) + (Col_VioletRed * (1.0f - blinking));
        gAIEnv.GetDebugRenderer()->DrawAABB(AABB(m_params.origin, m_params.origin + Vec3(m_params.sizeX, m_params.sizeY, m_params.sizeZ)), false, Col_Red, eBBD_Faceted);
        switch (mode)
        {
        case DrawNone:
        default:
            break;
        case DrawRawVoxels:
            if (m_spanGrid.GetSpanCount())
            {
                MNM::DrawSpanGrid(origin, m_params.voxelSize, m_spanGrid
                    , AllPassFilter(), RawColorPicker(Col_LightGray, Col_DarkGray));
            }
            break;
        case DrawFlaggedVoxels:
            if (m_spanGridFlagged.GetSpanCount())
            {
                MNM::DrawSpanGrid(origin, m_params.voxelSize, m_spanGridFlagged, NotWalkableFilter(), ColorB(Col_VioletRed));
                MNM::DrawSpanGrid(origin, m_params.voxelSize, m_spanGridFlagged, WalkableFilter(), ColorB(Col_SlateBlue));
                MNM::DrawSpanGrid(origin, m_params.voxelSize, m_spanGridFlagged, BoundaryFilter(), ColorB(Col_NavyBlue, 0.5f));
            }
            break;
        case DrawDistanceTransform:
            if (!m_distances.empty())
            {
                DrawSpanGridTop(origin, m_params.voxelSize, m_spanGrid, AllPassFilter(), DistanceColorPicker(&m_distances.front()));
            }
            break;
        case DrawSegmentation:
            if (!m_labels.empty())
            {
                DrawSpanGridTop(origin, m_params.voxelSize, m_spanGrid, WalkableFilter(), LabelColorPicker(&m_labels.front()));
            }

            for (size_t i = 0; i < m_regions.size(); ++i)
            {
                const Region& region = m_regions[i];

                if (region.contour.empty())
                {
                    continue;
                }

                size_t xx = 0;
                size_t yy = 0;
                size_t zz = 0;

                for (size_t c = 0; c < region.contour.size(); ++c)
                {
                    const ContourVertex& v = region.contour[c];
                    xx += v.x;
                    yy += v.y;
                    zz = std::max<size_t>(zz, v.z);
                }

                Vec3 idLocation(xx / (float)region.contour.size(), yy / (float)region.contour.size(), (float)zz);
                idLocation = origin + idLocation.CompMul(m_params.voxelSize);

                float x, y, z;
                if (gEnv->pRenderer->ProjectToScreen(idLocation.x, idLocation.y, idLocation.z, &x, &y, &z))
                {
                    if ((z >= 0.0f) && (z <= 1.0f))
                    {
                        x *= (float)gEnv->pRenderer->GetWidth() * 0.01f;
                        y *= (float)gEnv->pRenderer->GetHeight() * 0.01f;

                        const ColorB vcolor = LabelColor()(i);
                        ColorF textColor(vcolor.pack_abgr8888());

                        gEnv->pRenderer->Draw2dLabel(x, y, 1.8f, (float*)&textColor, true, "%" PRISIZE_T "", i);
                        gEnv->pRenderer->Draw2dLabel(x, y + 1.6f * 10.0f, 1.1f, (float*)&textColor, true,
                            "spans: %" PRISIZE_T " holes: %" PRISIZE_T "", region.spanCount, region.holes.size());
                    }
                }
            }
            break;
        case DrawNumberedContourVertices:
        case DrawContourVertices:
        {
            if (!m_labels.empty() && !m_regions.empty())
            {
                IRenderAuxGeom* renderAuxGeom = gEnv->pRenderer->GetIRenderAuxGeom();
                LabelColorPicker color(&m_labels.front());

                for (size_t i = 0; i < m_regions.size(); ++i)
                {
                    const Region& region = m_regions[i];

                    if (region.contour.empty())
                    {
                        continue;
                    }

                    const ColorB vcolor = LabelColor()(i);
                    const ContourVertex& v = region.contour.front();

                    Vec3 v0(origin.x + v.x * m_params.voxelSize.x,
                        origin.y + v.y * m_params.voxelSize.y,
                        origin.z + v.z * m_params.voxelSize.z);

                    for (size_t s = 1; s <= region.contour.size(); ++s)
                    {
                        const ContourVertex& nv = region.contour[s % region.contour.size()];
                        const ColorB pcolor = (nv.flags & ContourVertex::Unremovable) ? red : vcolor;

                        Vec3 v1(origin.x + nv.x * m_params.voxelSize.x,
                            origin.y + nv.y * m_params.voxelSize.y,
                            origin.z + nv.z * m_params.voxelSize.z);

                        renderAuxGeom->DrawLine(v0, vcolor, v1, vcolor, 7.0f);

                        if (nv.flags & ContourVertex::TileBoundary)
                        {
                            renderAuxGeom->DrawCone(v1, Vec3(0.0f, 0.0f, 1.0f), 0.03f, 0.085f, pcolor);

                            if (nv.flags & ContourVertex::TileBoundaryV)
                            {
                                renderAuxGeom->DrawAABB(AABB(v1, 0.03f), IDENTITY, true, pcolor, eBBD_Faceted);
                            }
                        }
                        else if (nv.flags & ContourVertex::TileBoundaryV)
                        {
                            renderAuxGeom->DrawAABB(AABB(v1, 0.03f), IDENTITY, true, pcolor, eBBD_Faceted);
                        }
                        else
                        {
                            renderAuxGeom->DrawSphere(v1, 0.025f, pcolor);
                        }

                        v0 = v1;
                    }

                    for (size_t h = 0; h < region.holes.size(); ++h)
                    {
                        const Contour& hole = region.holes[h];
                        const ContourVertex& hv = hole.front();

                        v0(origin.x + hv.x * m_params.voxelSize.x,
                            origin.y + hv.y * m_params.voxelSize.y,
                            origin.z + hv.z * m_params.voxelSize.z);

                        for (size_t s = 1; s <= hole.size(); ++s)
                        {
                            const ContourVertex& nv = hole[s % hole.size()];
                            const ColorB pcolor = (nv.flags & ContourVertex::Unremovable) ? red : vcolor;

                            Vec3 v1(origin.x + nv.x * m_params.voxelSize.x,
                                origin.y + nv.y * m_params.voxelSize.y,
                                origin.z + nv.z * m_params.voxelSize.z);

                            renderAuxGeom->DrawLine(v0, vcolor, v1, vcolor, 7.0f);

                            if (nv.flags & ContourVertex::TileBoundary)
                            {
                                renderAuxGeom->DrawCone(v1, Vec3(0.0f, 0.0f, 1.0f), 0.03f, 0.085f, pcolor);

                                if (nv.flags & ContourVertex::TileBoundaryV)
                                {
                                    renderAuxGeom->DrawAABB(AABB(v1, 0.03f), IDENTITY, true, pcolor, eBBD_Faceted);
                                }
                            }
                            else if (nv.flags & ContourVertex::TileBoundaryV)
                            {
                                renderAuxGeom->DrawAABB(AABB(v1, 0.03f), IDENTITY, true, pcolor, eBBD_Faceted);
                            }
                            else
                            {
                                renderAuxGeom->DrawCylinder(v1, Vec3(0.0f, 0.0f, 1.0f), 0.025f, 0.04f, pcolor);
                            }

                            v0 = v1;
                        }
                    }

                    if (mode == DrawNumberedContourVertices)
                    {
                        for (size_t s = 0; s < region.contour.size(); ++s)
                        {
                            const ContourVertex& cv = region.contour[s];

                            const Vec3 cv0(origin.x + cv.x * m_params.voxelSize.x,
                                origin.y + cv.y * m_params.voxelSize.y,
                                origin.z + cv.z * m_params.voxelSize.z);

                            float x, y, z;
                            if (gEnv->pRenderer->ProjectToScreen(cv0.x, cv0.y, cv0.z, &x, &y, &z))
                            {
                                if ((z >= 0.0f) && (z <= 1.0f))
                                {
                                    x *= (float)gEnv->pRenderer->GetWidth() * 0.01f;
                                    y *= (float)gEnv->pRenderer->GetHeight() * 0.01f;

                                    ColorF textColor(Col_White);

                                    gEnv->pRenderer->Draw2dLabel(x, y, 1.5f, (float*)&textColor, true, "%" PRISIZE_T "", s);
                                }
                            }
                        }

                        for (size_t h = 0; h < region.holes.size(); ++h)
                        {
                            for (size_t s = 0; s < region.holes[h].size(); ++s)
                            {
                                const ContourVertex& cv = region.holes[h][s];

                                const Vec3 cv0(origin.x + cv.x * m_params.voxelSize.x,
                                    origin.y + cv.y * m_params.voxelSize.y,
                                    origin.z + cv.z * m_params.voxelSize.z);

                                float x, y, z;
                                if (gEnv->pRenderer->ProjectToScreen(cv0.x, cv0.y, cv0.z, &x, &y, &z))
                                {
                                    if ((z >= 0.0f) && (z <= 1.0f))
                                    {
                                        x *= (float)gEnv->pRenderer->GetWidth() * 0.01f;
                                        y *= (float)gEnv->pRenderer->GetHeight() * 0.01f;

                                        ColorF textColor(Col_White);

                                        gEnv->pRenderer->Draw2dLabel(x, y, 1.5f, (float*)&textColor, true, "%" PRISIZE_T "", s);
                                    }
                                }
                            }
                        }
                    }

                    size_t xx = 0;
                    size_t yy = 0;
                    size_t zz = 0;

                    for (size_t c = 0; c < region.contour.size(); ++c)
                    {
                        const ContourVertex& cv = region.contour[c];
                        xx += cv.x;
                        yy += cv.y;
                        zz = std::max<size_t>(zz, cv.z);
                    }

                    Vec3 idLocation(xx / (float)region.contour.size(), yy / (float)region.contour.size(), (float)zz);
                    idLocation = origin + idLocation.CompMul(m_params.voxelSize);

                    float x, y, z;
                    if (gEnv->pRenderer->ProjectToScreen(idLocation.x, idLocation.y, idLocation.z, &x, &y, &z))
                    {
                        if ((z >= 0.0f) && (z <= 1.0f))
                        {
                            x *= (float)gEnv->pRenderer->GetWidth() * 0.01f;
                            y *= (float)gEnv->pRenderer->GetHeight() * 0.01f;

                            ColorF textColor(vcolor.pack_abgr8888());

                            gEnv->pRenderer->Draw2dLabel(x, y, 1.8f, (float*)&textColor, true, "%" PRISIZE_T "", i);
                            gEnv->pRenderer->Draw2dLabel(x, y + 1.6f * 10.0f, 1.1f, (float*)&textColor, true,
                                "spans: %" PRISIZE_T " holes: %" PRISIZE_T "", region.spanCount, region.holes.size());
                        }
                    }
                }
            }
        }
        break;

        case DrawTracers:
        {
            if (IRenderAuxGeom* pRender = gEnv->pRenderer->GetIRenderAuxGeom())
            {
                const SAuxGeomRenderFlags old = pRender->GetRenderFlags();
                SAuxGeomRenderFlags flags;
                flags.SetAlphaBlendMode(e_AlphaNone);
                flags.SetDepthWriteFlag(e_DepthWriteOff);
                pRender->SetRenderFlags(flags);
                {
                    const Vec3 corners[4] =
                    {
                        Vec3(-m_params.voxelSize.x, +m_params.voxelSize.y, 0.f) * 0.5f,
                        Vec3(+m_params.voxelSize.x, +m_params.voxelSize.y, 0.f) * 0.5f,
                        Vec3(+m_params.voxelSize.x, -m_params.voxelSize.y, 0.f) * 0.5f,
                        Vec3(-m_params.voxelSize.x, -m_params.voxelSize.y, 0.f) * 0.5f
                    };
                    const ColorF cols[4] =
                    {
                        Col_Red, Col_Yellow, Col_Green, Col_Blue
                    };
                    const int numPaths = m_tracerPaths.size();
                    for (int i = 0; i < numPaths; i++)
                    {
                        const TracerPath& path = m_tracerPaths[i];
                        const int numTracers = path.steps.size();
                        for (int j = 0; j < numTracers; j++)
                        {
                            const Tracer& tracer = path.steps[j];
                            const Vec3 mid(origin + (m_params.voxelSize.CompMul(tracer.pos)) + Vec3(0.f, 0.f, 0.05f * i));
                            pRender->DrawTriangle(mid, Col_White, mid + corners[tracer.dir], cols[tracer.dir], mid + corners[(tracer.dir + 3) & 3], cols[tracer.dir]);
                        }
                    }
                }
                pRender->SetRenderFlags(old);
            }
        }
        break;

        case DrawSimplifiedContours:
        {
            IRenderAuxGeom* renderAuxGeom = gEnv->pRenderer->GetIRenderAuxGeom();

            for (size_t i = 0; i < m_polygons.size(); ++i)
            {
                const PolygonContour& contour = m_polygons[i].contour;

                if (contour.empty())
                {
                    continue;
                }

                const ColorB vcolor = LabelColor()(i);

                const PolygonVertex& pv = contour[contour.size() - 1];

                Vec3 v0(origin.x + pv.x * m_params.voxelSize.x,
                    origin.y + pv.y * m_params.voxelSize.y,
                    origin.z + pv.z * m_params.voxelSize.z);

                for (size_t s = 0; s < contour.size(); ++s)
                {
                    const PolygonVertex& nv = contour[s];
                    const Vec3 v1(origin.x + nv.x * m_params.voxelSize.x, origin.y + nv.y * m_params.voxelSize.y, origin.z + nv.z * m_params.voxelSize.z);

                    renderAuxGeom->DrawLine(v0, vcolor, v1, vcolor, 8.0f);
                    renderAuxGeom->DrawSphere(v1, 0.025f, vcolor);
                    gEnv->pRenderer->DrawLabel(v1, 1.8f, "%d", (int)s);

                    v0 = v1;
                }

                {
                    for (size_t h = 0; h < m_polygons[i].holes.size(); ++h)
                    {
                        const PolygonContour& hole = m_polygons[i].holes[h].verts;
                        const PolygonVertex& hv = hole.front();

                        float holeHeightRaise = 0.05f;

                        v0(origin.x + hv.x * m_params.voxelSize.x,
                            origin.y + hv.y * m_params.voxelSize.y,
                            (origin.z + hv.z * m_params.voxelSize.z) + holeHeightRaise);

                        for (size_t s = 1; s <= hole.size(); ++s)
                        {
                            const PolygonVertex& nv = hole[s % hole.size()];
                            const ColorB pcolor = vcolor;

                            Vec3 v1(origin.x + nv.x * m_params.voxelSize.x,
                                origin.y + nv.y * m_params.voxelSize.y,
                                (origin.z + nv.z * m_params.voxelSize.z) + holeHeightRaise);

                            renderAuxGeom->DrawLine(v0, vcolor, v1, vcolor, 7.0f);
                            renderAuxGeom->DrawCylinder(v1, Vec3(0.0f, 0.0f, 1.0f), 0.025f, 0.04f, pcolor);

                            v0 = v1;
                        }
                    }
                }
            }
        }
        break;
        case DrawTriangulation:
        {
            IRenderAuxGeom* renderAuxGeom = gEnv->pRenderer->GetIRenderAuxGeom();
            const size_t triCount = m_triangles.size();

            const ColorB vcolor(Col_SlateBlue, 0.65f);
            const ColorB lcolor(Col_White);
            const ColorB bcolor(Col_Black);

            const Vec3 offset(0.0f, 0.0f, 0.05f);
            const Vec3 loffset(0.0f, 0.0f, 0.0005f);

            SAuxGeomRenderFlags oldFlags = renderAuxGeom->GetRenderFlags();
            SAuxGeomRenderFlags renderFlags(oldFlags);

            renderFlags.SetAlphaBlendMode(e_AlphaBlended);
            renderFlags.SetDepthWriteFlag(e_DepthWriteOff);

            renderAuxGeom->SetRenderFlags(renderFlags);

            for (size_t i = 0; i < triCount; ++i)
            {
                const Tile::Triangle& triangle = m_triangles[i];
                const Vec3 v0 = m_vertices[triangle.vertex[0]].GetVec3() + m_params.origin + offset;
                const Vec3 v1 = m_vertices[triangle.vertex[1]].GetVec3() + m_params.origin + offset;
                const Vec3 v2 = m_vertices[triangle.vertex[2]].GetVec3() + m_params.origin + offset;

                renderAuxGeom->DrawTriangle(v0, vcolor, v1, vcolor, v2, vcolor);
            }

            renderAuxGeom->SetRenderFlags(oldFlags);

            {
                for (size_t i = 0; i < triCount; ++i)
                {
                    const Tile::Triangle& triangle = m_triangles[i];

                    const Vec3 v0 = m_vertices[triangle.vertex[0]].GetVec3() + m_params.origin + offset;
                    const Vec3 v1 = m_vertices[triangle.vertex[1]].GetVec3() + m_params.origin + offset;
                    const Vec3 v2 = m_vertices[triangle.vertex[2]].GetVec3() + m_params.origin + offset;

                    renderAuxGeom->DrawLine(v0 + loffset, lcolor, v1 + loffset, lcolor);
                    renderAuxGeom->DrawLine(v1 + loffset, lcolor, v2 + loffset, lcolor);
                    renderAuxGeom->DrawLine(v2 + loffset, lcolor, v0 + loffset, lcolor);
                }
            }
        }
        break;
        case DrawBVTree:
        {
            Draw(DrawTriangulation);

            IRenderAuxGeom* renderAuxGeom = gEnv->pRenderer->GetIRenderAuxGeom();

            for (size_t i = 0; i < m_bvtree.size(); ++i)
            {
                const Tile::BVNode& node = m_bvtree[i];

                AABB nodeAABB;

                nodeAABB.min = m_params.origin + node.aabb.min.GetVec3();
                nodeAABB.max = m_params.origin + node.aabb.max.GetVec3();

                renderAuxGeom->DrawAABB(nodeAABB, IDENTITY, false, Col_White, eBBD_Faceted);
            }
        }
        break;
        }
    }
}
