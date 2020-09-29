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

#include <AzTest/AzTest.h>
#include <WhiteBox/WhiteBoxToolApi.h>

namespace WhiteBox
{
    std::ostream& operator<<(std::ostream& os, Api::FaceHandle faceHandle);
    std::ostream& operator<<(std::ostream& os, Api::VertexHandle vertexHandle);
    std::ostream& operator<<(std::ostream& os, Api::PolygonHandle& polygonHandle);
    std::ostream& operator<<(std::ostream& os, Api::EdgeHandle edgeHandle);
    std::ostream& operator<<(std::ostream& os, Api::HalfedgeHandle halfedgeHandle);
    std::ostream& operator<<(std::ostream& os, Api::FaceVertHandles& faceVertHandles);
    std::ostream& operator<<(std::ostream& os, Api::FaceVertHandlesList& faceVertHandlesList);
} // namespace WhiteBox

namespace UnitTest
{
    // Custom tuple matcher for comparing Vector3 elements using IsClose
    MATCHER(IsClose, "")
    {
        return std::get<0>(arg).IsClose(std::get<1>(arg));
    }

    void Create2x2CubeGrid(WhiteBox::WhiteBoxMesh& whiteBox);
    void Create3x3CubeGrid(WhiteBox::WhiteBoxMesh& whiteBox);
    void HideAllTopUserEdgesFor2x2Grid(WhiteBox::WhiteBoxMesh& whiteBox);
    void HideAllTopUserEdgesFor3x3Grid(WhiteBox::WhiteBoxMesh& whiteBox);
} // namespace UnitTest
