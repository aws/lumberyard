
#pragma once

#include <AzFramework/Physics/SystemBus.h>

#include <IRenderer.h>
#include <IRenderAuxGeom.h>
#include <MathConversion.h>

namespace Physics
{
    AZ_INLINE void GetDefaultDebugDrawSettings(DebugDrawSettings& settings)
    {
        static AZStd::vector<Vec3> cryVerts;
        static AZStd::vector<ColorB> cryColors;

        auto minfn = [](float a, AZ::VectorFloat b) -> float
        {
            return a < b ? a : static_cast<float>(b);
        };

        auto drawLine = [&](const DebugDrawVertex& from, const DebugDrawVertex& to, const Ptr<WorldBody>& body, float thickness, void* udata) -> void
        {
            const ColorF cryColorFrom(from.m_color.GetR(), from.m_color.GetG(), from.m_color.GetB(), minfn(0.5f, from.m_color.GetA()));
            const ColorF cryColorTo(to.m_color.GetR(), to.m_color.GetG(), to.m_color.GetB(), minfn(0.5f, to.m_color.GetA()));
            IRenderAuxGeom* renderAuxGeom = reinterpret_cast<IRenderAuxGeom*>(udata);
            renderAuxGeom->DrawLine(
                AZVec3ToLYVec3(from.m_position), cryColorFrom,
                AZVec3ToLYVec3(to.m_position), cryColorTo,
                thickness);
        };

        auto drawTriangleBatch = [&](const DebugDrawVertex* verts, AZ::u32 numVerts, const AZ::u32* indices, AZ::u32 numIndices, const Ptr<WorldBody>& body, void* udata) -> void
        {
            cryVerts.reserve(numVerts);
            cryColors.reserve(numVerts);
            cryVerts.clear();
            cryColors.clear();
            IRenderAuxGeom* renderAuxGeom = reinterpret_cast<IRenderAuxGeom*>(udata);

            for (AZ::u32 i = 0; i < numVerts; ++i)
            {
                const DebugDrawVertex& v = verts[i];
                const ColorF cryColor(v.m_color.GetR(), v.m_color.GetG(), v.m_color.GetB(), minfn(0.5f, v.m_color.GetA()));
                cryVerts.emplace_back(AZVec3ToLYVec3(v.m_position));
                cryColors.emplace_back(cryColor);
            }

            renderAuxGeom->DrawTriangles(cryVerts.begin(), cryVerts.size(), indices, numIndices, cryColors.begin());
        };

        IRenderAuxGeom* renderAuxGeom = gEnv->pRenderer->GetIRenderAuxGeom();
        SAuxGeomRenderFlags renderFlags = renderAuxGeom->GetRenderFlags();
        SAuxGeomRenderFlags originalRenderFlags = renderAuxGeom->GetRenderFlags();
        renderFlags.SetMode2D3DFlag(e_Mode3D);
        renderFlags.SetCullMode(e_CullModeBack);
        renderFlags.SetAlphaBlendMode(e_AlphaBlended);
        renderAuxGeom->SetRenderFlags(renderFlags);

        settings.m_drawLineCB       = drawLine;
        settings.m_drawTriBatchCB   = drawTriangleBatch;
        settings.m_cameraPos        = LYVec3ToAZVec3(gEnv->pRenderer->GetCamera().GetPosition());
        settings.m_udata            = reinterpret_cast<void*>(renderAuxGeom);
    }

    AZ_INLINE void DebugDrawWithDefaultSettings()
    {
        DebugDrawSettings settings;
        GetDefaultDebugDrawSettings(settings);
        SystemDebugRequestBus::Broadcast(&SystemDebugRequestBus::Events::DebugDrawPhysics, settings);
    }
}
