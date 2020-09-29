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

#include <IRenderAuxGeom.h>
#include <Cry_Vector3.h>
#include <gmock/gmock.h>

struct SAuxGeomRenderFlags;

struct IRenderAuxGeomMock
    : public IRenderAuxGeom
{
    MOCK_METHOD1(SetRenderFlags,
        void(const SAuxGeomRenderFlags& renderFlags));
    MOCK_METHOD0(GetRenderFlags,
        SAuxGeomRenderFlags());
    MOCK_METHOD3(DrawPoint,
        void(const Vec3& v, const ColorB& col, uint8 size));
    MOCK_METHOD4(DrawPoints,
        void(const Vec3* v, uint32 numPoints, const ColorB& col, uint8 size));
    MOCK_METHOD4(DrawPoints,
        void(const Vec3* v, uint32 numPoints, const ColorB* col, uint8 size));
    MOCK_METHOD5(DrawLine,
        void(const Vec3& v0, const ColorB& colV0, const Vec3& v1, const ColorB& colV1, float thickness));
    MOCK_METHOD4(DrawLines,
        void(const Vec3* v, uint32 numPoints, const ColorB& col, float thickness));
    MOCK_METHOD4(DrawLines,
        void(const Vec3* v, uint32 numPoints, const ColorB* col, float thickness));
    MOCK_METHOD6(DrawLines,
        void(const Vec3* v, uint32 numPoints, const vtx_idx* ind, uint32 numIndices, const ColorB& col, float thickness));
    MOCK_METHOD6(DrawLines,
        void(const Vec3* v, uint32 numPoints, const vtx_idx* ind, uint32 numIndices, const ColorB* col, float thickness));
    MOCK_METHOD6(DrawTriangle,
        void(const Vec3& v0, const ColorB& colV0, const Vec3& v1, const ColorB& colV1, const Vec3& v2, const ColorB& colV2));
    MOCK_METHOD3(DrawTriangles,
        void(const Vec3* v, uint32 numPoints, const ColorB& col));
    MOCK_METHOD3(DrawTriangles,
        void(const Vec3* v, uint32 numPoints, const ColorB* col));
    MOCK_METHOD5(DrawTriangles,
        void(const Vec3* v, uint32 numPoints, const vtx_idx* ind, uint32 numIndices, const ColorB& col));
    MOCK_METHOD5(DrawTriangles,
        void(const Vec3* v, uint32 numPoints, const vtx_idx* ind, uint32 numIndices, const ColorB* col));
    MOCK_METHOD5(DrawPolyline,
        void(const Vec3* v, uint32 numPoints, bool closed, const ColorB& col, float thickness));
    MOCK_METHOD5(DrawPolyline,
        void(const Vec3* v, uint32 numPoints, bool closed, const ColorB* col, float thickness));\
    MOCK_METHOD5(DrawQuad,
        void(float width, float height, const Matrix34& matWorld, const ColorB& col, bool drawShaded));
    MOCK_METHOD4(DrawAABB,
        void(const AABB& aabb, bool bSolid, const ColorB& col, const EBoundingBoxDrawStyle& bbDrawStyle));
    MOCK_METHOD5(DrawAABBs,
        void(const AABB* aabbs, uint32 aabbCount, bool bSolid, const ColorB& col, const EBoundingBoxDrawStyle& bbDrawStyle));
    MOCK_METHOD5(DrawAABB,
        void(const AABB& aabb, const Matrix34& matWorld, bool bSolid, const ColorB& col, const EBoundingBoxDrawStyle& bbDrawStyle));
    MOCK_METHOD5(DrawOBB,
        void(const OBB& obb, const Vec3& pos, bool bSolid, const ColorB& col, const EBoundingBoxDrawStyle& bbDrawStyle));
    MOCK_METHOD5(DrawOBB,
        void(const OBB& obb, const Matrix34& matWorld, bool bSolid, const ColorB& col, const EBoundingBoxDrawStyle& bbDrawStyle));
    MOCK_METHOD4(DrawSphere,
        void(const Vec3& pos, float radius, const ColorB& col, bool drawShaded));
    MOCK_METHOD5(DrawDisk,
        void(const Vec3& pos, const Vec3& dir, float radius, const ColorB& col, bool drawShaded));
    MOCK_METHOD6(DrawCone,
        void(const Vec3& pos, const Vec3& dir, float radius, float height, const ColorB& col, bool drawShaded));
    MOCK_METHOD6(DrawCylinder,
        void(const Vec3& pos, const Vec3& dir, float radius, float height, const ColorB& col, bool drawShaded));
    MOCK_METHOD3(DrawBone,
        void(const Vec3& rParent, const Vec3& rBone, ColorB col));
    MOCK_METHOD4(RenderText,
        void(Vec3 pos, SDrawTextInfo& ti, const char* format, va_list args));
    MOCK_METHOD0(Flush,
        void());
    MOCK_METHOD1(Commit,
        void(uint frames));
    MOCK_METHOD0(Process,
        void());
    MOCK_METHOD1(SetAlphaBlendMode,
        void(const EAuxGeomPublicRenderflags_AlphaBlendMode& state));

};
