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

#ifndef CRYINCLUDE_EDITOR_PARTICLES_PARTICLEMANAGER_H
#define CRYINCLUDE_EDITOR_PARTICLES_PARTICLEMANAGER_H
#pragma once

#include "Include/IEditorParticleManager.h"

class CParticleItem;
class CParticleLibrary;
struct SExportParticleEffect;

AZ_PUSH_DISABLE_DLL_EXPORT_BASECLASS_WARNING
/** Manages Particle libraries and systems.
*/
class CRYEDIT_API CEditorParticleManager
    : public IEditorParticleManager
{
AZ_POP_DISABLE_DLL_EXPORT_BASECLASS_WARNING
public:
    CEditorParticleManager();
    ~CEditorParticleManager();

    // Clear all prototypes and material libraries.
    void ClearAll() override;

    void DeleteItem(IDataBaseItem* pItem) override;

    //////////////////////////////////////////////////////////////////////////
    // Materials.
    //////////////////////////////////////////////////////////////////////////
    //! Loads a new material from a xml node.
    void PasteToParticleItem(CParticleItem* pItem, XmlNodeRef& node, bool bWithChilds) override;

    //! Export particle systems to game.
    void Export(XmlNodeRef& node) override;

    //! Root node where this library will be saved.
    QString GetRootNodeName() override;

    //! Get path to default emitter xml file
    QString GetDefaultEmitterXmlPath() const override;

    //! Loads the default emitter parameter to pItem based on its emitter shape
    void LoadDefaultEmitter(IParticleEffect* effect) override;

    //! load default emitter parameters for each shape of emitter and save them for late use.
    void LoadAllDefaultEmitters() override;

    //! get default particle parameters for specified emitter shape
    const ParticleParams& GetEmitterDefaultParams(ParticleParams::EEmitterShapeType emitterShape) override;

protected:

    void AddExportItem(std::vector<SExportParticleEffect>& effects, CParticleItem* pItem, int parent);
    virtual CBaseLibraryItem* MakeNewItem();
    virtual CBaseLibrary* MakeNewLibrary();

    //! Path to libraries in this manager.
    virtual QString GetLibsPath();

    QString m_libsPath;
    
    //default params for each shape of particle emitters. 
    ParticleParams m_defaultEmitterTypeParams[ParticleParams::EEmitterShapeType::_count];

    bool m_isDefaultParamsLoaded;
};

#endif // CRYINCLUDE_EDITOR_PARTICLES_PARTICLEMANAGER_H
