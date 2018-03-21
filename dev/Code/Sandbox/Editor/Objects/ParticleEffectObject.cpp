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
#include "ParticleEffectObject.h"
#include "ParticleEffectPanel.h"
#include "Particles/ParticleDialog.h"
#include "DataBaseDialog.h"
#include "Include/IEditorParticleManager.h"
#include "Particles/ParticleItem.h"
#include "GameEngine.h"
#include "QtViewPaneManager.h"

// Confetti Begin: Jurecka
#include "Include/IParticleEditor.h"
// Confetti End: Jurecka

namespace
{
    int s_particleEntityPanelIndex = 0;
    CParticleEffectPanel* s_particleEntityPanel = 0;
}

void CParticleEffectObject::BeginEditParams(IEditor* ie, int flags)
{
    if (!s_particleEntityPanel && m_pEntity)
    {
        s_particleEntityPanel = new CParticleEffectPanel;
        s_particleEntityPanelIndex = AddUIPage("Particle Menu", s_particleEntityPanel);
    }
    if (s_particleEntityPanel)
    {
        s_particleEntityPanel->SetParticleEffectEntity(this);
    }

    CEntityObject::BeginEditParams(ie, flags);
}

void CParticleEffectObject::EndEditParams(IEditor* ie)
{
    if (s_particleEntityPanelIndex != 0)
    {
        RemoveUIPage(s_particleEntityPanelIndex);
    }
    s_particleEntityPanelIndex = 0;
    s_particleEntityPanel = 0;

    CEntityObject::EndEditParams(ie);
}

bool CParticleEffectObject::Init(IEditor* ie, CBaseObject* prev, const QString& file)
{
    if (CEntityObject::Init(ie, prev, file))
    {
        m_entityClass = "ParticleEffect";
        m_ParticleEntityName = file;
        return true;
    }
    return false;
}

void CParticleEffectObject::Display(DisplayContext& dc)
{
    const Matrix34& wtm = GetWorldTM();

    float fHelperScale = 1 * m_helperScale * gSettings.gizmo.helpersScale;
    Vec3 dir = wtm.TransformVector(Vec3(0, 0, fHelperScale));
    Vec3 wp = wtm.GetTranslation();
    if (IsFrozen())
    {
        dc.SetFreezeColor();
    }
    else
    {
        dc.SetColor(1, 1, 0);
    }
    dc.DrawArrow(wp, wp + dir * 2, fHelperScale);

    DrawDefault(dc);
}

QString CParticleEffectObject::GetParticleName()    const
{
    QString effectName;
    if (GetProperties())
    {
        IVariable* pVar = GetProperties()->FindVariable("ParticleEffect");
        if (pVar && pVar->GetType() == IVariable::STRING)
        {
            pVar->Get(effectName);
        }
    }

    if (effectName.isEmpty())
    {
        return m_ParticleEntityName;
    }

    return effectName;
}

QString CParticleEffectObject::GetComment() const
{
    QString comment;
    if (GetProperties())
    {
        IVariable* pVar = GetProperties()->FindVariable("Comment");
        if (pVar && pVar->GetType() == IVariable::STRING)
        {
            pVar->Get(comment);
        }
    }
    return comment;
}

bool CParticleEffectObject::IsGroup(const QString& effectName)
{
    //! In order to filter names about particle group in upper level to avoid unnecessary loading.
    int dotCounter = 0;
    for (int i = 0; i < effectName.length(); ++i)
    {
        if (effectName[i] == '.')
        {
            ++dotCounter;
        }
    }
    return dotCounter <= 1;
}

