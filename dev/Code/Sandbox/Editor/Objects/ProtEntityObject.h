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

#ifndef CRYINCLUDE_EDITOR_OBJECTS_PROTENTITYOBJECT_H
#define CRYINCLUDE_EDITOR_OBJECTS_PROTENTITYOBJECT_H
#pragma once

#include "EntityObject.h"

class CEntityPrototype;
/*!
 *  Prototype entity object.
 *
 */
class CProtEntityObject
    : public CEntityObject
{
    Q_OBJECT
public:
    //////////////////////////////////////////////////////////////////////////
    // Ovverides from CBaseObject.
    //////////////////////////////////////////////////////////////////////////
    bool Init(IEditor* ie, CBaseObject* prev, const QString& file);
    void Done();
    bool CreateGameObject();

    QString GetTypeDescription() const { return m_prototypeName; };

    void BeginEditParams(IEditor* ie, int flags);
    void EndEditParams(IEditor* ie);
    void BeginEditMultiSelParams(bool bAllOfSameType);
    void EndEditMultiSelParams();
    bool HasMeasurementAxis() const {   return true;    }

    void Serialize(CObjectArchive& ar);

    XmlNodeRef Export(const QString& levelPath, XmlNodeRef& xmlNode);
    //////////////////////////////////////////////////////////////////////////

    void SetPrototype(REFGUID guid, bool bForceReload = false);
    void SetPrototype(CEntityPrototype* prototype, bool bForceReload, bool bOnlyDisabled = false);

protected:
    friend class CTemplateObjectClassDesc<CProtEntityObject>;

    //! Dtor must be protected.
    CProtEntityObject();

    static const GUID& GetClassID()
    {
        // {B9FB3D0B-ADA8-4380-A96F-A091E66E6EC1}
        static const GUID guid = {
            0xb9fb3d0b, 0xada8, 0x4380, { 0xa9, 0x6f, 0xa0, 0x91, 0xe6, 0x6e, 0x6e, 0xc1 }
        };
        return guid;
    }

    virtual void SpawnEntity();

    virtual void PostSpawnEntity();

    //! Callback called by prototype when its updated.
    void OnPrototypeUpdate();
    void SyncVariablesFromPrototype(bool bOnlyDisabled = false);

    //! Entity prototype name.
    QString m_prototypeName;
    //! Full prototype name.
    GUID m_prototypeGUID;
};

	
#endif // CRYINCLUDE_EDITOR_OBJECTS_PROTENTITYOBJECT_H
