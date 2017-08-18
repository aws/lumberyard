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
#ifndef CRYINCLUDE_EDITOR_INCLUDES_IPARTICLEMANAGER_H
#define CRYINCLUDE_EDITOR_INCLUDES_IPARTICLEMANAGER_H
#pragma once

#include "BaseLibraryManager.h"
#include "ParticleParams.h"

class CParticleItem;
struct SExportParticleEffect;

struct IEditorParticleManager
    : public CBaseLibraryManager
{
    virtual ~IEditorParticleManager(){}
    // Clear all prototypes and material libraries.
    virtual void ClearAll() = 0;

    virtual void DeleteItem(IDataBaseItem* pItem) = 0;

    //////////////////////////////////////////////////////////////////////////
    // Materials.
    //////////////////////////////////////////////////////////////////////////
    //! Loads a new material from a xml node.
    virtual void PasteToParticleItem(CParticleItem* pItem, XmlNodeRef& node, bool bWithChilds) = 0;

    //! Export particle systems to game.
    virtual void Export(XmlNodeRef& node) = 0;

    //! Root node where this library will be saved.
    virtual QString GetRootNodeName() = 0;

    //! Get path to default emitter xml file
    virtual QString GetDefaultEmitterXmlPath() const = 0;

    //! Loads the default emitter parameter to pItem based on its emitter type
    virtual void LoadDefaultEmitter(IParticleEffect* effect) = 0;

    //! Load default emitter parameters for each type of emitter and save them for late use.
    virtual void LoadAllDefaultEmitters() = 0;

    //! Get default particle parameters for specified emitter shape
    virtual const ParticleParams& GetEmitterDefaultParams(ParticleParams::EEmitterShapeType emitterShape) = 0;
};

#endif // CRYINCLUDE_EDITOR_INCLUDES_IPARTICLEMANAGER_H
