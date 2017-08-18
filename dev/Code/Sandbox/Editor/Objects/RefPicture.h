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

#ifndef CRYINCLUDE_EDITOR_OBJECTS_REFPICTURE_H
#define CRYINCLUDE_EDITOR_OBJECTS_REFPICTURE_H
#pragma once


#include "BaseObject.h"

class CMaterial;
class CEdMesh;

//////////////////////////////////////////////////////////////////////////
class CRefPicture
    : public CBaseObject
{
    Q_OBJECT
public:
    //-----------------------------------------------------------------------------
    //!
    CRefPicture();

    static const GUID& GetClassID()
    {
        // {76ED2D33-95B5-4a54-A3C6-ED1B9D6DCEEE}
        static const GUID guid =
        {
            0x76ed2d33, 0x95b5, 0x4a54, { 0xa3, 0xc6, 0xed, 0x1b, 0x9d, 0x6d, 0xce, 0xee }
        };
        return guid;
    }

    //-----------------------------------------------------------------------------
    //!
    void Display(DisplayContext& dc);

    //-----------------------------------------------------------------------------
    //!
    bool HitTest(HitContext& hc);

    //-----------------------------------------------------------------------------
    //!
    void GetLocalBounds(AABB& box);

    //-----------------------------------------------------------------------------
    //!
    int MouseCreateCallback(CViewport* view, EMouseEvent event, QPoint& point, int flags);

    //-----------------------------------------------------------------------------
    //!
    XmlNodeRef Export(const QString& levelPath, XmlNodeRef& xmlNode);

protected:
    //-----------------------------------------------------------------------------
    //!
    bool CreateGameObject();

    //-----------------------------------------------------------------------------
    //!
    void Done();

    //-----------------------------------------------------------------------------
    //!
    void DeleteThis() { delete this; };

    //-----------------------------------------------------------------------------
    //!
    void InvalidateTM(int nWhyFlags);

    //-----------------------------------------------------------------------------
    //!
    void OnVariableChanged(IVariable*   piVariable);

    //-----------------------------------------------------------------------------
    //!
    void UpdateImage(const QString& picturePath);

    //-----------------------------------------------------------------------------
    //!
    void ApplyScale(float fScale);


    CVariable<QString>  mv_picFilepath;
    CVariable<float>    mv_scale;

private:
    _smart_ptr<IMaterial>   m_pMaterial;
    IRenderNode*            m_pRenderNode;
    _smart_ptr<CEdMesh>     m_pGeometry;

    float               m_aspectRatio;
    AABB                m_bbox;
    Matrix34            m_invertTM;
};

#endif // CRYINCLUDE_EDITOR_OBJECTS_REFPICTURE_H
