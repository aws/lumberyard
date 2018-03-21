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

#include "StdAfx.h"
#include "ParticleManager.h"

#include "ParticleItem.h"
#include "ParticleLibrary.h"

#include "GameEngine.h"

#include <AzCore/std/containers/array.h>

#define PARTICLES_LIBS_PATH "Libs/Particles/"
#define DEFAULT_EMITTER_XML "Editor/Plugins/ParticleEditorPlugin/defaults/DefaultParticleEmitters.xml"

//////////////////////////////////////////////////////////////////////////
// CEditorParticleManager implementation.
//////////////////////////////////////////////////////////////////////////
CEditorParticleManager::CEditorParticleManager()
    : m_isDefaultParamsLoaded(false)
{
    m_bUniqNameMap = true;
    m_bUniqGuidMap = false;
    m_pLevelLibrary = nullptr;
}

//////////////////////////////////////////////////////////////////////////
CEditorParticleManager::~CEditorParticleManager()
{
}

//////////////////////////////////////////////////////////////////////////
void CEditorParticleManager::ClearAll()
{
    CBaseLibraryManager::ClearAll();

    m_pLevelLibrary = nullptr;
}

//////////////////////////////////////////////////////////////////////////
CBaseLibraryItem* CEditorParticleManager::MakeNewItem()
{
    return new CParticleItem();
}
//////////////////////////////////////////////////////////////////////////
CBaseLibrary* CEditorParticleManager::MakeNewLibrary()
{
    return new CParticleLibrary(this);
}
//////////////////////////////////////////////////////////////////////////
QString CEditorParticleManager::GetRootNodeName()
{
    return "ParticleLibs";
}

QString CEditorParticleManager::GetDefaultEmitterXmlPath() const
{
    return DEFAULT_EMITTER_XML;
}

//////////////////////////////////////////////////////////////////////////
QString CEditorParticleManager::GetLibsPath()
{
    if (m_libsPath.isEmpty())
    {
        m_libsPath = PARTICLES_LIBS_PATH;
    }
    return m_libsPath;
}

//////////////////////////////////////////////////////////////////////////
void CEditorParticleManager::Export(XmlNodeRef& node)
{
    XmlNodeRef libs = node->newChild("ParticlesLibrary");
    for (int i = 0; i < GetLibraryCount(); i++)
    {
        IDataBaseLibrary* pLib = GetLibrary(i);
        if (pLib->IsLevelLibrary())
        {
            continue;
        }
        // Level libraries are saved in level.
        XmlNodeRef libNode = libs->newChild("Library");
        libNode->setAttr("Name", pLib->GetName().toUtf8().data());
    }
}

//////////////////////////////////////////////////////////////////////////
void CEditorParticleManager::PasteToParticleItem(CParticleItem* pItem, XmlNodeRef& node, bool bWithChilds)
{
    assert(pItem);
    assert(node != NULL);

    CBaseLibraryItem::SerializeContext serCtx(node, true);
    serCtx.bCopyPaste = true;
    serCtx.bIgnoreChilds = !bWithChilds;
    pItem->Serialize(serCtx);
}

//////////////////////////////////////////////////////////////////////////
void CEditorParticleManager::DeleteItem(IDataBaseItem* pItem)
{
    CParticleItem* pPartItem = ((CParticleItem*)pItem);

    // Set the new parent of the children of this particle item to nullptr
    for (int i = pPartItem->GetChildCount() - 1; i >= 0; --i)
    {
        static_cast<CParticleItem*>(pPartItem->GetChild(i))->SetParent(nullptr);
    }

    if (pPartItem->GetParent())
    {
        // Setting the parent of this item to nullptr will also remove this item from the childlist of the current parent
        pPartItem->SetParent(nullptr);
    }

    CBaseLibraryManager::DeleteItem(pItem);
}

void CEditorParticleManager::LoadDefaultEmitter(IParticleEffect* effect)
{
    if (effect == nullptr)
    {
        AZ_Assert(false, "LoadDefaultEmitter: pItem shouldn't be null");
        return;
    }

    if (!m_isDefaultParamsLoaded)
    {
        LoadAllDefaultEmitters();
    }

    //set the particle parameter from default one
    ParticleParams::EEmitterShapeType shape = effect->GetParticleParams().GetEmitterShape();
    effect->SetParticleParams(m_defaultEmitterTypeParams[shape]);
}

void CEditorParticleManager::LoadAllDefaultEmitters()
{
    //set to true for avoiding some frequent loading attempts
    m_isDefaultParamsLoaded = true;

    //load default emitters from xml
    XmlNodeRef xml = GetIEditor()->GetSystem()->LoadXmlFromFile(DEFAULT_EMITTER_XML);
    if (xml == 0)
    {
        //output error
        AZ_Error("Editor", false, "Default particle emitter configuration file [%s] does not exist", DEFAULT_EMITTER_XML);
        return;
    }

    //flags to check whether the default parameter of certain type of emitter are set in the xml
    AZStd::array <bool, ParticleParams::EEmitterShapeType::_count> checked = { { false } };

    //compare each particle's name with emitter shape type name
    for (int childIdx = 0; childIdx < xml->getChildCount(); childIdx++)
    {
        XmlNodeRef defEmit = xml->getChild(childIdx);
        const char* nodeName = defEmit->getAttr("name");
        for (int shapeIdx = 0; shapeIdx < ParticleParams::EEmitterShapeType::_count; shapeIdx++)
        {
            cstr name = ParticleParams::EEmitterShapeType::TypeInfo().ToName(shapeIdx);
            if (azstricmp(name, nodeName) == 0)
            {
                //load matching particle effect
                IParticleEffect *effect = gEnv->pParticleManager->LoadEffect(nodeName, defEmit, false);
                if (effect)
                {
                    m_defaultEmitterTypeParams[shapeIdx] = effect->GetParticleParams();
                    gEnv->pParticleManager->DeleteEffect(effect);
                    checked[shapeIdx] = true;
                }
                break;
            }
        }
    }

    //output warning
    for (int shapeIdx = 0; shapeIdx < ParticleParams::EEmitterShapeType::_count; shapeIdx++)
    {
        if (!checked[shapeIdx])
        {
            AZ_Warning("Editor", false, "Can't load default particle parameters for emitter shape [ %s ]", ParticleParams::EEmitterShapeType::TypeInfo().ToName(shapeIdx));
        }
    }
}

const ParticleParams& CEditorParticleManager::GetEmitterDefaultParams(ParticleParams::EEmitterShapeType emitterShape)
{
    if (!m_isDefaultParamsLoaded)
    {
        LoadAllDefaultEmitters();
    }

    return m_defaultEmitterTypeParams[emitterShape];
}
