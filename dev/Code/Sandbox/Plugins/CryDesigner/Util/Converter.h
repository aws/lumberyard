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

#pragma once
////////////////////////////////////////////////////////////////////////////
//  Crytek Engine Source File.
//  (c) 2001 - 2013 Crytek GmbH
// -------------------------------------------------------------------------
//  File name:   Converter.h
//  Created:     April/20/2012 by Jaesik.
////////////////////////////////////////////////////////////////////////////
class DesignerObject;

namespace CD
{
    class Converter
    {
    public:

        bool CreateNewDesignerObject();
        bool ConvertToDesignerObject();

        static bool ConvertSolidXMLToDesignerObject(XmlNodeRef pSolidNode, DesignerObject* pDesignerObject);
        static bool ConvertMeshToBrushDesigner(IIndexedMesh* pMesh, CD::Model* pDesignerObject);

    private:

        struct SSolidPolygon
        {
            std::vector<uint16> vIndexList;
            int matID;
            CD::STexInfo texinfo;
        };

        static void LoadTexInfo(CD::STexInfo*  texinfo, const XmlNodeRef& node);
        static void LoadPolygon(SSolidPolygon* polygon, const XmlNodeRef& polygonNode);
        static void LoadVertexList(std::vector<BrushVec3>& vertexlist, const XmlNodeRef& node);

        static void AddPolygonsToDesigner(const std::vector<SSolidPolygon>& polygonList, const std::vector<BrushVec3>& vList, DesignerObject* pDesignerObject);

    private:

        struct SSelectedMesh
        {
            SSelectedMesh()
            {
                m_pIndexedMesh = NULL;
                m_bLoadedIndexedMeshFromFile = false;
                m_pMaterial = NULL;
            }
            Matrix34 m_worldTM;
            CMaterial* m_pMaterial;
            IIndexedMesh* m_pIndexedMesh;
            bool m_bLoadedIndexedMeshFromFile;
            _smart_ptr<CBaseObject> m_pOriginalObject;
        };

        void GetSelectedObjects(std::vector<SSelectedMesh>& pObjects) const;

        DesignerObject* CreateDesignerObject(IIndexedMesh* pMesh);
        bool ConvertMeshToDesignerObject(DesignerObject* pDesignerObject, IIndexedMesh* pMesh);

        void CreateDesignerObjects(std::vector<SSelectedMesh>& pSelectedMeshes, std::vector<DesignerObject*>& pOutDesignerObjects);
    };
};