void CParticleEffectObject::OnMenuGoToDatabase() const
{
    QString particleName(GetParticleName());
    const QString libraryName = particleName.split(".").first();
    IEditorParticleManager* particleMgr = GetIEditor()->GetParticleManager();
    IDataBaseLibrary* pLibrary = particleMgr->FindLibrary(libraryName);
    if (pLibrary == NULL)
    {
        QString fullLibraryName = libraryName + ".xml";
        fullLibraryName = QString("Libs/Particles/") + fullLibraryName.toLower();
        GetIEditor()->SuspendUndo();
        pLibrary = particleMgr->LoadLibrary(fullLibraryName);
        GetIEditor()->ResumeUndo();
        if (pLibrary == NULL)
        {
            return;
        }
    }

    particleName.remove(0, libraryName.length() + 1);
    CBaseLibraryItem* pItem = (CBaseLibraryItem*)pLibrary->FindItem(particleName);
    if (pItem == NULL)
    {
        QString leafParticleName(particleName);

        int lastDotPos = particleName.lastIndexOf('.');
        while (!pItem && lastDotPos > -1)
        {
            particleName.remove(lastDotPos, particleName.length() - lastDotPos);
            lastDotPos = particleName.lastIndexOf('.');
            pItem = (CBaseLibraryItem*)pLibrary->FindItem(particleName);
        }

        leafParticleName.replace(particleName, "");
        if (leafParticleName.isEmpty() || (leafParticleName.length() == 1 && leafParticleName[0] == '.'))
        {
            return;
        }
        if (leafParticleName[0] == '.')
        {
            leafParticleName.remove(0, 1);
        }

        pItem = GetChildParticleItem((CParticleItem*)pItem, "", leafParticleName);
        if (pItem == NULL)
        {
            return;
        }
    }

    QtViewPaneManager::instance()->OpenPane(LyViewPane::DatabaseView);

    CDataBaseDialog* pDataBaseDlg = FindViewPane<CDataBaseDialog>(LyViewPane::DatabaseView);
    if (!pDataBaseDlg)
    {
        return;
    }
    pDataBaseDlg->SelectDialog(EDB_TYPE_PARTICLE);

    CParticleDialog* particleDlg = (CParticleDialog*)pDataBaseDlg->GetCurrent();
    if (particleDlg == NULL)
    {
        return;
    }

    particleDlg->Reload();
    particleDlg->SelectLibrary(libraryName);
    particleDlg->SelectItem(pItem);
}

// Confetti Begin: Jurecka
void CParticleEffectObject::OnMenuGoToEditor() const
{
    IParticleEditor* particleEditor = NULL;
    std::vector<IClassDesc*> classes;
    GetIEditor()->GetClassFactory()->GetClassesBySystemID(ESYSTEM_CLASS_EDITTOOL, classes);
    for (int i = 0; i < classes.size(); i++)
    {
        IClassDesc* pClass = classes[i];
        IParticleEditor* pEditor = NULL;
        HRESULT hRes = pClass->QueryInterface(__uuidof(IParticleEditor), (void**)&pEditor);
        if (!FAILED(hRes) && pEditor)
        {
            particleEditor = pEditor;
            break;
        }
    }

    if (particleEditor)
    {
        QString particleName(GetParticleName());
        const QString libraryName = particleName.split(".").first();
        IEditorParticleManager* particleMgr = GetIEditor()->GetParticleManager();
        IDataBaseLibrary* pLibrary = particleMgr->FindLibrary(libraryName);
        if (pLibrary == NULL)
        {
            QString fullLibraryName = libraryName + ".xml";
            fullLibraryName = QString("Libs/Particles/") + fullLibraryName.toLower();
            GetIEditor()->SuspendUndo();
            pLibrary = particleMgr->LoadLibrary(fullLibraryName);
            GetIEditor()->ResumeUndo();
            if (pLibrary == NULL)
            {
                return;
            }
        }

        particleName.remove(0, libraryName.length() + 1);
        CBaseLibraryItem* pItem = (CBaseLibraryItem*)pLibrary->FindItem(particleName);
        if (pItem == NULL)
        {
            QString leafParticleName(particleName);

            int lastDotPos = particleName.lastIndexOf('.');
            while (!pItem && lastDotPos > -1)
            {
                particleName.remove(lastDotPos, particleName.length() - lastDotPos);
                lastDotPos = particleName.lastIndexOf('.');
                pItem = (CBaseLibraryItem*)pLibrary->FindItem(particleName);
            }

            leafParticleName.replace(particleName, "");
            if (leafParticleName.isEmpty() || (leafParticleName.length() == 1 && leafParticleName[0] == '.'))
            {
                return;
            }
            if (leafParticleName[0] == '.')
            {
                leafParticleName.remove(0, 1);
            }

            pItem = GetChildParticleItem((CParticleItem*)pItem, "", leafParticleName);
            if (pItem == NULL)
            {
                return;
            }
        }

        particleEditor->Select(libraryName, pItem);
    }
}
// Confetti End: Jurecka

CParticleItem* CParticleEffectObject::GetChildParticleItem(CParticleItem* pParentItem, QString composedName, const QString& wishName) const
{
    if (!pParentItem || (pParentItem && pParentItem->GetChildCount() <= 0))
    {
        return NULL;
    }

    for (int i = 0; i < pParentItem->GetChildCount(); ++i)
    {
        CParticleItem* pChildParticle = static_cast<CParticleItem*>(pParentItem->GetChild(i));
        if (pChildParticle == NULL)
        {
            continue;
        }
        if (wishName == (composedName + pChildParticle->GetName()))
        {
            return pChildParticle;
        }
        CParticleItem* pWishItem = GetChildParticleItem(pChildParticle, composedName + pChildParticle->GetName() + ".", wishName);
        if (pWishItem)
        {
            return pWishItem;
        }
    }

    return NULL;
}

