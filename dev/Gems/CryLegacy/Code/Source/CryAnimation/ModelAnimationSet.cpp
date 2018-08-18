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

#include "CryLegacy_precompiled.h"
#include "CharacterManager.h"
#include "ModelAnimationSet.h"
#include "Model.h"

const char* strEmpty = "";

//////////////////////////////////////////////////////////////////////////
const char* ModelAnimationHeader::GetFilePath() const
{
    if (m_nAssetType == CAF_File)
    {
        return g_AnimationManager.m_arrGlobalCAF[m_nGlobalAnimId].GetFilePath();
    }
    else if (m_nAssetType == AIM_File)
    {
        return g_AnimationManager.m_arrGlobalAIM[m_nGlobalAnimId].GetFilePath();
    }
    else if (m_nAssetType == LMG_File)
    {
        return g_AnimationManager.m_arrGlobalLMG[m_nGlobalAnimId].GetFilePath();
    }

    CRY_ASSERT(m_nAssetType == CAF_File || m_nAssetType == AIM_File || m_nAssetType == LMG_File);
    return 0;
}

//////////////////////////////////////////////////////////////////////////
CAnimationSet::CAnimationSet(const char* pSkeletonFilePath)
    : m_strSkeletonFilePath(pSkeletonFilePath)
    , m_listeners(1)
{
    m_nRefCounter = 0;
}

CAnimationSet::~CAnimationSet()
{
    ReleaseAnimationData();

    for (CListenerSet<IAnimationSetListener*>::Notifier notifier(m_listeners); notifier.IsValid(); notifier.Next())
    {
        notifier->m_pIAnimationSet = 0;
    }
}


void CAnimationSet::ReleaseAnimationData()
{
    // now there are no controllers referred to by this object, we can release the animations
    uint32 numAnimations = m_arrAnimations.size();
    for (uint32 nAnimId = 0; nAnimId < numAnimations; nAnimId++)
    {
        const ModelAnimationHeader* anim = GetModelAnimationHeader(nAnimId);
        if (anim)
        {
            if (anim->m_nAssetType == CAF_File)
            {
                GlobalAnimationHeaderCAF& rCAF = g_AnimationManager.m_arrGlobalCAF[anim->m_nGlobalAnimId];
                g_AnimationManager.AnimationReleaseCAF(rCAF);
            }
            if (anim->m_nAssetType == AIM_File)
            {
                GlobalAnimationHeaderAIM& rAIM = g_AnimationManager.m_arrGlobalAIM[anim->m_nGlobalAnimId];
                g_AnimationManager.AnimationReleaseAIM(rAIM);
            }
            if (anim->m_nAssetType == LMG_File)
            {
                GlobalAnimationHeaderLMG& rLMG = g_AnimationManager.m_arrGlobalLMG[anim->m_nGlobalAnimId];
                g_AnimationManager.AnimationReleaseLMG(rLMG);
            }
        }
    }
    m_arrAnimations.clear();
    stl::free_container(m_AnimationHashMapKey);
    stl::free_container(m_AnimationHashMapValue);
    m_hashToNameMap.clear();
}




void CAnimationSet::RegisterListener(IAnimationSetListener* pListener)
{
    if (pListener->m_pIAnimationSet == this)
    {
        return;
    }
    if (pListener->m_pIAnimationSet != 0)
    {
        pListener->m_pIAnimationSet->UnregisterListener(pListener);
    }

    m_listeners.Add(pListener);

    pListener->m_pIAnimationSet = this;
}

void CAnimationSet::UnregisterListener(IAnimationSetListener* pListener)
{
    m_listeners.Remove(pListener);
    pListener->m_pIAnimationSet = 0;
}


int CAnimationSet::GetNumFacialAnimations() const
{
    return m_facialAnimations.size();
}

const char* CAnimationSet::GetFacialAnimationPathByName(const char* szName) const
{
    FacialAnimationSet::const_iterator itFacialAnim = std::lower_bound(m_facialAnimations.begin(), m_facialAnimations.end(), szName, stl::less_stricmp<const char*>());
    if (itFacialAnim != m_facialAnimations.end() && stl::less_stricmp<const char*>()(szName, *itFacialAnim))
    {
        itFacialAnim = m_facialAnimations.end();
    }
    const char* szPath = (itFacialAnim != m_facialAnimations.end() ? (*itFacialAnim).path.c_str() : 0);
    return szPath;
}


const char* CAnimationSet::GetFacialAnimationName(int index) const
{
    if (index < 0 || index >= (int)m_facialAnimations.size())
    {
        return 0;
    }
    return m_facialAnimations[index].name.c_str();
}


const char* CAnimationSet::GetFacialAnimationPath(int index) const
{
    if (index < 0 || index >= (int)m_facialAnimations.size())
    {
        return 0;
    }
    return m_facialAnimations[index].path.c_str();
}

void CAnimationSet::PrepareToReuseAnimations(size_t amount)
{
    m_AnimationHashMapKey.reserve(m_AnimationHashMapKey.size() + amount);
    m_AnimationHashMapValue.reserve(m_AnimationHashMapValue.size() + amount);
}

void CAnimationSet::ReuseAnimation(const ModelAnimationHeader& header)
{
    auto it = std::find(m_AnimationHashMapKey.begin(), m_AnimationHashMapKey.end(), header.m_CRC32Name);
    if (it == m_AnimationHashMapKey.end())
    {
        m_arrAnimations.push_back(header);
        if (g_AnimationManager.m_arrGlobalCAF.size() > header.m_nGlobalAnimId)
        {
            GlobalAnimationHeader& globalHeader = g_AnimationManager.m_arrGlobalCAF[header.m_nGlobalAnimId];
            globalHeader.AddNewDynamicControllers(this);
        }
        int nLocalAnimId2 = m_arrAnimations.size() - 1;
        InsertHash(header.m_CRC32Name, nLocalAnimId2);
    }
}

void CAnimationSet::GetNewDynamicControllers(const IDefaultSkeleton* pIDefaultSkeleton, DynArray<uint32>& controllerNames)
{
    for (auto it = m_AnimationDynamicControllers.begin(); it != m_AnimationDynamicControllers.end(); ++it)
    {
        if (!pIDefaultSkeleton->HasDynamicController(*it))
        {
            controllerNames.push_back(*it);
        }
    }
}





//////////////////////////////////////////////////////////////////////////
// Loads animation file. Returns the global anim id of the file, or -1 if error
// SIDE EFFECT NOTES:
//  THis function does not put up a warning in the case the animation couldn't be loaded.
//  It returns an error (false) and the caller must process it.
int CAnimationSet::LoadFileCAF(const char* szFilePath, const char* szAnimName)
{
    //NEW RULE: every single CAF file is now an OnDemand streaming animation
    int nAnimId = GetAnimIDByName (szAnimName);
    if (nAnimId != -1)
    {
        int nGlobalAnimID = m_arrAnimations[nAnimId].m_nGlobalAnimId;
        g_pILog->LogWarning("CryAnimation:: Trying to load animation with alias \"%s\" from file \"%s\" into the animation container. Such animation alias already exists and uses file \"%s\". Please use another animation alias.", szAnimName, szFilePath, g_AnimationManager.m_arrGlobalCAF[nGlobalAnimID].GetFilePath());
        return nGlobalAnimID;
    }

    int nGlobalAnimID = g_AnimationManager.CreateGAH_CAF(szFilePath); //create the global header. the data will be streamed in later.

    ModelAnimationHeader localAnim;
    localAnim.m_nGlobalAnimId = nGlobalAnimID;
    localAnim.m_nAssetType = CAF_File;
    localAnim.SetAnimName(szAnimName);
    m_arrAnimations.push_back(localAnim);

    StoreAnimName(localAnim.m_CRC32Name, szAnimName);

    int nLocalAnimId2 = m_arrAnimations.size() - 1;
    InsertHash(localAnim.m_CRC32Name, nLocalAnimId2);

    GlobalAnimationHeaderCAF& rCAF = g_AnimationManager.m_arrGlobalCAF[nGlobalAnimID];
    rCAF.AddRef();

    uint32 IsCreated = rCAF.IsAssetCreated();
    if (IsCreated == 0)
    {
        //asset is not created, so let's create by loading the CAF
        CRY_ASSERT(rCAF.GetControllersCount() == 0);
        uint8 status = rCAF.LoadCAF();
        if (status)
        {
            rCAF.ClearAssetRequested();
            rCAF.AddNewDynamicControllers(this);
            CRY_ASSERT(rCAF.IsAssetLoaded());
            if (Console::GetInst().ca_UseIMG_CAF)
            {
                g_pISystem->Warning(VALIDATOR_MODULE_ANIMATION, VALIDATOR_WARNING,  VALIDATOR_FLAG_FILE, rCAF.GetFilePath(), "Unnecessary Loading of CAF-files: Probably no valid IMG-file available");
            }
        }
    }
    else
    {
        //this is only for production mode
        if (Console::GetInst().ca_UseIMG_CAF == 0)
        {
            if (rCAF.IsAssetOnDemand() == 0)
            {
                //asset is already in a DBA
                uint8 exist = rCAF.DoesExistCAF();  //check if we also have a CAF-file
                if (exist)
                {
                    //overload the DBA animation.
                    uint8 status = rCAF.LoadCAF();
                    if (status)
                    {
                        rCAF.ClearAssetRequested();
                        CRY_ASSERT(rCAF.IsAssetLoaded());
                        if (Console::GetInst().ca_UseIMG_CAF)
                        {
                            g_pISystem->Warning(VALIDATOR_MODULE_ANIMATION, VALIDATOR_WARNING,  VALIDATOR_FLAG_FILE, rCAF.GetFilePath(), "Unnecessary Loading of CAF-files: Probably no valid IMG-file availbale");
                        }
                    }
                }
            }
        }
    }

#ifdef EDITOR_PCDEBUGCODE
    NotifyListenersAboutFileAdded(szFilePath, szAnimName);
#endif // EDITOR_PCDEBUGCODE

    return nGlobalAnimID;
}

