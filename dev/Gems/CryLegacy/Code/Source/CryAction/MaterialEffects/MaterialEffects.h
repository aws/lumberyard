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

#ifndef CRYINCLUDE_CRYACTION_MATERIALEFFECTS_MATERIALEFFECTS_H
#define CRYINCLUDE_CRYACTION_MATERIALEFFECTS_MATERIALEFFECTS_H
#pragma once

#include <IMaterialEffects.h>

#include "MFXLibrary.h"
#include "MFXContainer.h"
#include "SurfacesLookupTable.h"

class CMaterialFGManager;
class CScriptBind_MaterialEffects;

namespace MaterialEffectsUtils
{
    class CVisualDebug;
}

class CMaterialEffects
    : public IMaterialEffects
{
private:
    friend class CScriptBind_MaterialEffects;

    static const int kMaxSurfaceCount = 255 + 1; // from SurfaceTypeManager.h, but will also work with any other number
    typedef IEntityClass*   TPtrIndex;
    typedef int             TIndex;
    typedef std::pair<TIndex, TIndex> TIndexPair;

    typedef std::vector<TIndex>       TSurfaceIdToLookupTable;
    typedef std::map<TMFXNameId, CMFXLibrary, stl::less_stricmp<string> > TFXLibrariesMap;
    typedef std::map<const TPtrIndex, TIndex> TPointerToLookupTable;
    typedef std::map<string, TIndex, stl::less_stricmp<string> > TCustomNameToLookupTable;
    typedef std::vector<TMFXContainerPtr>  TMFXContainers;

    struct SDelayedEffect
    {
        SDelayedEffect()
            : m_delay(0.0f)
        {
        }

        SDelayedEffect(TMFXContainerPtr pEffectContainer, const SMFXRunTimeEffectParams& runtimeParams)
            : m_pEffectContainer(pEffectContainer)
            , m_effectRuntimeParams(runtimeParams)
            , m_delay(pEffectContainer->GetParams().delay)
        {
        }

        void GetMemoryUsage(ICrySizer* pSizer) const
        {
            pSizer->AddObject(m_pEffectContainer);
        }

        TMFXContainerPtr m_pEffectContainer;
        SMFXRunTimeEffectParams m_effectRuntimeParams;
        float m_delay;
    };

    typedef std::vector<SDelayedEffect> TDelayedEffects;

public:
    CMaterialEffects();
    virtual ~CMaterialEffects();

    // load flowgraph based effects
    bool LoadFlowGraphLibs();

    // serialize
    void Serialize(TSerialize ser);

    // load assets referenced by material effects.
    void PreLoadAssets();

    // IMaterialEffects
    virtual void LoadFXLibraries();
    virtual void Reset(bool bCleanup);
    virtual void ClearDelayedEffects();
    virtual TMFXEffectId GetEffectIdByName(const char* libName, const char* effectName);
    virtual TMFXEffectId GetEffectId(int surfaceIndex1, int surfaceIndex2);
    virtual TMFXEffectId GetEffectId(const char* customName, int surfaceIndex2);
    virtual TMFXEffectId GetEffectId(IEntityClass* pEntityClass, int surfaceIndex2);
    virtual SMFXResourceListPtr GetResources(TMFXEffectId effectId) const;
    virtual bool ExecuteEffect(TMFXEffectId effectId, SMFXRunTimeEffectParams& runtimeParams);
    virtual void StopEffect(TMFXEffectId effectId);
    virtual int GetDefaultSurfaceIndex() { return m_defaultSurfaceId; }
    virtual int GetDefaultCanopyIndex();
    virtual bool PlayBreakageEffect(ISurfaceType* pSurfaceType, const char* breakageType, const SMFXBreakageParams& breakageParams);
    virtual void SetCustomParameter(TMFXEffectId effectId, const char* customParameter, const SMFXCustomParamValue& customParameterValue);
    virtual void CompleteInit();
    virtual void ReloadMatFXFlowGraphs(bool editorReload);
    virtual int GetMatFXFlowGraphCount() const;
    virtual IFlowGraphPtr GetMatFXFlowGraph(int index, string* pFileName = NULL) const;
    virtual IFlowGraphPtr LoadNewMatFXFlowGraph(const string& filename);
    // ~IMaterialEffects

    void GetMemoryUsage(ICrySizer* s) const;
    void NotifyFGHudEffectEnd(IFlowGraphPtr pFG);
    void Update(float frameTime);
    void SetUpdateMode(bool bMode);
    CMaterialFGManager* GetFGManager() const { return m_pMaterialFGManager; }

    void FullReload();

private:

    // Loading data from files
    void LoadSpreadSheet();
    void LoadFXLibrary(const char* name);

    // schedule effect
    void TimedEffect(TMFXContainerPtr pEffectContainer, const SMFXRunTimeEffectParams& params);

    // index1 x index2 are used to lookup in m_matmat array
    inline TMFXEffectId InternalGetEffectId(int index1, int index2) const
    {
        const TMFXEffectId effectId = m_surfacesLookupTable.GetValue(index1, index2, InvalidEffectId);
        return effectId;
    }

    inline TMFXContainerPtr InternalGetEffect(const char* libName, const char* effectName) const
    {
        TFXLibrariesMap::const_iterator iter = m_mfxLibraries.find(CONST_TEMP_STRING(libName));
        if (iter != m_mfxLibraries.end())
        {
            const CMFXLibrary& mfxLibrary = iter->second;
            return mfxLibrary.FindEffectContainer(effectName);
        }
        return 0;
    }

    inline TMFXContainerPtr InternalGetEffect(TMFXEffectId effectId) const
    {
        assert (effectId < m_effectContainers.size());
        if (effectId < m_effectContainers.size())
        {
            return m_effectContainers[effectId];
        }
        return 0;
    }

    inline TIndex SurfaceIdToMatrixEntry(int surfaceIndex)
    {
        return (surfaceIndex >= 0 && surfaceIndex < m_surfaceIdToMatrixEntry.size()) ? m_surfaceIdToMatrixEntry[surfaceIndex] : 0;
    }

private:

    int                 m_defaultSurfaceId;
    int                 m_canopySurfaceId;
    bool                m_bUpdateMode;
    bool                m_bDataInitialized;
    CMaterialFGManager* m_pMaterialFGManager;

    // The libraries we have loaded
    TFXLibrariesMap m_mfxLibraries;

    // This maps a surface type to the corresponding column(==row) in the matrix
    TSurfaceIdToLookupTable m_surfaceIdToMatrixEntry;

    // This maps custom pointers (entity classes)
    TPointerToLookupTable m_ptrToMatrixEntryMap;

    // Converts custom tags to indices
    TCustomNameToLookupTable m_customConvert;

    // Main lookup surface x surface -> effectId
    MaterialEffectsUtils::SSurfacesLookupTable<TMFXEffectId> m_surfacesLookupTable;

    // All effect containers (indexed by TEffectId)
    TMFXContainers m_effectContainers;

    // runtime effects which are delayed
    TDelayedEffects m_delayedEffects;

#ifdef MATERIAL_EFFECTS_DEBUG
    friend class MaterialEffectsUtils::CVisualDebug;

    MaterialEffectsUtils::CVisualDebug* m_pVisualDebug;
#endif
};


#endif // CRYINCLUDE_CRYACTION_MATERIALEFFECTS_MATERIALEFFECTS_H