//////////////////////////////////////////////////////////////////////////
void CParticleEffectObject::CFastParticleParser::ExtractParticlePathes(const QString& particlePath)
{
    m_ParticleList.clear();

    XmlNodeRef rootNode = XmlHelpers::LoadXmlFromFile(particlePath.toUtf8().data());

    if (rootNode == NULL)
    {
        return;
    }

    QString rootName;
    rootNode->getAttr("Name", rootName);

    for (int i = 0; i < rootNode->getChildCount(); ++i)
    {
        XmlNodeRef itemNode = rootNode->getChild(i);
        if (itemNode)
        {
            if (itemNode->getTag() && !_stricmp(itemNode->getTag(), "Particles"))
            {
                ParseParticle(itemNode, rootName);
            }
        }
    }
}

void CParticleEffectObject::CFastParticleParser::ExtractLevelParticles()
{
    m_ParticleList.clear();

    if (IEditorParticleManager* pParticleManager = GetIEditor()->GetParticleManager())
    {
        if (IDataBaseLibrary* pLevelLibrary = pParticleManager->FindLibrary("Level"))
        {
            if (CGameEngine* pGameEngine = GetIEditor()->GetGameEngine())
            {
                const QString& levelPath = pGameEngine->GetLevelPath();
                const QString& levelName = pGameEngine->GetLevelName();
                QString newPath = levelPath + "/" + levelName + ".ly";
                QString oldPath = levelPath + "/" + levelName + ".cry";

                ICryPak* pIPak = GetIEditor()->GetSystem()->GetIPak();

                if (pIPak)
                {
                    bool newExtension = false;
                    if (!pIPak->OpenPack(oldPath.toUtf8().data()))
                    {
                        if (pIPak->OpenPack(newPath.toUtf8().data()))
                        {
                            newExtension = true;
                        }
                        else
                        {
                            return;
                        }
                    }

                    QString particleLibrary = levelPath + "/" + "Level.editor_xml";
                    if (XmlNodeRef rootNode = XmlHelpers::LoadXmlFromFile(particleLibrary.toUtf8().data()))
                    {
                        if (XmlNodeRef particleLibrary = rootNode->findChild(pParticleManager->GetRootNodeName().toUtf8().data()))
                        {
                            if (XmlNodeRef levelLibrary = particleLibrary->findChild("LevelLibrary"))
                            {
                                for (int i = 0; i < levelLibrary->getChildCount(); ++i)
                                {
                                    if (XmlNodeRef itemNode = levelLibrary->getChild(i))
                                    {
                                        if (itemNode->getTag() && !_stricmp(itemNode->getTag(), "Particles"))
                                        {
                                            ParseParticle(itemNode, "Level");
                                        }
                                    }
                                }
                            }
                        }
                    }

                    if (newExtension)
                    {
                        pIPak->ClosePack(newPath.toUtf8().data());
                    }
                    else
                    {
                        pIPak->ClosePack(oldPath.toUtf8().data());
                    }
                }
            }
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CParticleEffectObject::CFastParticleParser::ParseParticle(XmlNodeRef& node, const QString& parentPath)
{
    QString particleName;
    node->getAttr("Name", particleName);
    QString particlePath = parentPath + QString(".") + particleName;

    SParticleInfo si;
    si.pathName = particlePath;
    for (int i = 0; i < node->getChildCount(); ++i)
    {
        XmlNodeRef childItemNode = node->getChild(i);

        if (childItemNode == NULL)
        {
            continue;
        }

        if (childItemNode->getTag() && !_stricmp(childItemNode->getTag(), "Childs"))
        {
            si.bHaveChildren = true;
            break;
        }
    }
    m_ParticleList.push_back(si);

    for (int i = 0; i < node->getChildCount(); ++i)
    {
        XmlNodeRef childItemNode = node->getChild(i);

        if (childItemNode == NULL)
        {
            continue;
        }

        if (childItemNode->getTag() && !_stricmp(childItemNode->getTag(), "Childs"))
        {
            for (int k = 0; k < childItemNode->getChildCount(); ++k)
            {
                XmlNodeRef child2ItemNode = childItemNode->getChild(k);

                if (child2ItemNode == NULL)
                {
                    continue;
                }

                if (child2ItemNode->getTag() && !_stricmp(child2ItemNode->getTag(), "Particles"))
                {
                    ParseParticle(child2ItemNode, particlePath);
                }
            }
        }
    }
}

#include <Objects/ParticleEffectObject.moc>