//////////////////////////////////////////////////////////////////////////
// Loads animation file. Returns the global anim id of the file, or -1 if error
// SIDE EFFECT NOTES:
//  THis function does not put up a warning in the case the animation couldn't be loaded.
//  It returns an error (false) and the caller must process it.
int CAnimationSet::LoadFileAIM(const char* szFilePath, const char* szAnimName, const IDefaultSkeleton* pIDefaultSkeleton)
{
    //NEW RULE: every single AIM file is now an OnDemand streaming animation
    int nAnimId = GetAnimIDByName (szAnimName);
    if (nAnimId != -1)
    {
        int nGlobalAnimID = m_arrAnimations[nAnimId].m_nGlobalAnimId;
        g_pILog->LogWarning("CryAnimation:: Trying to load animation with alias \"%s\" from file \"%s\" into the animation container. Such animation alias already exists and uses file \"%s\". Please use another animation alias.", szAnimName, szFilePath, g_AnimationManager.m_arrGlobalAIM[nGlobalAnimID].GetFilePath());
        return nGlobalAnimID;
    }

    int nGlobalAnimID = g_AnimationManager.CreateGAH_AIM(szFilePath);


    ModelAnimationHeader localAnim;
    localAnim.m_nGlobalAnimId = nGlobalAnimID;
    localAnim.m_nAssetType = AIM_File;
    localAnim.SetAnimName(szAnimName);
    m_arrAnimations.push_back(localAnim);

    StoreAnimName(localAnim.m_CRC32Name, szAnimName);

    int nLocalAnimId2 = m_arrAnimations.size() - 1;
    InsertHash(localAnim.m_CRC32Name, nLocalAnimId2);

    GlobalAnimationHeaderAIM& rAIM = g_AnimationManager.m_arrGlobalAIM[nGlobalAnimID];
    rAIM.AddRef();

#ifdef EDITOR_PCDEBUGCODE
    NotifyListenersAboutFileAdded(szFilePath, szAnimName);
#endif // EDITOR_PCDEBUGCODE

    uint32 IsCreated = rAIM.IsAssetCreated();
    if (IsCreated)
    {
        return nGlobalAnimID;
    }

    //----------------------------------------------------------------------------------------

    CRY_ASSERT(rAIM.GetControllersCount() == 0);
    uint8 status = rAIM.LoadAIM();
    if (status)
    {
        rAIM.ProcessPoses(pIDefaultSkeleton, szAnimName);
    }
    else
    {
        //Aim-Pose does not exist as file. this is an error
        if (rAIM.IsAssetNotFound() == 0)
        {
            const char* pPathName = rAIM.GetFilePath();
            g_pISystem->Warning(VALIDATOR_MODULE_ANIMATION, VALIDATOR_WARNING,  VALIDATOR_FLAG_FILE, pPathName,  "Failed to load animation CAF file");
            rAIM.OnAssetNotFound();
        }
    }

    return nGlobalAnimID;
}





//////////////////////////////////////////////////////////////////////////
// Loads animation file. Returns the global anim id of the file, or -1 if error
// SIDE EFFECT NOTES:
//  THis function does not put up a warning in the case the animation couldn't be loaded.
//  It returns an error (false) and the caller must process it.
int CAnimationSet::LoadFileANM(const char* szFileName, const char* szAnimName, DynArray<CControllerTCB>& m_LoadCurrAnimation, CryCGALoader* pCGA, const IDefaultSkeleton* pIDefaultSkeleton)
{
    int nModelAnimId = GetAnimIDByName (szAnimName);
    if (nModelAnimId == -1)
    {
        int nGlobalAnimID = g_AnimationManager.CreateGAH_CAF(szFileName);

        int nLocalAnimId = m_arrAnimations.size();
        ModelAnimationHeader LocalAnim;
        LocalAnim.m_nGlobalAnimId = nGlobalAnimID;
        LocalAnim.m_nAssetType = CAF_File;
        LocalAnim.SetAnimName(szAnimName);
        m_arrAnimations.push_back(LocalAnim);

        StoreAnimName(LocalAnim.m_CRC32Name, szAnimName);

        //m_arrAnimByGlobalId.insert (std::lower_bound(m_arrAnimByGlobalId.begin(), m_arrAnimByGlobalId.end(), nGlobalAnimID, AnimationGlobIdPred(m_arrAnimations)), LocalAnimId(nLocalAnimId));
        InsertHash(LocalAnim.m_CRC32Name, nLocalAnimId);
        //      m_arrAnimByLocalName.insert (std::lower_bound(m_arrAnimByLocalName.begin(), m_arrAnimByLocalName.end(), szAnimName, AnimationNamePred(m_arrAnimations)), nLocalAnimId);

        g_AnimationManager.m_arrGlobalCAF[nGlobalAnimID].AddRef();

        uint32 loaded = g_AnimationManager.m_arrGlobalCAF[nGlobalAnimID].IsAssetLoaded();
        if (loaded)
        {
            // No CRY_ASSERTs on data
            //CRY_ASSERT(g_AnimationManager.m_arrGlobalCAF[nGlobalAnimID].m_arrController.size());
        }
        else
        {
            //CRY_ASSERT(g_AnimationManager.m_arrGlobalCAF[nGlobalAnimID].m_arrController.size()==0);
            //CRY_ASSERT(g_AnimationManager.m_arrGlobalCAF[nGlobalAnimID].m_arrController);
            g_AnimationManager.LoadAnimationTCB(nGlobalAnimID, m_LoadCurrAnimation, pCGA, pIDefaultSkeleton);
        }

#ifdef EDITOR_PCDEBUGCODE
        NotifyListenersAboutFileAdded(szFileName, szAnimName);
#endif // EDITOR_PCDEBUGCODE

        return nGlobalAnimID;
    }
    else
    {
        int nGlobalAnimID = m_arrAnimations[nModelAnimId].m_nGlobalAnimId;
        g_pILog->LogWarning("CryAnimation:: Trying to load animation with alias \"%s\" from file \"%s\" into the animation container. Such animation alias already exists and uses file \"%s\". Please use another animation alias.", szAnimName,    szFileName, g_AnimationManager.m_arrGlobalCAF[nGlobalAnimID].GetFilePath());
        return nGlobalAnimID;
    }
}






//-------------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------------

