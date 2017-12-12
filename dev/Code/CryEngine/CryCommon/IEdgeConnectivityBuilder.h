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

#ifndef CRYINCLUDE_CRYCOMMON_IEDGECONNECTIVITYBUILDER_H
#define CRYINCLUDE_CRYCOMMON_IEDGECONNECTIVITYBUILDER_H
#pragma once


class IStencilShadowConnectivity;
class CStencilShadowConnectivity;

//
//
class IStencilShadowConnectivity
{
public:
    typedef unsigned short vindex;              // vertex index (0..0xffff)

    // <interfuscator:shuffle>
    //! don't forget to call Release for freeing the memory resources
    virtual void Release(void) = 0;

    //! to keep the interface small and the access fast
    //! /return pointer to the internal memory representation (only used within module 3DEngine)
    const virtual CStencilShadowConnectivity* GetInternalRepresentation(void) const = 0;

    //! for debugging and profiling
    //! /param outVertexCount
    //! /param outTriangleCount
    virtual void GetStats(DWORD& outVertexCount, DWORD& outTriangleCount) = 0;

    //! Memorize the vertex buffer in this connectivity object. This is needed if a static object doesn't need this info
    virtual void SetVertices(const Vec3* pVertices, unsigned numVertices) = 0;

    //! Serializes this object to the given memory block; if it's NULL, only returns
    //! the number of bytes required. If it's not NULL, returns the number of bytes written
    //! (0 means error, insufficient space)
    virtual unsigned Serialize (bool bSave, void* pStream, unsigned nSize, IMiniLog* pWarningLog = NULL) = 0;

    //! Calculates the size of this object
    virtual void GetMemoryUsage(ICrySizer* pSizer) = 0;

    //! If this returns true, then the meshing functions don't need external input of mesh information
    //! This is for static objects only
    virtual bool IsStandalone() const {return false; };

    //! number of orphaned (open) edges
    //! /return orphaned (open) count
    virtual unsigned numOrphanEdges() const = 0;
    // </interfuscator:shuffle>

#ifdef WIN32
    //! /param szFilename filename with path (or relative) and extension
    //! /return true=success, false otherwise
    virtual bool DebugConnectivityInfo(const char* szFilename) = 0;
#endif
};

// (don't copy the interface pointer and don't forget to call Release)
class IEdgeConnectivityBuilder
{
public:
    typedef IStencilShadowConnectivity::vindex vindex;
    // <interfuscator:shuffle>
    virtual ~IEdgeConnectivityBuilder() {}

    //! return to the state right after construction
    virtual void Reinit(void) = 0;

    virtual void SetWeldTolerance (float fTolerance) {}

    //! reserves space for the given number of triangles that are to be added
    //! /param innNumTriangles
    //! /param innNumVertices
    virtual void ReserveForTriangles(unsigned innNumTriangles, unsigned innNumVertices) = 0;

    //! adds a single triangle to the mesh
    //! the triangle is defined by three vertices, in counter-clockwise order
    //! these vertex indices will be used later when accessing the array of
    //! deformed character/model vertices to determine the shadow volume boundary
    //! /param nV0 vertex index one 0..0xffff
    //! /param nV1 vertex index two 0..0xffff
    //! /param nV2 vertex index three 0..0xffff
    virtual void AddTriangle(vindex nV0, vindex nV1, vindex nV2) = 0;

    //! slower than AddTriangle but with the auto weld feature (if there are vertices with the same position your result is smaller and therefore faster)
    //! /param nV0 vertex index one 0..0xffff
    //! /param nV1 vertex index two 0..0xffff
    //! /param nV2 vertex index three 0..0xffff
    //! /param vV0 original vertex one position
    //! /param vV1 original vertex two position
    //! /param vV2 original vertex three position
    virtual void AddTriangleWelded(vindex nV0, vindex nV1, vindex nV2, const Vec3& vV0, const Vec3& vV1, const Vec3& vV2) = 0;

    //! constructs/compiles the optimum representation of the connectivity
    //! to be used in run-time
    //! WARNING: use Release method to dispose the connectivity object
    //! /return
    virtual IStencilShadowConnectivity* ConstructConnectivity(void) = 0;

    //! returns the number of single (with no pair faces found) or orphaned edges
    //! /return
    virtual unsigned numOrphanedEdges(void) const = 0;

    //! Returns the list of faces for orphaned edges into the given buffer;
    //! For each orphaned edge, one face will be returned; some faces may be duplicated
    virtual void getOrphanedEdgeFaces (unsigned* pBuffer) = 0;

    //! return memory usage
    virtual void GetMemoryUsage(class ICrySizer* pSizer) {}  // todo: implement
    // </interfuscator:shuffle>
};

// *****************************************************************

// used for runtime edge calculations
class IEdgeDetector
{
public:
    typedef unsigned short vindex;

    // <interfuscator:shuffle>
    virtual ~IEdgeDetector(){}
    // vertex index

    //! for deformable objects
    //! /param pConnectivity must not be 0 - don't use if there is nothing to do
    //! /param invLightPos
    //! /param pDeformedVertices must not be 0 - don't use if there is nothing to do
    virtual void BuildSilhuetteFromPos(const IStencilShadowConnectivity* pConnectivity, const Vec3& invLightPos, const Vec3* pDeformedVertices) = 0;

    //! for undeformed objects)
    //! /param outiNumTriangles, must not be 0 - don't use if there is nothing to do
    //! /return pointer to the cleared (set to zero) bitfield where each bit represents the orientation of one triangle
    virtual unsigned* getOrientationBitfield(int iniNumTriangles) = 0;

    //! /param pConnectivity must not be 0 - don't use if there is nothing to do
    //! /param inpVertices must not be 0 - don't use if there is nothing to do
    virtual void BuildSilhuetteFromBitfield(const IStencilShadowConnectivity* pConnectivity, const Vec3* inpVertices) = 0;

    // pointer to the triplets defining shadow faces
    virtual const vindex* getShadowFaceIndices(unsigned& outCount) const = 0;

    //! O(1), returs the pointer to an array of unsigned short pairs of vertex numbers
    virtual const vindex* getShadowEdgeArray(unsigned& outiNumEdges) const = 0;

    //!
    //! /return
    virtual unsigned numShadowVolumeVertices(void) const = 0;

    //! number of indices required to define the shadow volume,
    //! use this to determine the size of index buffer passed to meshShadowVolume
    //! this is always a dividend of 3
    //! /return
    virtual unsigned numShadowVolumeIndices(void) const = 0;

    //! make up the shadow volume
    //! constructs the shadow volume mesh, and puts the mesh definition into the
    //! vertex buffer (vertices that define the mesh) and index buffer (triple
    //! integers defining the triangular faces, counterclockwise order)
    //! /param vLight
    //! /param fFactor
    //! /param outpVertexBuf The size of the vertex buffer must be at least numVertices()
    //! /param outpIndexBuf The size of the index buffer must be at least numIndices()
    virtual void meshShadowVolume (Vec3 vLight, float fFactor, Vec3* outpVertexBuf, unsigned short* outpIndexBuf) = 0;

    //! get memory usage
    virtual void GetMemoryUsage(class ICrySizer* pSizer) {}
    // </interfuscator:shuffle>
};





#endif // CRYINCLUDE_CRYCOMMON_IEDGECONNECTIVITYBUILDER_H
