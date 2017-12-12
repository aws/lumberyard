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
//  Copyright (C), Crytek Studios, 2011.
// -------------------------------------------------------------------------
//  File name:   ModelCompiler.h
//  Created:     14/9/2011 by Jaesik.
////////////////////////////////////////////////////////////////////////////

namespace CD
{
    class Model;

    enum ECompilerFlag
    {
        eCompiler_CastShadow = BIT(1),
        eCompiler_Physicalize = BIT(2),
        eCompiler_General = eCompiler_CastShadow | eCompiler_Physicalize
    };

    class ModelCompiler
        : public CRefCountBase
    {
    public:

        ModelCompiler(int nCompilerFlag);
        ModelCompiler(const ModelCompiler& compiler);
        virtual ~ModelCompiler();

        bool IsValid() const;

        void Compile(CBaseObject* pBaseObject, Model* pModel, ShelfID shelfID = -1, bool bUpdateOnlyRenderNode = false);

        void DisplayTriangulation(CBaseObject* pBaseObject, Model* pModel, DisplayContext& dc);
        void DeleteAllRenderNodes();
        void DeleteRenderNode(ShelfID shelfID);
        IRenderNode* GetRenderNode(){ return m_pRenderNode[0]; }
        bool GetIStatObj(_smart_ptr<IStatObj>* pStatObj);

        bool GenerateIndexMesh(Model* pModel, IIndexedMesh* pMesh, bool bGenerateBackFaces);
        void SaveToCgf(const char* filename);

        void SetViewDistanceMultiplier(float multiplier){ m_viewDistanceMultiplier = multiplier; }
        float GetViewDistanceMultiplier() const { return m_viewDistanceMultiplier; }

        void SetRenderFlags(int nRenderFlag);
        int GetRenderFlags() const { return m_RenderFlags; }

        void SetStaticObjFlags(int nStaticObjFlag);
        int GetStaticObjFlags() const;

        void AddFlags(int nFlags) { m_nCompilerFlag |= nFlags; }
        void RemoveFlags(int nFlags) { m_nCompilerFlag &= (~nFlags); }
        bool CheckFlags(int nFlags) const { return (m_nCompilerFlag & nFlags) ? true : false; }

        void SaveMesh(CArchive& ar, CBaseObject* pObj, Model* pModel);
        bool LoadMesh(CArchive& ar, CBaseObject* pObj, Model* pModel);
        bool SaveMesh(int nVersion, std::vector<char>& buffer, CBaseObject* pObj, Model* pModel);
        bool LoadMesh(int nVersion, std::vector<char>& buffer, CBaseObject* pObj, Model* pModel);

        int GetPolygonCount() const;

    private:

        bool UpdateMesh(CBaseObject* pBaseObject, Model* pModel);
        void UpdateRenderNode(CBaseObject* pBaseObject, Model* pModel);

        void RemoveStatObj();
        void CreateStatObj(int nShelf) const;
        _smart_ptr<IMaterial>  GetMaterialFromBaseObj(CBaseObject* pObj) const;
        void InvalidateStatObj(IStatObj* pStatObj, bool bPhysics);

    private:

        mutable IStatObj* m_pStatObj[kMaxShelfCount];
        mutable IRenderNode* m_pRenderNode[kMaxShelfCount];

        int m_RenderFlags;
        float m_viewDistanceMultiplier;
        int m_nCompilerFlag;
    };
};