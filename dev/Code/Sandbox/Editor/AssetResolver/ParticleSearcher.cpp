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
#include "ParticleSearcher.h"
#include "Particles/ParticleManager.h"
#include "GenericSelectItemDialog.h"
#include <IParticles.h>

////////////////////////////////////////////////////////////////////////////
CParticleSearcher::CParticleSearcher()
{
}

////////////////////////////////////////////////////////////////////////////
CParticleSearcher::~CParticleSearcher()
{
}

////////////////////////////////////////////////////////////////////////////
bool CParticleSearcher::Accept(const char* asset, int& assetTypeId) const
{
    return false;
}

////////////////////////////////////////////////////////////////////////////
bool CParticleSearcher::Accept(const char varType, int& assetTypeId) const
{
    if (varType == IVariable::DT_PARTICLE_EFFECT)
    {
        assetTypeId = 0;
        return true;
    }
    return false;
}

////////////////////////////////////////////////////////////////////////////
QString CParticleSearcher::GetAssetTypeName(int assetTypeId)
{
    assert(assetTypeId == 0);
    return "Particle";
}

////////////////////////////////////////////////////////////////////////////
bool CParticleSearcher::Exists(const char* asset, int assetTypeId) const
{
    assert(assetTypeId == 0);
    return gEnv->pParticleManager->FindEffect(asset, "", false) != NULL;
}

////////////////////////////////////////////////////////////////////////////
bool CParticleSearcher::GetReplacement(QString& replacement, int assetTypeId)
{
    assert(assetTypeId == 0);
    IParticleEffectIteratorPtr iter = gEnv->pParticleManager->GetEffectIterator();
    if (iter->GetCount() == 0)
    {
        return false;
    }

    QStringList items;
    while (IParticleEffect* pEffect = iter->Next())
    {
        string name = pEffect->GetFullName();
        items.push_back(name.c_str());
    }

    CGenericSelectItemDialog gtDlg;
    gtDlg.setWindowTitle(QObject::tr("Select Particle Effect"));
    gtDlg.SetMode(CGenericSelectItemDialog::eMODE_TREE);
    gtDlg.SetTreeSeparator(".");
    gtDlg.SetItems(items);
    int res = gtDlg.exec();
    QString resStr = gtDlg.GetSelectedItem();
    if (res == QDialog::Accepted && !resStr.isEmpty())
    {
        replacement = resStr;
        return true;
    }

    return false;
}

////////////////////////////////////////////////////////////////////////////
void CParticleSearcher::StartSearcher()
{
    gEnv->pParticleManager->LoadLibrary("*", NULL, false);
}

////////////////////////////////////////////////////////////////////////////
void CParticleSearcher::StopSearcher()
{
}

////////////////////////////////////////////////////////////////////////////
IAssetSearcher::TAssetSearchId CParticleSearcher::AddSearch(const char* asset, int assetTypeId)
{
    assert(assetTypeId == 0);

    TAssetSearchId id = GetNextFreeId();
    FindReplacement(asset, m_requests[id]);
    return id;
}

////////////////////////////////////////////////////////////////////////////
void CParticleSearcher::CancelSearch(TAssetSearchId id)
{
    TParticleSearchRequests::iterator it = m_requests.find(id);
    assert(it != m_requests.end());
    m_requests.erase(it);
}

////////////////////////////////////////////////////////////////////////////
bool CParticleSearcher::GetResult(TAssetSearchId id, QStringList& result, bool& doneSearching)
{
    TParticleSearchRequests::iterator it = m_requests.find(id);
    assert(it != m_requests.end());
    bool res = !it->second.empty();
    for (TStringVec::const_iterator it2 = it->second.begin(); it2 != it->second.end(); ++it2)
    {
        result.push_back(*it2);
    }
    doneSearching = true;
    return res;
}

////////////////////////////////////////////////////////////////////////////
void CParticleSearcher::FindReplacement(const char* particleName, TStringVec& res)
{
    QStringList particlePath;
    SplitPath(particleName, particlePath);
    if (particlePath.empty())
    {
        return;
    }

    IParticleEffectIteratorPtr iter = gEnv->pParticleManager->GetEffectIterator();
    std::map<int, TStringVec> sortedRes;
    while (IParticleEffect* pEffect = iter->Next())
    {
        QStringList path;
        SplitPath(pEffect->GetFullName().c_str(), path);
        int r = FindMatching(particlePath, path);
        if (r > 0)
        {
            sortedRes[r].push_back(pEffect->GetFullName().c_str());
        }
    }

    for (std::map<int, TStringVec>::const_reverse_iterator it = sortedRes.rbegin(); it != sortedRes.rend(); ++it)
    {
        for (TStringVec::const_iterator it2 = it->second.begin(); it2 != it->second.end(); ++it2)
        {
            res.push_back(*it2);
        }
    }
}

////////////////////////////////////////////////////////////////////////////
int CParticleSearcher::FindMatching(const TStringVec& search, const TStringVec& sub)
{
    int r = 0;
    const int count = min(search.size(), sub.size());
    for (; r < count && search[r] == sub[r]; ++r)
    {
        ;
    }
    return r;
}

////////////////////////////////////////////////////////////////////////////
void CParticleSearcher::SplitPath(const char* name, QStringList& path)
{
    QString n = QString::fromLatin1(name).toLower();
    int pos = n.lastIndexOf('.');
    while (pos >= 0)
    {
        QString part = n.mid(pos + 1);
        n = n.left(pos);
        if (!part.isEmpty())
        {
            path.push_back(part);
        }
        pos = n.lastIndexOf('.');
    }
    if (!n.isEmpty())
    {
        path.push_back(n);
    }
}

////////////////////////////////////////////////////////////////////////////
IAssetSearcher::TAssetSearchId CParticleSearcher::GetNextFreeId() const
{
    TAssetSearchId id = IAssetSearcher::FIRST_VALID_ID;
    for (TParticleSearchRequests::const_iterator it = m_requests.begin(); it != m_requests.end() && it->first == id; ++it, ++id)
    {
        ;
    }
    return id;
}
////////////////////////////////////////////////////////////////////////////
