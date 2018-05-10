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

#ifndef CRYINCLUDE_EDITOR_OBJECTS_ENVIRONMENTPROBEOBJECT_H
#define CRYINCLUDE_EDITOR_OBJECTS_ENVIRONMENTPROBEOBJECT_H
#pragma once


#include "EntityObject.h"

/*!
 *  CEnvironmentProbeObject is an object that represent an environment map generator
 *  also it encapsulates a light
 */

class CEnvironmentProbePanel;
struct IMaterial;

class SANDBOX_API CEnvironementProbeObject
    : public CEntityObject
{
    Q_OBJECT
public:
    //////////////////////////////////////////////////////////////////////////
    // Ovverides from CBaseObject.
    //////////////////////////////////////////////////////////////////////////
    bool Init(IEditor* ie, CBaseObject* prev, const QString& file);
    void InitVariables();
    void Serialize(CObjectArchive& ar);
    void BeginEditParams(IEditor* ie, int flags);
    void EndEditParams(IEditor* ie);
    void Display(DisplayContext& dc);
    void GetBoundBox(AABB& box);
    void GetLocalBounds(AABB& aabb);
    virtual void GenerateCubemap();
    void OnPreviewCubemap(IVariable*   piVariable);
    void ToggleCubemapPreview( bool enable );
    void OnCubemapResolutionChange(IVariable*  piVariable);
    _smart_ptr<IMaterial> CreateMaterial();
    virtual void UpdateLinks();

    virtual int AddEntityLink(const QString& name, GUID targetEntityId) override;
    void OnPropertyChanged(IVariable* pVar);
    void OnMultiSelPropertyChanged(IVariable* pVar);

protected:
    friend class CTemplateObjectClassDesc<CEnvironementProbeObject>;
    //! Dtor must be protected.
    CEnvironementProbeObject();

    static const GUID& GetClassID()
    {
        // {77048AAA-02A4-4a98-AAD4-71A0EBA659EE}
        static const GUID guid = {
            0x77048aaa, 0x2a4, 0x4a98, { 0xaa, 0xd4, 0x71, 0xa0, 0xeb, 0xa6, 0x59, 0xee }
        };
        return guid;
    }

    static CEnvironmentProbePanel* s_pProbePanel;
    static int s_panelProbeID;

    CSmartVariable<bool> m_preview_cubemap;
    CSmartVariableEnum<int> m_cubemap_resolution;
};

class CEnvironementProbeTODObject
    : public CEnvironementProbeObject
{
    Q_OBJECT
public:
    //////////////////////////////////////////////////////////////////////////
    // Overrides from CBaseObject.
    //////////////////////////////////////////////////////////////////////////
    void GenerateCubemap();
    void UpdateLinks();

protected:
    friend class CTemplateObjectClassDesc<CEnvironementProbeTODObject>;
    //! Dtor must be protected.
    CEnvironementProbeTODObject();

    static const GUID& GetClassID()
    {
        // {D7A9256C-2F1C-4665-A043-49671C6F723A}
        static const GUID guid = {
            0xd7a9256c, 0x2f1c, 0x4665, { 0xa0, 0x43, 0x49, 0x67, 0x1c, 0x6f, 0x72, 0x3a }
        };
        return guid;
    }

private:
    int m_timeSlots;
};

#endif // CRYINCLUDE_EDITOR_OBJECTS_ENVIRONMENTPROBEOBJECT_H
