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

#include "stdafx.h"
#include "PanelPreview.h"
#include "Particles/ParticleItem.h"
#include "Objects/ParticleEffectObject.h"
#include <QBoxLayout>

// CPanelPreview dialog

CPanelPreview::CPanelPreview(QWidget* pParent /*=nullptr*/)
    : QWidget(pParent)
    , m_previewCtrl(new CPreviewModelCtrl(this))
{
    QBoxLayout* layout = new QHBoxLayout;
    layout->setMargin(0);
    layout->addWidget(m_previewCtrl);
    setLayout(layout);
}

//////////////////////////////////////////////////////////////////////////
void CPanelPreview::LoadFile(const QString& filename)
{
    if (!filename.isEmpty())
    {
        m_previewCtrl->EnableUpdate(false);
        m_previewCtrl->LoadFile(filename, false);
    }
}

//////////////////////////////////////////////////////////////////////////
static IParticleEffect* FindUpperValidParticleEffect(const QString& effectName)
{
    if (effectName.isEmpty())
    {
        return NULL;
    }

    IParticleEffect* pEffect = gEnv->pParticleManager->FindEffect(effectName.toUtf8().data());
    if (pEffect)
    {
        return pEffect;
    }

    QString upperEffectName(effectName);
    int dotPos = upperEffectName.lastIndexOf('.');
    if (dotPos == -1)
    {
        return NULL;
    }
    upperEffectName.remove(dotPos, upperEffectName.length() - dotPos);

    return FindUpperValidParticleEffect(upperEffectName);
}

//////////////////////////////////////////////////////////////////////////
static IParticleEffect* FindValidParticleEffect(IParticleEffect* pParentEffect, const QString& effectName)
{
    if (pParentEffect == NULL)
    {
        return NULL;
    }

    for (int i = 0; i < pParentEffect->GetChildCount(); ++i)
    {
        IParticleEffect* pItem = pParentEffect->GetChild(i);
        if (effectName == pItem->GetName())
        {
            return pItem;
        }
        pItem = FindValidParticleEffect(pItem, effectName);
        if (pItem)
        {
            return pItem;
        }
    }

    return NULL;
}

//////////////////////////////////////////////////////////////////////////
bool CPanelPreview::LoadParticleEffect(const QString& effectName)
{
    if (effectName.isEmpty())
    {
        return false;
    }

    if (CParticleEffectObject::IsGroup(effectName))
    {
        return false;
    }

    _smart_ptr<IParticleEffect> pEffect = gEnv->pParticleManager->FindEffect(effectName.toUtf8().data());
    if (pEffect == NULL)
    {
        IParticleEffect* pParentEffect = FindUpperValidParticleEffect(effectName);
        if (pParentEffect == NULL)
        {
            return false;
        }
        pEffect = FindValidParticleEffect(pParentEffect, effectName);
        if (pEffect == NULL)
        {
            return false;
        }
    }

    m_previewCtrl->EnableUpdate(true);
    m_previewCtrl->LoadParticleEffect(pEffect);

    return true;
}