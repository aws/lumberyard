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

#include "WhiteBox_precompiled.h"

#include "WhiteBoxTestUtil.h"

namespace WhiteBox
{
    std::ostream& operator<<(std::ostream& os, const Api::FaceHandle faceHandle)
    {
        return os << Api::ToString(faceHandle).c_str();
    }

    std::ostream& operator<<(std::ostream& os, const Api::VertexHandle vertexHandle)
    {
        return os << Api::ToString(vertexHandle).c_str();
    }

    std::ostream& operator<<(std::ostream& os, const Api::PolygonHandle& polygonHandle)
    {
        return os << Api::ToString(polygonHandle).c_str();
    }

    std::ostream& operator<<(std::ostream& os, const Api::EdgeHandle edgeHandle)
    {
        return os << Api::ToString(edgeHandle).c_str();
    }

    std::ostream& operator<<(std::ostream& os, const Api::HalfedgeHandle halfedgeHandle)
    {
        return os << Api::ToString(halfedgeHandle).c_str();
    }

    std::ostream& operator<<(std::ostream& os, const Api::FaceVertHandles& faceVertHandles)
    {
        return os << Api::ToString(faceVertHandles).c_str();
    }

    std::ostream& operator<<(std::ostream& os, const Api::FaceVertHandlesList& faceVertHandlesList)
    {
        return os << Api::ToString(faceVertHandlesList).c_str();
    }
} // namespace WhiteBox

namespace UnitTest
{
    void Create2x2CubeGrid(WhiteBox::WhiteBoxMesh& whiteBox)
    {
        namespace Api = WhiteBox::Api;

        Api::InitializeAsUnitCube(whiteBox);

        // form a 2x2 grid of connected cubes
        Api::TranslatePolygonAppend(whiteBox, Api::FacePolygonHandle(whiteBox, Api::FaceHandle{4}), 1.0f);
        Api::HideEdge(whiteBox, Api::EdgeHandle{12});
        Api::TranslatePolygonAppend(whiteBox, Api::FacePolygonHandle(whiteBox, Api::FaceHandle{5}), 1.0f);
    }

    void Create3x3CubeGrid(WhiteBox::WhiteBoxMesh& whiteBox)
    {
        namespace Api = WhiteBox::Api;

        Api::InitializeAsUnitCube(whiteBox);

        // form a 3x3 grid of connected cubes
        Api::TranslatePolygonAppend(whiteBox, Api::FacePolygonHandle(whiteBox, Api::FaceHandle{4}), 1.0f);
        Api::TranslatePolygonAppend(whiteBox, Api::FacePolygonHandle(whiteBox, Api::FaceHandle{11}), 1.0f);
        Api::HideEdge(whiteBox, Api::EdgeHandle{21});
        Api::HideEdge(whiteBox, Api::EdgeHandle{12});
        Api::TranslatePolygonAppend(whiteBox, Api::FacePolygonHandle(whiteBox, Api::FaceHandle{27}), 1.0f);
        Api::TranslatePolygonAppend(whiteBox, Api::FacePolygonHandle(whiteBox, Api::FaceHandle{26}), 1.0f);
    }

    void HideAllTopUserEdgesFor2x2Grid(WhiteBox::WhiteBoxMesh& whiteBox)
    {
        namespace Api = WhiteBox::Api;

        Api::HideEdge(whiteBox, Api::EdgeHandle{43});
        Api::HideEdge(whiteBox, Api::EdgeHandle{12});
        Api::HideEdge(whiteBox, Api::EdgeHandle{4});
    }

    void HideAllTopUserEdgesFor3x3Grid(WhiteBox::WhiteBoxMesh& whiteBox)
    {
        namespace Api = WhiteBox::Api;

        // hide all top 'user' edges (top is one polygon)
        Api::HideEdge(whiteBox, Api::EdgeHandle{41});
        Api::HideEdge(whiteBox, Api::EdgeHandle{12});
        Api::HideEdge(whiteBox, Api::EdgeHandle{59});
        Api::HideEdge(whiteBox, Api::EdgeHandle{47});
        Api::HideEdge(whiteBox, Api::EdgeHandle{4});
        Api::HideEdge(whiteBox, Api::EdgeHandle{48});
        Api::HideEdge(whiteBox, Api::EdgeHandle{27});
        Api::HideEdge(whiteBox, Api::EdgeHandle{45});
    }
} // namespace UnitTest
