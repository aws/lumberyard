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

#ifndef CRYINCLUDE_EDITOR_OBJECTS_SMARTOBJECT_H
#define CRYINCLUDE_EDITOR_OBJECTS_SMARTOBJECT_H
#pragma once


#include "EntityObject.h"
#include "SmartObjects/SmartObjectsEditorDialog.h"

/*!
 *  CSmartObject is an special entity visible in editor but invisible in game, that's registered with AI system as an smart object.
 *
 */
class CSmartObject
    : public CEntityObject
{
    Q_OBJECT
protected:
    IStatObj *  m_pStatObj;
    AABB        m_bbox;
    _smart_ptr<IMaterial>  m_pHelperMtl;

    CSOLibrary::CClassTemplateData const* m_pClassTemplate;

public:
    //////////////////////////////////////////////////////////////////////////
    // Overrides from CBaseObject.
    //////////////////////////////////////////////////////////////////////////
    virtual bool Init(IEditor* ie, CBaseObject* prev, const QString& file);
    virtual void InitVariables() {}
    virtual void Done();
    virtual void Display(DisplayContext& disp);
    virtual bool HitTest(HitContext& hc);
    virtual void GetLocalBounds(AABB& box);
    bool IsScalable() { return false; }
    virtual void SetHelperScale(float scale) { m_helperScale = scale; }
    virtual float GetHelperScale() { return m_helperScale; }
    virtual void OnEvent(ObjectEvent eventID);
    virtual void GetScriptProperties(CEntityScript* pEntityScript, XmlNodeRef xmlProperties, XmlNodeRef xmlProperties2);
    virtual void BeginEditParams(IEditor* ie, int flags);
    virtual void EndEditParams(IEditor* ie);

    //////////////////////////////////////////////////////////////////////////

    IStatObj* GetIStatObj();

protected:
    friend class CTemplateObjectClassDesc<CSmartObject>;

    //! Dtor must be protected.
    CSmartObject();
    ~CSmartObject();

    static const GUID& GetClassID()
    {
        // {8287BBFD-2581-47c1-99AE-6102F9893B7A}
        static const GUID guid = {
            0x8287bbfd, 0x2581, 0x47c1, { 0x99, 0xae, 0x61, 0x02, 0xf9, 0x89, 0x3b, 0x7a }
        };
        return guid;
    };

    float GetRadius();

    void DeleteThis() { delete this; }
    _smart_ptr<IMaterial> GetHelperMaterial();

public:
    void OnPropertyChange(IVariable* var);
};

#endif // CRYINCLUDE_EDITOR_OBJECTS_SMARTOBJECT_H