int CAnimationSet::LoadFileLMG(const char* szFilePath, const char* szAnimName)
{
    int nModelAnimId = GetAnimIDByName (szAnimName);
    if (nModelAnimId == -1)
    {
        int nGlobalAnimID = g_AnimationManager.CreateGAH_LMG(szFilePath);
        int nLocalAnimId = m_arrAnimations.size();

        ModelAnimationHeader LocalAnim;
        LocalAnim.m_nGlobalAnimId = nGlobalAnimID;
        LocalAnim.m_nAssetType = LMG_File;
        LocalAnim.SetAnimName(szAnimName);
        m_arrAnimations.push_back(LocalAnim);

        StoreAnimName(LocalAnim.m_CRC32Name, szAnimName);

        InsertHash(LocalAnim.m_CRC32Name, nLocalAnimId);

        g_AnimationManager.m_arrGlobalLMG[nGlobalAnimID].AddRef();

#ifdef EDITOR_PCDEBUGCODE
        NotifyListenersAboutFileAdded(szFilePath, szAnimName);
#endif // EDITOR_PCDEBUGCODE

        return nGlobalAnimID;
    }
    else
    {
        const ModelAnimationHeader* anim = GetModelAnimationHeader(nModelAnimId);
        if (anim)
        {
            if (anim->m_nAssetType == CAF_File)
            {
                GlobalAnimationHeaderCAF& rCAF = g_AnimationManager.m_arrGlobalCAF[anim->m_nGlobalAnimId];
                g_pILog->LogWarning("CryAnimation:: weird access of CAF \"%s\"",    rCAF.GetFilePath());
                return anim->m_nGlobalAnimId;
            }
            if (anim->m_nAssetType == AIM_File)
            {
                GlobalAnimationHeaderAIM& rAIM = g_AnimationManager.m_arrGlobalAIM[anim->m_nGlobalAnimId];
                g_pILog->LogWarning("CryAnimation:: weird access of AIM \"%s\"",    rAIM.GetFilePath());
                return anim->m_nGlobalAnimId;
            }
            if (anim->m_nAssetType == LMG_File)
            {
                GlobalAnimationHeaderLMG& rLMG = g_AnimationManager.m_arrGlobalLMG[anim->m_nGlobalAnimId];
                g_pILog->LogWarning("CryAnimation:: Trying to load animation with alias \"%s\" from file \"%s\" into the animation container. Such animation alias already exists and uses file \"%s\". Please use another animation alias.", szAnimName, szFilePath,    rLMG.GetFilePath());
                return anim->m_nGlobalAnimId;
            }
        }

        return -1;
    }
}

bool CAnimationSet::RemoveFile(const char* szFilename, uint32& crcOut)
{
#ifdef EDITOR_PCDEBUGCODE // Removal is only valid in editor environments.

    int16 assetType = -1;
    int nGlobalAnimID = g_AnimationManager.GetGlobalIDbyFilePath_LMG(szFilename);

    if (nGlobalAnimID != -1)
    {
        assetType = LMG_File;
    }
    else
    {
        nGlobalAnimID = g_AnimationManager.GetGlobalIDbyFilePath_CAF(szFilename);

        if (nGlobalAnimID != -1)
        {
            assetType = CAF_File;
        }
        else
        {
            nGlobalAnimID = g_AnimationManager.GetGlobalIDbyFilePath_AIM(szFilename);

            if (nGlobalAnimID != -1)
            {
                assetType = AIM_File;
            }
        }
    }

    if (nGlobalAnimID == -1)
    {
        return false;
    }

    for (auto iter = m_arrAnimations.begin(); iter != m_arrAnimations.end(); ++iter)
    {
        if (iter->m_nAssetType == assetType && iter->m_nGlobalAnimId == nGlobalAnimID)
        {
            crcOut = iter->m_CRC32Name;

            // Invalidate hash lookup. We have to invalidate entries as opposed to removing them, since one map
            // stores indices into another map, and we can't invalidate other entries in the animation set.
            DynArray<uint32>::iterator it = std::lower_bound(m_AnimationHashMapKey.begin(), m_AnimationHashMapKey.end(), iter->m_CRC32Name);
            if (it != m_AnimationHashMapKey.end() && *it == iter->m_CRC32Name)
            {
                *it = static_cast<uint32>(-1);
                const size_t index = it - m_AnimationHashMapKey.begin();
                m_AnimationHashMapValue[index] = static_cast<uint16>(-1);
            }

            const AZStd::string animName = iter->GetAnimName();
            m_arrAnimations.erase(iter);
            NotifyListenersAboutFileRemoved(szFilename, iter->GetAnimName());

            return true;
        }
    }

#endif // EDITOR_PCDEBUGCODE

    return false;
}

// prepares to load the specified number of CAFs by reserving the space for the controller pointers
void CAnimationSet::prepareLoadCAFs (uint32 nReserveAnimations)
{
    uint32 numAnims = m_arrAnimations.size();
    if (numAnims)
    {
        CryFatalError("CryAnimation: chrparams file loaded twice");
    }

    m_arrAnimations.reserve (nReserveAnimations);
}


// prepares to load the specified number of CAFs by reserving the space for the controller pointers
void CAnimationSet::prepareLoadANMs (uint32 nReserveAnimations)
{
    nReserveAnimations += m_arrAnimations.size();
    m_arrAnimations.reserve (nReserveAnimations);
}



const ModelAnimationHeader* CAnimationSet::GetModelAnimationHeader(int32 i) const
{
    DEFINE_PROFILER_FUNCTION();

    int32 numAnimation = (int32)m_arrAnimations.size();
    if (numAnimation == 0)
    {
        return 0;
    }

    if (i < 0 || i >= numAnimation)
    {
        if (Console::GetInst().ca_AnimWarningLevel > 0)
        {
            gEnv->pLog->LogError("Invalid Animation ID: %d for Model: %s", i, m_strSkeletonFilePath.c_str());
        }

        return 0;
    }
    return &m_arrAnimations[i];  //the animation-asset exists
}




//----------------------------------------------------------------------------------
// Returns the index of the animation in the set, -1 if there's no such animation
//----------------------------------------------------------------------------------
int CAnimationSet::GetAnimIDByName(const char* szAnimationName) const
{
    uint32 numAnimsInList = m_arrAnimations.size();
    if (numAnimsInList == 0)
    {
        return -1;
    }
    if (szAnimationName == 0)
    {
        return -1;
    }

    return GetValue(szAnimationName);
}

// Returns the given animation name
const char* CAnimationSet::GetNameByAnimID (int nAnimationId) const
{
    if (nAnimationId < 0)
    {
        return "!NEGATIVE ANIMATION ID!";
    }

    int numAnimation = m_arrAnimations.size();
    if (nAnimationId >= numAnimation)
    {
        return "!ANIMATION ID OUT OF RANGE!";
    }

#ifdef STORE_ANIMATION_NAMES
    return m_arrAnimations[nAnimationId].GetAnimName();
#else
    HashToNameMap::const_iterator iter(m_hashToNameMap.find(m_arrAnimations[nAnimationId].m_CRC32Name));
    if (iter != m_hashToNameMap.end())
    {
        return iter->second;
    }
    else
    {
        return m_arrAnimations[nAnimationId].GetAnimName();
    }
#endif
}

//------------------------------------------------------------------------------
int CAnimationSet::GetAnimIDByCRC(uint32 animationCRC) const
{
    size_t value = GetValueCRC(animationCRC);
    return value;
}

uint32 CAnimationSet::GetCRCByAnimID(int nAnimationId) const
{
    if ((nAnimationId >= 0) && (nAnimationId < (int)m_arrAnimations.size()))
    {
        return m_arrAnimations[nAnimationId].m_CRC32Name;
    }
    else
    {
        return 0;
    }
}

uint32 CAnimationSet::GetFilePathCRCByAnimID(int nAnimationId) const
{
    const ModelAnimationHeader* anim = GetModelAnimationHeader(nAnimationId);
    if (anim == 0)
    {
        return 0;
    }

    uint32 GlobalAnimationID            = anim->m_nGlobalAnimId;
    if (anim->m_nAssetType == CAF_File)
    {
        return g_AnimationManager.m_arrGlobalCAF[GlobalAnimationID].GetFilePathCRC32();
    }
    if (anim->m_nAssetType == AIM_File)
    {
        return g_AnimationManager.m_arrGlobalAIM[GlobalAnimationID].GetFilePathCRC32();
    }
    if (anim->m_nAssetType == LMG_File)
    {
        return g_AnimationManager.m_arrGlobalLMG[GlobalAnimationID].GetFilePathCRC32();
    }

    return 0;
}

