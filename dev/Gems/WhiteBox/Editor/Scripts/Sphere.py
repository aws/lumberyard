"""
All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
its licensors.

For complete copyright and license terms please see the LICENSE at the root of this
distribution (the "License"). All use of this software is governed by the License,
or, if provided, by the license below or the license accompanying this file. Do not
remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
"""

import Icosahedron
import WhiteBoxMath as whiteBoxMath
import WhiteBoxInit as init 

import argparse
import azlmbr.legacy.general as general
import azlmbr.bus as bus
import azlmbr.whitebox.api as api

# usage: pyRunFile path/to/file/sphere.py <subdivisions>


# divide each triangular face into four smaller faces
def subdivide_faces(whiteBoxMesh, faces, radius):
    new_faces = []
    for faceVertHandles in faces:
        # get each vertex
        v0 = faceVertHandles.VertexHandles[0]
        v1 = faceVertHandles.VertexHandles[1]
        v2 = faceVertHandles.VertexHandles[2]

        # use vertex positions to get their normalized midpoints
        positions = whiteBoxMesh.VertexPositions(faceVertHandles.VertexHandles)
        v3 = whiteBoxMesh.AddVertex(whiteBoxMath.normalize_midpoint(positions[0], positions[1], radius))
        v4 = whiteBoxMesh.AddVertex(whiteBoxMath.normalize_midpoint(positions[1], positions[2], radius))
        v5 = whiteBoxMesh.AddVertex(whiteBoxMath.normalize_midpoint(positions[0], positions[2], radius))

        # create four subdivided faces for each face
        new_faces.append(api.util_MakeFaceVertHandles(v0, v3, v5))
        new_faces.append(api.util_MakeFaceVertHandles(v3, v1, v4))
        new_faces.append(api.util_MakeFaceVertHandles(v4, v2, v5))
        new_faces.append(api.util_MakeFaceVertHandles(v3, v4, v5))

    return new_faces
    

# create sphere by subdividing an icosahedron
def create_sphere(whiteBoxMesh, subdivisions=2, radius=0.55):
    # create icosahedron faces and subdivide them to create a sphere
    icosahedron_faces = Icosahedron.create_icosahedron_faces(whiteBoxMesh, radius)
    for division in range (0, subdivisions):
        icosahedron_faces = subdivide_faces(whiteBoxMesh, icosahedron_faces, radius)
    for face in icosahedron_faces:
        whiteBoxMesh.AddPolygon([face])


if __name__ == "__main__":
    # cmdline arguments
    parser = argparse.ArgumentParser(description='Creates a sphere shaped white box mesh.')
    parser.add_argument('subdivisions', nargs='?', default=3, choices=range(0, 5), type=int, help='number of subdivisions to form sphere from icosahedron')
    parser.add_argument('radius', nargs='?', default=0.55, type=float, help='radius of the sphere')
    args = parser.parse_args()

    # initialize whiteBoxMesh
    whiteBoxEntity = init.create_white_box_entity("WhiteBox-Sphere")
    whiteBoxMeshComponent = init.create_white_box_component(whiteBoxEntity)
    whiteBoxMesh = init.create_white_box_handle(whiteBoxMeshComponent)

    # clear whiteBoxMesh to make a sphere from scratch
    whiteBoxMesh.Clear()

    create_sphere(whiteBoxMesh, args.subdivisions, args.radius)

    # update whiteBoxMesh
    init.update_white_box(whiteBoxMesh, whiteBoxMeshComponent)