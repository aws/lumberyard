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

// Description : CloudGroup object definition.


#ifndef CRYINCLUDE_EDITOR_OBJECTS_CLOUDGROUP_H
#define CRYINCLUDE_EDITOR_OBJECTS_CLOUDGROUP_H
#pragma once


#include "Group.h"

class CCloudSprite;
struct ICloudRenderNode;

/*!
 *  CCloudGroup groups object together.
 *  Object can only be assigned to one group.
 *
 */
class CCloudGroup
    : public CGroup
{
    Q_OBJECT
public:
    CCloudGroup();
    virtual ~CCloudGroup();

    static const GUID& GetClassID()
    {
        // {d397bd6a-07d1-4673-ba11-c1fa64bc2f21}
        static const GUID guid = {
            0xd397bd6a, 0x07d1, 0x4673, { 0xba, 0x11, 0xc1, 0xfa, 0x64, 0xbc, 0x2f, 0x21 }
        };
        return guid;
    }

    virtual bool CreateGameObject();
    void Display(DisplayContext& dc);
    void OnChildModified();

    void BeginEditParams(IEditor* ie, int flags);
    void EndEditParams(IEditor* ie);

    void InvalidateTM(int nWhyFlags);
    void UpdateVisibility(bool visible);

    void Serialize(CObjectArchive& ar);

    void UpdateRenderObject();
    XmlNodeRef GenerateXml();
    void Generate();
    void Export();
    void Import();
    bool ImportFS(const QString& name);
    bool ImportExportFolder(const QString& fileName);
    void AddSprite(CCloudSprite* pSprite);
    void ReleaseCloudSprites();
    void BrowseFile();
    QString GetExportFileName();
    void UpdateSpritesBox();

protected:

    void OnParamChange(IVariable* pVar);
    void OnPreviewChange(IVariable* pVar);
    void FillBox(CBaseObject* pObj);

    CVariableArray varTextureArray;
    CVariable<int> mv_numRows;
    CVariable<int> mv_numCols;

    CVariableArray varCloudArray;
    CVariable<int> mv_spriteRow;
    CVariable<int> mv_numSprites;
    CVariable<float> mv_sizeSprites;
    CVariable<float> mv_cullDistance;
    CVariable<float> mv_randomSizeValue;
    CVariable<float> m_angleVariations;
    CVariable<bool> mv_spritePerBox;
    CVariable<float> mv_density;

    CVariableArray varVisArray;
    //CVariable<bool> mv_isRegeneratePos;
    CVariable<bool> mv_bShowSpheres;
    CVariable<bool> mv_bPreviewCloud;
    CVariable<bool> mv_bAutoUpdate;

    CRenderObject* pObj;
    SShaderItem SI;

    QString m_exportFile;

    CCloudSprite** ppCloudSprites;
    int nNumCloudSprites;

    float m_boxTotArea;

    AABB m_SpritesBox;

    ICloudRenderNode* m_pCloudRenderNode;
};

#endif // CRYINCLUDE_EDITOR_OBJECTS_CLOUDGROUP_H