bool CAnimationSet::IsAnimLoaded(int nAnimationId) const
{
    bool bInMemory = false;
    const ModelAnimationHeader* pAnim = GetModelAnimationHeader(nAnimationId);
    if (pAnim)
    {
        if (pAnim->m_nAssetType == CAF_File)
        {
            int32 nGlobalID = pAnim->m_nGlobalAnimId;
            GlobalAnimationHeaderCAF& rGAHeader = g_AnimationManager.m_arrGlobalCAF[nGlobalID];
            bInMemory = rGAHeader.IsAssetLoaded() != 0;
        }
        else if (pAnim->m_nAssetType == AIM_File)
        {
            int32 nGlobalID = pAnim->m_nGlobalAnimId;
            GlobalAnimationHeaderAIM& rGAHeader = g_AnimationManager.m_arrGlobalAIM[nGlobalID];
            bInMemory = rGAHeader.IsAssetLoaded() != 0;
        }
        else if (pAnim->m_nAssetType == LMG_File)
        {
            int32 nGlobalID = pAnim->m_nGlobalAnimId;
            GlobalAnimationHeaderLMG& rGAHeader = g_AnimationManager.m_arrGlobalLMG[nGlobalID];
            bInMemory = rGAHeader.IsAssetLoaded() != 0;
        }
    }
    return bInMemory;
}

bool CAnimationSet::IsVEGLoaded(int nGlobalVEGId) const
{
    bool bInMemory = false;

    GlobalAnimationHeaderLMG& rLMG = g_AnimationManager.m_arrGlobalLMG[nGlobalVEGId];
    bInMemory = rLMG.IsAssetLoaded() != 0;

    if (rLMG.m_numExamples)
    {
        for (uint32 i = 0; (i < rLMG.m_numExamples) && bInMemory; i++)
        {
            int exampleAnimID = GetAnimIDByCRC(rLMG.m_arrParameter[i].m_animName.m_CRC32);
            if (exampleAnimID >= 0)
            {
                bInMemory = IsAnimLoaded(exampleAnimID);
            }
            else
            {
                bInMemory = false;
            }
        }
    }
    else
    {
        uint32 numBlendSpaces = rLMG.m_arrCombinedBlendSpaces.size();
        for (uint32 bs = 0; (bs < numBlendSpaces) && bInMemory; bs++)
        {
            int32 nSubGlobalID = rLMG.m_arrCombinedBlendSpaces[bs].m_ParaGroupID;
            if (nSubGlobalID >= 0)
            {
                bInMemory = IsVEGLoaded(nSubGlobalID);
            }
            else
            {
                bInMemory = false;
            }
        }
    }

    return bInMemory;
}

static bool AnimTokenMatchesOneOfDirectionalBlends(const CAnimationSet* pAnimationSet, int nAnimationId, const DynArray<DirectionalBlends>& blends)
{
    const ModelAnimationHeader* pAnim = pAnimationSet->GetModelAnimationHeader(nAnimationId);
    if (pAnim && pAnim->m_nAssetType == AIM_File)
    {
        int32 nGlobalID = pAnim->m_nGlobalAnimId;
        uint32 animTokenCRC32 = g_AnimationManager.m_arrGlobalAIM[nGlobalID].m_AnimTokenCRC32;
        size_t numBlends = blends.size();
        for (size_t i = 0; i < numBlends; ++i)
        {
            if (blends[i].m_AnimTokenCRC32 == animTokenCRC32)
            {
                return true;
            }
        }
    }
    return false;
}

bool CAnimationSet::IsAimPose(int nAnimationId, const IDefaultSkeleton& defaultSkeleton) const
{
    return AnimTokenMatchesOneOfDirectionalBlends(this, nAnimationId, static_cast<const CDefaultSkeleton&>(defaultSkeleton).m_poseBlenderAimDesc.m_blends);
}

