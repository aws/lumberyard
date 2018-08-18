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

#include <AzCore/base.h>
#include <AzCore/std/string/string.h>
#include <AzCore/EBus/EBus.h>
#include <AzCore/Math/Aabb.h>
#include <AzCore/Math/Vector2.h>
#include <AzCore/Math/Vector3.h>
#include <AzCore/Math/Vector4.h>
#include <AzCore/Math/Color.h>
#include <AzCore/Math/Transform.h>
#include <AzCore/Component/ComponentBus.h>

namespace AZ
{
    class Entity;
}

struct DisplayContext;
class ITexture;

namespace AzFramework
{
    /**
     */
    class EntityDebugDisplayRequests
        : public AZ::EBusTraits
    {
    public:

        //////////////////////////////////////////////////////////////////////////
        // EBusTraits overrides
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        //////////////////////////////////////////////////////////////////////////

        virtual void SetColor(float r, float g, float b, float a = 1.f) { (void)r; (void)g; (void)b; (void)a; }
        virtual void SetColor(const AZ::Color& color) { (void)color; }
        virtual void SetColor(const AZ::Vector4& color) { (void)color; }
        virtual void SetAlpha(float a) { (void)a; }
        virtual void DrawQuad(const AZ::Vector3& p1, const AZ::Vector3& p2, const AZ::Vector3& p3, const AZ::Vector3& p4) { (void)p1; (void)p2; (void)p3; (void)p4; }
        virtual void DrawQuadGradient(const AZ::Vector3& p1, const AZ::Vector3& p2, const AZ::Vector3& p3, const AZ::Vector3& p4, const AZ::Vector4& firstColor, const AZ::Vector4& secondColor) { (void)p1; (void)p2; (void)p3; (void)p4; (void)firstColor; (void)secondColor; }
        virtual void DrawTri(const AZ::Vector3& p1, const AZ::Vector3& p2, const AZ::Vector3& p3) { (void)p1; (void)p2; (void)p3; }
        virtual void DrawTriangles(const AZStd::vector<AZ::Vector3>& vertices, const AZ::Color& color) { (void)vertices; (void)color; }
        virtual void DrawTrianglesIndexed(const AZStd::vector<AZ::Vector3>& vertices, const AZStd::vector<AZ::u32>& indices, const AZ::Color& color) { (void)vertices; (void)indices, (void)color; }
        virtual void DrawWireBox(const AZ::Vector3& min, const AZ::Vector3& max) { (void)min; (void)max; }
        virtual void DrawSolidBox(const AZ::Vector3& min, const AZ::Vector3& max) { (void)min; (void)max; }
        virtual void DrawSolidOBB(const AZ::Vector3& center, const AZ::Vector3& axisX, const AZ::Vector3& axisY, const AZ::Vector3& axisZ, const AZ::Vector3& halfExtents) { (void)center; (void)axisX; (void)axisY; (void)axisZ; (void)halfExtents; }
        virtual void DrawPoint(const AZ::Vector3& p, int nSize = 1) { (void)p; (void)nSize; }
        virtual void DrawLine(const AZ::Vector3& p1, const AZ::Vector3& p2) { (void)p1; (void)p2; }
        virtual void DrawLine(const AZ::Vector3& p1, const AZ::Vector3& p2, const AZ::Vector4& col1, const AZ::Vector4& col2) { (void)p1; (void)p2; (void)col1; (void)col2; }
        virtual void DrawLines(const AZStd::vector<AZ::Vector3>& lines, const AZ::Color& color) { (void)lines; (void)color; }
        virtual void DrawPolyLine(const AZ::Vector3* pnts, int numPoints, bool cycled = true) { (void)pnts; (void)numPoints; (void)cycled; }
        virtual void DrawWireQuad2d(const AZ::Vector2& p1, const AZ::Vector2& p2, float z) { (void)p1; (void)p2; (void)z; }
        virtual void DrawLine2d(const AZ::Vector2& p1, const AZ::Vector2& p2, float z) { (void)p1; (void)p2; (void)z; }
        virtual void DrawLine2dGradient(const AZ::Vector2& p1, const AZ::Vector2& p2, float z, const AZ::Vector4& firstColor, const AZ::Vector4& secondColor) { (void)p1; (void)p2; (void)z; (void)firstColor; (void)secondColor; }
        virtual void DrawWireCircle2d(const AZ::Vector2& center, float radius, float z) { (void)center; (void)radius; (void)z; }
        virtual void DrawTerrainCircle(const AZ::Vector3& worldPos, float radius, float height) { (void)worldPos; (void)radius; (void)height; }
        virtual void DrawTerrainCircle(const AZ::Vector3& center, float radius, float angle1, float angle2, float height) { (void)center; (void)radius; (void)angle1; (void)angle2; (void)height; }
        virtual void DrawArc(const AZ::Vector3& pos, float radius, float startAngleDegrees, float sweepAngleDegrees, float angularStepDegrees, int referenceAxis = 2) { (void)pos; (void)radius; (void)startAngleDegrees; (void)sweepAngleDegrees; (void)angularStepDegrees; (void)referenceAxis; }
        virtual void DrawArc(const AZ::Vector3& pos, float radius, float startAngleDegrees, float sweepAngleDegrees, float angularStepDegrees, const AZ::Vector3& fixedAxis) { (void)pos; (void)radius; (void)startAngleDegrees; (void)sweepAngleDegrees; (void)angularStepDegrees; (void)fixedAxis; }
        virtual void DrawCircle(const AZ::Vector3& pos, float radius, int nUnchangedAxis = 2 /*z axis*/) { (void)pos; (void)radius; (void)nUnchangedAxis; }
        virtual void DrawHalfDottedCircle(const AZ::Vector3& pos, float radius, const AZ::Vector3& viewPos, int nUnchangedAxis = 2 /*z axis*/) { (void)pos; (void)radius; (void)viewPos; (void)nUnchangedAxis; }
        virtual void DrawCone(const AZ::Vector3& pos, const AZ::Vector3& dir, float radius, float height) { (void)pos; (void)dir; (void)radius; (void)height; }
        virtual void DrawWireCylinder(const AZ::Vector3& center, const AZ::Vector3& axis, float radius, float height) { (void)center; (void)axis; (void)radius; (void)height; }
        virtual void DrawSolidCylinder(const AZ::Vector3& center, const AZ::Vector3& axis, float radius, float height) { (void)center; (void)axis; (void)radius; (void)height; }
        virtual void DrawWireCapsule(const AZ::Vector3& center, const AZ::Vector3& axis, float radius, float heightStraightSection) { (void)center; (void)axis; (void)radius; (void)heightStraightSection; }
        virtual void DrawTerrainRect(float x1, float y1, float x2, float y2, float height) { (void)x1; (void)y1; (void)x2; (void)y2; (void)height; }
        virtual void DrawTerrainLine(AZ::Vector3 worldPos1, AZ::Vector3 worldPos2) { (void)worldPos1; (void)worldPos2; }
        virtual void DrawWireSphere(const AZ::Vector3& pos, float radius) { (void)pos; (void)radius; }
        virtual void DrawWireSphere(const AZ::Vector3& pos, const AZ::Vector3 radius) { (void)pos; (void)radius; }
        virtual void DrawBall(const AZ::Vector3& pos, float radius) { (void)pos; (void)radius; }
        virtual void DrawArrow(const AZ::Vector3& src, const AZ::Vector3& trg, float fHeadScale = 1, bool b2SidedArrow = false) { (void)src; (void)trg; (void)fHeadScale; (void)b2SidedArrow; }
        virtual void DrawTextLabel(const AZ::Vector3& pos, float size, const char* text, const bool bCenter = false, int srcOffsetX = 0, int srcOffsetY = 0) { (void)pos; (void)size; (void)text; (void)bCenter; (void)srcOffsetX; (void)srcOffsetY; }
        virtual void Draw2dTextLabel(float x, float y, float size, const char* text, bool bCenter = false) { (void)x; (void)y; (void)size; (void)text; (void)bCenter; }
        virtual void DrawTextOn2DBox(const AZ::Vector3& pos, const char* text, float textScale, const AZ::Vector4& TextColor, const AZ::Vector4& TextBackColor) { (void)pos; (void)text; (void)textScale; (void)TextColor; (void)TextBackColor; }
        virtual void DrawTextureLabel(ITexture* texture, const AZ::Vector3& pos, float sizeX, float sizeY, int texIconFlags) { (void)texture; (void)pos; (void)sizeX; (void)sizeY; (void)texIconFlags; }
        virtual void SetLineWidth(float width) { (void)width; }
        virtual bool IsVisible(const AZ::Aabb& bounds) { (void)bounds; return false; }
        virtual int SetFillMode(int nFillMode) { (void)nFillMode; return 0; }
        virtual float GetLineWidth() { return 0; }
        virtual float GetAspectRatio() { return 0.f; }
        virtual void DepthTestOff() { }
        virtual void DepthTestOn() { }
        virtual void DepthWriteOff() { }
        virtual void DepthWriteOn() { }
        virtual void CullOff() { }
        virtual void CullOn() { }
        virtual bool SetDrawInFrontMode(bool bOn) { (void)bOn; return false; }
        virtual AZ::u32 GetState() { return 0; }
        virtual AZ::u32 SetState(AZ::u32 state) { (void)state; return 0; }
        virtual AZ::u32 SetStateFlag(AZ::u32 state) { (void)state; return 0; }
        virtual AZ::u32 ClearStateFlag(AZ::u32 state) { (void)state; return 0; }
        virtual void PushMatrix(const AZ::Transform& tm) { (void)tm; }
        virtual void PopMatrix() {};
        virtual void SetDC(DisplayContext* dc) { (void)dc; }
    };

    using EntityDebugDisplayRequestBus = AZ::EBus<EntityDebugDisplayRequests>;

    class EntityDebugDisplayEvents
        : public AZ::ComponentBus
    {
    public:

        using Bus = AZ::EBus<EntityDebugDisplayEvents>;

        virtual void DisplayEntity(bool& handled) { (void)handled; }
    };

    using EntityDebugDisplayEventBus = AZ::EBus<EntityDebugDisplayEvents>;
} // namespace AzFramework