bool CAnimationSet::IsLookPose(int nAnimationId, const IDefaultSkeleton& defaultSkeleton) const
{
    return AnimTokenMatchesOneOfDirectionalBlends(this, nAnimationId, static_cast<const CDefaultSkeleton&>(defaultSkeleton).m_poseBlenderLookDesc.m_blends);
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
GlobalAnimationHeaderCAF* CAnimationSet::GetGAH_CAF(const char* AnimationName) const
{
    int32 subAnimID = GetAnimIDByName(AnimationName);
    if (subAnimID < 0)
    {
        return 0; //error -> name not found
    }
    return GetGAH_CAF(subAnimID);
}
GlobalAnimationHeaderAIM* CAnimationSet::GetGAH_AIM(const char* AnimationName) const
{
    int32 subAnimID = GetAnimIDByName(AnimationName);
    if (subAnimID < 0)
    {
        return 0; //error -> name not found
    }
    return GetGAH_AIM(subAnimID);
}
GlobalAnimationHeaderLMG* CAnimationSet::GetGAH_LMG(const char* AnimationName) const
{
    int32 subAnimID = GetAnimIDByName(AnimationName);
    if (subAnimID < 0)
    {
        return 0; //error -> name not found
    }
    return GetGAH_LMG(subAnimID);
}




GlobalAnimationHeaderCAF* CAnimationSet::GetGAH_CAF(int nAnimationId) const
{
    int32 numAnimation = (int32)m_arrAnimations.size();
    if ((nAnimationId < 0) || (nAnimationId >= numAnimation))
    {
        gEnv->pLog->LogError("illegal animation index '%d' for Model: %s", nAnimationId, m_strSkeletonFilePath.c_str());
        return NULL;
    }

    const ModelAnimationHeader* anim = GetModelAnimationHeader(nAnimationId);
    if (anim == NULL)
    {
        return NULL;
    }
    if (anim->m_nAssetType == CAF_File)
    {
        return &g_AnimationManager.m_arrGlobalCAF[anim->m_nGlobalAnimId];
    }
    return 0;
}

GlobalAnimationHeaderAIM* CAnimationSet::GetGAH_AIM(int nAnimationId) const
{
    int32 numAnimation = (int32)m_arrAnimations.size();
    if ((nAnimationId < 0) || (nAnimationId >= numAnimation))
    {
        gEnv->pLog->LogError("illegal animation index '%d' for Model: %s", nAnimationId, m_strSkeletonFilePath.c_str());
        return NULL;
    }

    const ModelAnimationHeader* anim = GetModelAnimationHeader(nAnimationId);
    if (anim == NULL)
    {
        return NULL;
    }
    if (anim->m_nAssetType == AIM_File)
    {
        return &g_AnimationManager.m_arrGlobalAIM[anim->m_nGlobalAnimId];
    }
    return 0;
}


GlobalAnimationHeaderLMG* CAnimationSet::GetGAH_LMG(int nAnimationId) const
{
    int32 numAnimation = (int32)m_arrAnimations.size();
    if ((nAnimationId < 0) || (nAnimationId >= numAnimation))
    {
        gEnv->pLog->LogError("illegal animation index '%d' for Model: %s", nAnimationId, m_strSkeletonFilePath.c_str());
        return NULL;
    }

    const ModelAnimationHeader* anim = GetModelAnimationHeader(nAnimationId);
    if (anim == NULL)
    {
        return NULL;
    }
    if (anim->m_nAssetType == LMG_File)
    {
        return &g_AnimationManager.m_arrGlobalLMG[anim->m_nGlobalAnimId];
    }
    return 0;
}




//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
f32 CAnimationSet::GetDuration_sec(int nAnimationId) const
{
    const f32 INVALID_DURATION = 0.0f;

    int32 numAnimation = (int32)m_arrAnimations.size();
    if (nAnimationId < 0 || nAnimationId >= numAnimation)
    {
        if (Console::GetInst().ca_AnimWarningLevel > 0)
        {
            gEnv->pLog->LogError("illegal animation index used in function GetDuration_sec'%d' for Model: %s", nAnimationId, m_strSkeletonFilePath.c_str());
        }
        return INVALID_DURATION;
    }

    const ModelAnimationHeader* anim = GetModelAnimationHeader(nAnimationId);
    if (anim == 0)
    {
        return INVALID_DURATION;
    }

    if (anim->m_nAssetType == CAF_File)
    {
        uint32 GlobalAnimationID = anim->m_nGlobalAnimId;
        GlobalAnimationHeaderCAF& rGlobalAnimHeader = g_AnimationManager.m_arrGlobalCAF[GlobalAnimationID];
        f32   fDuration = rGlobalAnimHeader.m_fEndSec - rGlobalAnimHeader.m_fStartSec;
        return fDuration;
    }

    if (anim->m_nAssetType == AIM_File)
    {
        uint32 GlobalAnimationID = anim->m_nGlobalAnimId;
        GlobalAnimationHeaderAIM& rGlobalAnimHeader = g_AnimationManager.m_arrGlobalAIM[GlobalAnimationID];
        f32   fDuration = rGlobalAnimHeader.m_fEndSec - rGlobalAnimHeader.m_fStartSec;
        return fDuration;
    }

    if (anim->m_nAssetType == LMG_File)
    {
        uint32 GlobalAnimationID = anim->m_nGlobalAnimId;
        GlobalAnimationHeaderLMG& rGlobalAnimHeader = g_AnimationManager.m_arrGlobalLMG[GlobalAnimationID];
        uint32 lmg = rGlobalAnimHeader.IsAssetLMG();
        CRY_ASSERT(lmg);
        if (rGlobalAnimHeader.IsAssetLMGValid() == 0)
        {
            return 0;
        }
        if (rGlobalAnimHeader.IsAssetInternalType())
        {
            return 1.0f; //there is no reliable way to compute the duration.
        }
        const uint32 numBlendSpaces = rGlobalAnimHeader.m_arrCombinedBlendSpaces.size();
        if (numBlendSpaces > 0)
        {
            f32 fDuration = 0.0f;
            f32 totalExamples = 0.0f;
            for (uint32 i = 0; i < numBlendSpaces; i++)
            {
                int32 bspaceID = rGlobalAnimHeader.m_arrCombinedBlendSpaces[i].m_ParaGroupID;
                if (bspaceID < 0)
                {
                    return INVALID_DURATION;
                }

                GlobalAnimationHeaderLMG& rGlobalAnimHeaderBS = g_AnimationManager.m_arrGlobalLMG[bspaceID];

                const uint32 numBS = rGlobalAnimHeaderBS.m_numExamples;
                for (uint32 e = 0; e < numBS; e++)
                {
                    int32 aid = GetAnimIDByCRC(rGlobalAnimHeaderBS.m_arrParameter[e].m_animName.m_CRC32);
                    CRY_ASSERT(aid >= 0);
                    fDuration += GetDuration_sec(aid);
                    totalExamples += 1.0f;
                }
            }
            return (totalExamples > 0) ? fDuration / totalExamples : INVALID_DURATION;
        }
        else
        {
            uint32 numBS = rGlobalAnimHeader.m_numExamples;
            if (numBS == 0)
            {
                return INVALID_DURATION;
            }
            f32 fDuration = 0;
            for (uint32 i = 0; i < numBS; i++)
            {
                int32 aid = GetAnimIDByCRC(rGlobalAnimHeader.m_arrParameter[i].m_animName.m_CRC32);
                CRY_ASSERT(aid >= 0);
                fDuration += GetDuration_sec(aid);
            }
            return fDuration / numBS;
        }
    }

    return 0;
}

uint32 CAnimationSet::GetAnimationFlags(int nAnimationId) const
{
    int32 numAnimation = (int32)m_arrAnimations.size();
    if (nAnimationId < 0 || nAnimationId >= numAnimation)
    {
        if (Console::GetInst().ca_AnimWarningLevel > 0)
        {
            gEnv->pLog->LogError("illegal animation index used in function GetAnimationFlags '%d' for Model: %s", nAnimationId, m_strSkeletonFilePath.c_str());
        }
        return 0;
    }
    const ModelAnimationHeader* pAnimHeader = GetModelAnimationHeader(nAnimationId);
    if (pAnimHeader == 0)
    {
        return 0;
    }

    uint32 nGlobalAnimationID           = pAnimHeader->m_nGlobalAnimId;

    if (pAnimHeader->m_nAssetType == CAF_File)
    {
        if (g_AnimationManager.m_arrGlobalCAF[nGlobalAnimationID].IsAssetOnDemand())
        {
            g_AnimationManager.m_arrGlobalCAF[nGlobalAnimationID].m_nFlags |= CA_ASSET_ONDEMAND;
        }
        return g_AnimationManager.m_arrGlobalCAF[nGlobalAnimationID].m_nFlags;
    }
    if (pAnimHeader->m_nAssetType == AIM_File)
    {
        return g_AnimationManager.m_arrGlobalAIM[nGlobalAnimationID].m_nFlags;
    }
    if (pAnimHeader->m_nAssetType == LMG_File)
    {
        return g_AnimationManager.m_arrGlobalLMG[nGlobalAnimationID].m_nFlags;
    }

    return 0;
}

const char* CAnimationSet::GetAnimationStatus(int nAnimationId) const
{
    int32 numAnimation = (int32)m_arrAnimations.size();
    if (nAnimationId < 0 || nAnimationId >= numAnimation)
    {
        if (Console::GetInst().ca_AnimWarningLevel > 0)
        {
            gEnv->pLog->LogError("illegal animation index used in function GetAnimationFlags '%d' for Model: %s", nAnimationId, m_strSkeletonFilePath.c_str());
        }
        return 0;
    }
    const ModelAnimationHeader* pAnimHeader = GetModelAnimationHeader(nAnimationId);
    if (pAnimHeader == 0)
    {
        return 0;
    }

    uint32 nGlobalAnimationID           = pAnimHeader->m_nGlobalAnimId;

    if (pAnimHeader->m_nAssetType == CAF_File)
    {
        return 0;
    }
    if (pAnimHeader->m_nAssetType == AIM_File)
    {
        return 0;
    }
    if (pAnimHeader->m_nAssetType == LMG_File)
    {
        return g_AnimationManager.m_arrGlobalLMG[nGlobalAnimationID].m_Status.c_str();
    }

    return 0;
}



void CAnimationSet::AddRef(const int32 nAnimationId) const
{
    int32 numAnimation = (int32)m_arrAnimations.size();
    if (nAnimationId < 0 || nAnimationId >= numAnimation)
    {
        if (Console::GetInst().ca_AnimWarningLevel > 0)
        {
            gEnv->pLog->LogError("illegal animation index used in function AddRef '%d' for Model: %s", nAnimationId, m_strSkeletonFilePath.c_str());
        }
        return;
    }

    const ModelAnimationHeader* pAnimHeader = GetModelAnimationHeader(nAnimationId);
    if (pAnimHeader == 0)
    {
        return;
    }

    uint32 nGlobalAnimationID           = pAnimHeader->m_nGlobalAnimId;
    if (pAnimHeader->m_nAssetType == CAF_File)
    {
        g_pCharacterManager->CAF_AddRefByGlobalId(nGlobalAnimationID);
    }
    else if (pAnimHeader->m_nAssetType == LMG_File)
    {
        GlobalAnimationHeaderLMG& rLMG = g_AnimationManager.m_arrGlobalLMG[nGlobalAnimationID];

        rLMG.AddRef();
        if (rLMG.m_numExamples)
        {
            for (uint32 i = 0; i < rLMG.m_numExamples; i++)
            {
                int exampleAnimID = GetAnimIDByCRC(rLMG.m_arrParameter[i].m_animName.m_CRC32);
                if (exampleAnimID >= 0)
                {
                    const ModelAnimationHeader* pExampleAnimHeader = GetModelAnimationHeader(exampleAnimID);
                    g_pCharacterManager->CAF_AddRefByGlobalId(pExampleAnimHeader->m_nGlobalAnimId);
                }
            }
        }
        else
        {
            uint32 totalExamples = 0;

            uint32 numBlendSpaces = rLMG.m_arrCombinedBlendSpaces.size();
            for (uint32 bs = 0; bs < numBlendSpaces; bs++)
            {
                int32 nGlobalID = rLMG.m_arrCombinedBlendSpaces[bs].m_ParaGroupID;
                if (nGlobalID < 0)
                {
                    continue;
                }


                GlobalAnimationHeaderLMG& rsubLMG = g_AnimationManager.m_arrGlobalLMG[nGlobalID];

                for (uint32 i = 0; i < rsubLMG.m_numExamples; i++)
                {
                    int exampleAnimID = GetAnimIDByCRC(rsubLMG.m_arrParameter[i].m_animName.m_CRC32);
                    if (exampleAnimID >= 0)
                    {
                        const ModelAnimationHeader* pExampleAnimHeader = GetModelAnimationHeader(exampleAnimID);
                        g_pCharacterManager->CAF_AddRefByGlobalId(pExampleAnimHeader->m_nGlobalAnimId);
                    }
                }
            }
        }
    }
}

void CAnimationSet::Release(const int32 nAnimationId) const
{
    int32 numAnimation = (int32)m_arrAnimations.size();
    if (nAnimationId < 0 || nAnimationId >= numAnimation)
    {
        if (Console::GetInst().ca_AnimWarningLevel > 0)
        {
            gEnv->pLog->LogError("illegal animation index used in function AddRef '%d' for Model: %s", nAnimationId, m_strSkeletonFilePath.c_str());
        }
        return;
    }

    const ModelAnimationHeader* pAnimHeader = GetModelAnimationHeader(nAnimationId);
    if (pAnimHeader == 0)
    {
        return;
    }

    uint32 nGlobalAnimationID           = pAnimHeader->m_nGlobalAnimId;
    if (pAnimHeader->m_nAssetType == CAF_File)
    {
        g_pCharacterManager->CAF_ReleaseByGlobalId(nGlobalAnimationID);
    }
    else if (pAnimHeader->m_nAssetType == LMG_File)
    {
        GlobalAnimationHeaderLMG& rLMG = g_AnimationManager.m_arrGlobalLMG[nGlobalAnimationID];

        if (rLMG.m_numExamples)
        {
            for (uint32 i = 0; i < rLMG.m_numExamples; i++)
            {
                int exampleAnimID = GetAnimIDByCRC(rLMG.m_arrParameter[i].m_animName.m_CRC32);
                if (exampleAnimID >= 0)
                {
                    const ModelAnimationHeader* pExampleAnimHeader = GetModelAnimationHeader(exampleAnimID);
                    g_pCharacterManager->CAF_ReleaseByGlobalId(pExampleAnimHeader->m_nGlobalAnimId);
                }
            }
        }
        else
        {
            uint32 totalExamples = 0;

            uint32 numBlendSpaces = rLMG.m_arrCombinedBlendSpaces.size();
            for (uint32 bs = 0; bs < numBlendSpaces; bs++)
            {
                int32 nGlobalID = rLMG.m_arrCombinedBlendSpaces[bs].m_ParaGroupID;
                if (nGlobalID >= 0)
                {
                    GlobalAnimationHeaderLMG& rsubLMG = g_AnimationManager.m_arrGlobalLMG[nGlobalID];

                    for (uint32 i = 0; i < rsubLMG.m_numExamples; i++)
                    {
                        int exampleAnimID = GetAnimIDByCRC(rsubLMG.m_arrParameter[i].m_animName.m_CRC32);
                        if (exampleAnimID >= 0)
                        {
                            const ModelAnimationHeader* pExampleAnimHeader = GetModelAnimationHeader(exampleAnimID);
                            g_pCharacterManager->CAF_ReleaseByGlobalId(pExampleAnimHeader->m_nGlobalAnimId);
                        }
                    }
                }
            }
        }
        rLMG.Release();
    }
}




bool CAnimationSet::GetAnimationDCCWorldSpaceLocation(const char* szAnimationName, QuatT& startLocation) const
{
    int32 AnimID = GetAnimIDByName(szAnimationName);
    return GetAnimationDCCWorldSpaceLocation(AnimID, startLocation);
}

bool CAnimationSet::GetAnimationDCCWorldSpaceLocation(int32 AnimID, QuatT& startLocation) const
{
    startLocation.SetIdentity();
    if (AnimID < 0)
    {
        if (Console::GetInst().ca_AnimWarningLevel > 0)
        {
            gEnv->pLog->LogError("animation-name not in chrparams file: %s for Model: %s", GetNameByAnimID(AnimID), m_strSkeletonFilePath.c_str());
        }
        return false;
    }
    const ModelAnimationHeader* anim = GetModelAnimationHeader(AnimID);
    if ((anim == 0) || (anim->m_nAssetType != CAF_File))
    {
        return false;
    }
    uint32 GlobalAnimationID = anim->m_nGlobalAnimId;
    GlobalAnimationHeaderCAF& rGlobalAnimHeader = g_AnimationManager.m_arrGlobalCAF[GlobalAnimationID];
    if (rGlobalAnimHeader.IsAssetCreated() == 0)
    {
        return false;
    }
    startLocation = rGlobalAnimHeader.m_StartLocation;
    return true;
}

bool CAnimationSet::GetAnimationDCCWorldSpaceLocation(const CAnimation* pAnim, QuatT& startLocation, uint32 nControllerID) const
{
    startLocation.SetIdentity();
    if (!pAnim)
    {
        return false;
    }

    int16 animID = pAnim->GetAnimationId();
    if (animID < 0)
    {
        if (Console::GetInst().ca_AnimWarningLevel > 0)
        {
            gEnv->pLog->LogError("animation-name not in chrparams file: %s for Model: %s", GetNameByAnimID(animID), m_strSkeletonFilePath.c_str());
        }
        return false;
    }

    const ModelAnimationHeader* pAnimHeader = GetModelAnimationHeader(animID);
    if (!pAnimHeader || pAnimHeader->m_nAssetType != CAF_File)
    {
        return false;
    }

    GlobalAnimationHeaderCAF& rCAF = g_AnimationManager.m_arrGlobalCAF[pAnimHeader->m_nGlobalAnimId];
    if (rCAF.IsAssetCreated() == 0)
    {
        return false;
    }

    startLocation = rCAF.m_StartLocation;

    if (rCAF.IsAssetLoaded() == 0)
    {
        return true; // the startLocation is already valid at this point
    }
    const IController* pController = rCAF.GetControllerByJointCRC32(nControllerID);
    CRY_ASSERT(pController);
    if (!pController)
    {
        return true;
    }

    QuatT key0(IDENTITY);
    QuatT key1(IDENTITY);
    const f32 fKeyTime0 = rCAF.NTime2KTime(0);
    pController->GetOP(fKeyTime0, key0.q, key0.t);  //the first key should be always identity, except in the case of raw-assets
    const float ntime = rCAF.GetNTimeforEntireClip(pAnim->GetCurrentSegmentIndex(), pAnim->GetCurrentSegmentNormalizedTime());

#ifndef _RELEASE
    // Debug code to try and figure out rare crash bug DT-2674, which crashes on a floating point error during the second GetOP call
    {
        if (!NumberValid(ntime))
        {
            static float lastntime = 0.0f;
            lastntime = ntime;
            CryFatalError("CAnimationSet::GetAnimationDCCWorldSpaceLocation: Invalid nTime");
        }
    }
#endif

    const f32 fKeyTime1 = rCAF.NTime2KTime(ntime);

#ifndef _RELEASE
    // Debug code to try and figure out rare crash bug DT-2674, which crashes on a floating point error during the second GetOP call
    {
        if (!NumberValid(fKeyTime1))
        {
            static float lastfKeyTime1 = 0.0f;
            lastfKeyTime1 = fKeyTime1;
            CryFatalError("CAnimationSet::GetAnimationDCCWorldSpaceLocation: Invalid fKeyTime1");
        }
    }
#endif

    pController->GetOP(fKeyTime1, key1.q, key1.t);
    const QuatT animationMovement = key0.GetInverted() * key1;
    startLocation = startLocation * animationMovement;
    return true;
}


CAnimationSet::ESampleResult CAnimationSet::SampleAnimation(const int32 animationId, const float animationNormalizedTime, const uint32 controllerId, QuatT& relativeLocationOutput) const
{
    relativeLocationOutput.SetIdentity();

    const ModelAnimationHeader* pModelAnimationHeader = GetModelAnimationHeader(animationId);
    if (!pModelAnimationHeader)
    {
        return eSR_InvalidAnimationId;
    }

    if (pModelAnimationHeader->m_nAssetType != CAF_File)
    {
        return eSR_UnsupportedAssetType;
    }

    const GlobalAnimationHeaderCAF& globalAnimationHeader = g_AnimationManager.m_arrGlobalCAF[pModelAnimationHeader->m_nGlobalAnimId];
    if (!globalAnimationHeader.IsAssetCreated() || !globalAnimationHeader.IsAssetLoaded())
    {
        return eSR_NotInMemory;
    }

    const IController* pController = globalAnimationHeader.GetControllerByJointCRC32(controllerId);
    if (!pController)
    {
        return eSR_ControllerNotFound;
    }

    pController->GetOP(globalAnimationHeader.NTime2KTime(animationNormalizedTime), relativeLocationOutput.q, relativeLocationOutput.t);

    return eSR_Success;
}


const char* CAnimationSet::GetFilePathByName (const char* szAnimationName) const
{
    int32 AnimID = GetAnimIDByName(szAnimationName);
    if (AnimID < 0)
    {
        gEnv->pLog->LogError("animation-name not in chrparams file: %s for Model: %s", szAnimationName, m_strSkeletonFilePath.c_str());
        return 0;
    }
    const ModelAnimationHeader* anim = GetModelAnimationHeader(AnimID);
    if (anim == 0)
    {
        return 0;
    }
    return anim->GetFilePath();
};

const char* CAnimationSet::GetFilePathByID(int nAnimationId) const
{
    const ModelAnimationHeader* anim = GetModelAnimationHeader(nAnimationId);
    if (anim == 0)
    {
        return 0;
    }
    return anim->GetFilePath();
};





int32 CAnimationSet::GetGlobalIDByName(const char* szAnimationName) const
{
    int32 AnimID = GetAnimIDByName(szAnimationName);
    if (AnimID < 0)
    {
        gEnv->pLog->LogError("CryAnimation. animation-name not in chrparams file: %s for Model: %s", szAnimationName, m_strSkeletonFilePath.c_str());
        return -1;
    }
    const ModelAnimationHeader* anim = GetModelAnimationHeader(AnimID);
    if (anim == 0)
    {
        return -1;
    }
    return (int32)anim->m_nGlobalAnimId;
}

int32 CAnimationSet::GetGlobalIDByAnimID(int nAnimationId) const
{
    const ModelAnimationHeader* anim = GetModelAnimationHeader(nAnimationId);
    if (anim == 0)
    {
        return -1;
    }
    return (int32)anim->m_nGlobalAnimId;
}




uint32 CAnimationSet::GetAnimationSize(const uint32 nAnimationId) const
{
    CRY_ASSERT(nAnimationId >= 0);
    CRY_ASSERT((int)nAnimationId < m_arrAnimations.size());
    const ModelAnimationHeader& header = m_arrAnimations[nAnimationId];

    int32 globalID = header.m_nGlobalAnimId;
    if (globalID < 0)
    {
        return 0;
    }

    switch (header.m_nAssetType)
    {
    case LMG_File:
        return g_AnimationManager.m_arrGlobalLMG[globalID].SizeOfLMG();
    case CAF_File:
        return g_AnimationManager.m_arrGlobalCAF[globalID].SizeOfCAF(true);
    default:
        return 0;
    }
}


const char* CAnimationSet::GetDBAFilePath(const uint32 nAnimationId) const
{
    CRY_ASSERT(nAnimationId >= 0);
    CRY_ASSERT((int)nAnimationId < m_arrAnimations.size());
    const ModelAnimationHeader& header = m_arrAnimations[nAnimationId];

    int32 globalID = header.m_nGlobalAnimId;
    if (header.m_nAssetType == CAF_File)
    {
        CRY_ASSERT(globalID >= 0);
        CRY_ASSERT(globalID < g_AnimationManager.m_arrGlobalCAF.size());
        return g_pCharacterManager->GetDBAFilePathByGlobalID(globalID);
    }
    else
    {
        return NULL;
    }
}




#ifdef EDITOR_PCDEBUGCODE
uint32 CAnimationSet::GetTotalPosKeys(const uint32 nAnimationId) const
{
    CRY_ASSERT(nAnimationId >= 0);
    CRY_ASSERT((int)nAnimationId < m_arrAnimations.size());
    const ModelAnimationHeader& header = m_arrAnimations[nAnimationId];

    int32 globalID = header.m_nGlobalAnimId;

    if (header.m_nAssetType == CAF_File)
    {
        CRY_ASSERT(globalID >= 0);
        CRY_ASSERT(globalID < g_AnimationManager.m_arrGlobalCAF.size());
        return g_AnimationManager.m_arrGlobalCAF[globalID].GetTotalPosKeys();
    }
    return 0;
}

uint32 CAnimationSet::GetTotalRotKeys(const uint32 nAnimationId) const
{
    CRY_ASSERT(nAnimationId >= 0);
    CRY_ASSERT((int)nAnimationId < m_arrAnimations.size());
    const ModelAnimationHeader& header = m_arrAnimations[nAnimationId];

    int32 globalID = header.m_nGlobalAnimId;

    if (header.m_nAssetType == CAF_File)
    {
        CRY_ASSERT(globalID >= 0);
        CRY_ASSERT(globalID < g_AnimationManager.m_arrGlobalCAF.size());
        return g_AnimationManager.m_arrGlobalCAF[globalID].GetTotalRotKeys();
    }
    return 0;
}

void CAnimationSet::RebuildAimHeader(const char* szAnimationName, const IDefaultSkeleton* pIDefaultSkeleton)
{
    const int32 animationId = GetAnimIDByName(szAnimationName);
    const ModelAnimationHeader* pAnim = GetModelAnimationHeader(animationId);
    if (pAnim->m_nAssetType != AIM_File)
    {
        return;
    }

    GlobalAnimationHeaderAIM& aimHeader = g_AnimationManager.m_arrGlobalAIM[ pAnim->m_nGlobalAnimId ];
    const int32 loadSuccess = aimHeader.LoadAIM();
    if (loadSuccess == 0)
    {
        return;
    }

    aimHeader.ProcessPoses(pIDefaultSkeleton, szAnimationName);
}
#endif

int CAnimationSet::AddAnimationByPath(const char* animationName, const char* animationPath, const IDefaultSkeleton* pIDefaultSkeleton)
{
    {
        const int localAnimationId = GetAnimIDByName(animationName);
        if (localAnimationId != -1)
        {
            return localAnimationId;
        }
    }

    const char* fileExtension = PathUtil::GetExt(animationPath);

    const bool isCaf = (_stricmp(fileExtension, "caf") == 0);
    const bool isLmg = (_stricmp(fileExtension, "lmg") == 0) || (_stricmp(fileExtension, "bspace") == 0) || (_stricmp(fileExtension, "comb") == 0);

    const bool isValidAnimationExtension = (isCaf || isLmg);
    if (!isValidAnimationExtension)
    {
        return -1;
    }

    int globalAnimationId = -1;
    if (isCaf)
    {
        const bool isAim = false;

        if (isAim)
        {
            globalAnimationId = LoadFileAIM(animationPath, animationName, pIDefaultSkeleton);
        }
        else
        {
            globalAnimationId = LoadFileCAF(animationPath, animationName);
        }
    }
    else if(isLmg)
    {
        globalAnimationId = LoadFileLMG(animationPath, animationName);
    }

    if (globalAnimationId == -1)
    {
        return -1;
    }

    NotifyListenersAboutFileAdded(animationPath, animationName);

    const int localAnimationId = GetAnimIDByName(animationName);
    return localAnimationId;
}

void CAnimationSet::NotifyListenersAboutReload()
{
    for (CListenerSet<IAnimationSetListener*>::Notifier notifier(m_listeners); notifier.IsValid(); notifier.Next())
    {
        notifier->OnAnimationSetReload();
    }
}

void CAnimationSet::NotifyListenersAboutFileAdded(const char* szFilename, const char* szAnimName)
{
    for (CListenerSet<IAnimationSetListener*>::Notifier notifier(m_listeners); notifier.IsValid(); notifier.Next())
    {
        notifier->OnAnimationSetAddAnimation(szFilename, szAnimName);
    }
}

void CAnimationSet::NotifyListenersAboutFileRemoved(const char* szFilename, const char* szAnimName)
{
    for (CListenerSet<IAnimationSetListener*>::Notifier notifier(m_listeners); notifier.IsValid(); notifier.Next())
    {
        notifier->OnAnimationSetRemovedAnimation(szFilename, szAnimName);
    }
}



//----------------------------------------------------------------------------------
//----      check if all animation-assets in a locomotion group are valid       ----
//----------------------------------------------------------------------------------
void CAnimationSet::VerifyLMGs()
{
    uint32 numAnimNames = m_arrAnimations.size();
    for (uint32 i = 0; i < numAnimNames; i++)
    {
        //const char* pAnimName = m_arrAnimations[i].m_Name;
        if (m_arrAnimations[i].m_nAssetType != LMG_File)
        {
            continue;
        }
        uint32 GlobalAnimationID = m_arrAnimations[i].m_nGlobalAnimId;
        GlobalAnimationHeaderLMG& rLMG = g_AnimationManager.m_arrGlobalLMG[GlobalAnimationID];
        uint32 IsCreated = rLMG.IsAssetCreated();//Loaded();
        if (IsCreated)
        {
            continue;
        }
        const char* strFilePath = rLMG.GetFilePath();
        const char* szExt = PathUtil::GetExt(strFilePath);
        uint32 IsLMG = (strcmp(szExt, "lmg") == 0) || (strcmp(szExt, "bspace") == 0) || (strcmp(szExt, "comb") == 0);
        if (IsLMG == 0)
        {
            continue;
        }
        rLMG.OnAssetLMG();
        rLMG.LoadAndParseXML(this);
    }
}

size_t CAnimationSet::SizeOfAnimationSet() const
{
    size_t nSize = sizeof(CAnimationSet);

    nSize += m_arrAnimations.get_alloc_size();
    uint32 numAnimations = m_arrAnimations.size();
    for (uint32 i = 0; i < numAnimations; i++)
    {
        nSize += m_arrAnimations[i].SizeOfAnimationHeader();
    }

    nSize += m_AnimationHashMapKey.get_alloc_size();
    nSize += m_AnimationHashMapValue.get_alloc_size();

    for (FacialAnimationSet::const_iterator it = m_facialAnimations.begin(), end = m_facialAnimations.end(); it != end; ++it)
    {
        nSize += it->name.capacity();
        nSize += it->path.capacity();
    }

    for (HashToNameMap::const_iterator iter(m_hashToNameMap.begin()), end(m_hashToNameMap.end()); iter != end; ++iter)
    {
        nSize += sizeof(HashToNameMap::key_type);
        nSize += iter->second.capacity();
    }

    return nSize;
}

void CAnimationSet::GetMemoryUsage(ICrySizer* pSizer) const
{
    pSizer->AddObject(m_arrAnimations);
    pSizer->AddObject(m_AnimationHashMapKey);
    pSizer->AddObject(m_AnimationHashMapValue);
    pSizer->AddObject(m_facialAnimations);
    pSizer->AddObject(m_hashToNameMap);
}


#ifdef EDITOR_PCDEBUGCODE
void CAnimationSet::GetSubAnimations(DynArray<int>& animIdsOut, const int animId) const
{
    const ModelAnimationHeader* pHeader = GetModelAnimationHeader(animId);
    if (!pHeader)
    {
        return;
    }

    if (pHeader->m_nAssetType != LMG_File)
    {
        return;
    }

    const GlobalAnimationHeaderLMG& bspaceHeader = g_AnimationManager.m_arrGlobalLMG[pHeader->m_nGlobalAnimId];
    assert(bspaceHeader.IsAssetLMG());

    for (uint32 i = 0; i < bspaceHeader.m_numExamples; ++i)
    {
        const BSParameter& parameter = bspaceHeader.m_arrParameter[i];
        const int exampleAnimId = GetAnimIDByCRC(parameter.m_animName.m_CRC32);
        animIdsOut.push_back(exampleAnimId);
    }

    int numCombinedBlendspaces = bspaceHeader.m_arrCombinedBlendSpaces.size();
    for (int i = 0; i < numCombinedBlendspaces; ++i)
    {
        const int32 bspaceId = bspaceHeader.m_arrCombinedBlendSpaces[i].m_ParaGroupID;
        if (bspaceId < 0)
        {
            continue;
        }

        const GlobalAnimationHeaderLMG& subBspaceHeader = g_AnimationManager.m_arrGlobalLMG[bspaceId];
        for (uint32 e = 0; e < subBspaceHeader.m_numExamples; ++e)
        {
            const BSParameter& parameter = subBspaceHeader.m_arrParameter[e];
            const int exampleAnimId = GetAnimIDByCRC(parameter.m_animName.m_CRC32);
            animIdsOut.push_back(exampleAnimId);
        }
    }
}

bool CAnimationSet::IsCombinedBlendSpace(int nAnimationId) const
{
    const ModelAnimationHeader* pHeader = GetModelAnimationHeader(nAnimationId);
    if (!pHeader)
    {
        return false;
    }

    if (pHeader->m_nAssetType != LMG_File)
    {
        return false;
    }

    size_t headerIndex = pHeader->m_nGlobalAnimId;
    if (headerIndex >= g_AnimationManager.m_arrGlobalLMG.size())
    {
        return false;
    }

    GlobalAnimationHeaderLMG& bspaceHeader = g_AnimationManager.m_arrGlobalLMG[headerIndex];
    if (!bspaceHeader.IsAssetLMG())
    {
        return false;
    }

    return bspaceHeader.m_arrCombinedBlendSpaces.size() != 0;
}

bool CAnimationSet::GetMotionParameters(uint32 nAnimationId, int32 exampleIndex, IDefaultSkeleton* pDefaultSkeleton, Vec4& outParameters) const
{
    const ModelAnimationHeader* pHeader = GetModelAnimationHeader(nAnimationId);
    if (!pHeader)
    {
        return false;
    }

    if (pHeader->m_nAssetType != LMG_File)
    {
        return false;
    }

    size_t headerIndex = pHeader->m_nGlobalAnimId;
    if (headerIndex >= g_AnimationManager.m_arrGlobalLMG.size())
    {
        return false;
    }

    GlobalAnimationHeaderLMG& bspaceHeader = g_AnimationManager.m_arrGlobalLMG[headerIndex];
    if (!bspaceHeader.IsAssetLMG())
    {
        return false;
    }

    if (exampleIndex >= bspaceHeader.m_arrParameter.size())
    {
        return false;
    }

    size_t numExamples = bspaceHeader.m_arrParameter.size();
    for (int i = 0; i < numExamples; ++i)
    {
        int32 animID = GetAnimIDByCRC(bspaceHeader.m_arrParameter[i].m_animName.m_CRC32);
        if (animID < 0)
        {
            return false;
        }
        int32 globalID = GetGlobalIDByAnimID_Fast(animID);
        if (globalID < 0)
        {
            return false;
        }
        GlobalAnimationHeaderCAF& rCAF = g_AnimationManager.m_arrGlobalCAF[globalID];
        if (rCAF.IsAssetLoaded() == 0 && rCAF.IsAssetRequested() == 0)
        {
            rCAF.LoadCAF();
        }
        if (rCAF.IsAssetLoaded() == 0) // possible in DBA case
        {
            return false;
        }
    }

    for (int i = 0; i < bspaceHeader.m_Dimensions; ++i)
    {
        bspaceHeader.ParameterExtraction(this, static_cast<CDefaultSkeleton*>(pDefaultSkeleton), bspaceHeader.m_DimPara[i].m_ParaID, i);
    }

    const BSParameter& example = bspaceHeader.m_arrParameter[exampleIndex];
    outParameters = example.m_Para;
    return true;
}

bool CAnimationSet::GetMotionParameterRange(uint32 nAnimationId, EMotionParamID paramID, float& outMin, float& outMax) const
{
    const ModelAnimationHeader* pHeader = GetModelAnimationHeader(nAnimationId);
    if (!pHeader)
    {
        return false;
    }

    if (pHeader->m_nAssetType != LMG_File)
    {
        return false;
    }

    size_t headerIndex = pHeader->m_nGlobalAnimId;
    if (headerIndex >= g_AnimationManager.m_arrGlobalLMG.size())
    {
        return false;
    }

    GlobalAnimationHeaderLMG& bspaceHeader = g_AnimationManager.m_arrGlobalLMG[headerIndex];
    if (!bspaceHeader.IsAssetLMG())
    {
        return false;
    }

    if (bspaceHeader.m_arrCombinedBlendSpaces.size() > 0)
    {
        float resultMin = FLT_MAX;
        float resultMax = -FLT_MAX;
        size_t numCombinedBlendSpaces = bspaceHeader.m_arrCombinedBlendSpaces.size();
        for (size_t i = 0; i < numCombinedBlendSpaces; ++i)
        {
            uint32 combinedHeaderIndex = bspaceHeader.m_arrCombinedBlendSpaces[i].m_ParaGroupID;
            if (combinedHeaderIndex >= g_AnimationManager.m_arrGlobalLMG.size())
            {
                continue;
            }

            GlobalAnimationHeaderLMG& combinedHeader = g_AnimationManager.m_arrGlobalLMG[combinedHeaderIndex];
            for (uint32 i = 0; i < sizeof(combinedHeader.m_DimPara) / sizeof(combinedHeader.m_DimPara[0]); ++i)
            {
                if (combinedHeader.m_DimPara[i].m_ParaID == paramID)
                {
                    float bspaceMin = combinedHeader.m_DimPara[i].m_min;
                    float bspaceMax = combinedHeader.m_DimPara[i].m_max;
                    if (bspaceMin < resultMin)
                    {
                        resultMin = bspaceMin;
                    }
                    if (bspaceMax > resultMax)
                    {
                        resultMax = bspaceMax;
                    }
                }
            }
        }

        if (resultMin != FLT_MAX && resultMax != -FLT_MAX)
        {
            outMin = resultMin;
            outMax = resultMax;
            return true;
        }
        return false;
    }
    else
    {
        for (uint32 i = 0; i < sizeof(bspaceHeader.m_DimPara) / sizeof(bspaceHeader.m_DimPara[0]); ++i)
        {
            if (bspaceHeader.m_DimPara[i].m_ParaID == paramID)
            {
                outMin = bspaceHeader.m_DimPara[i].m_min;
                outMax = bspaceHeader.m_DimPara[i].m_max;
                return true;
            }
        }
        return false;
    }
}


#endif



