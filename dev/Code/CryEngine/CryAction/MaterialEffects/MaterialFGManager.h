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

// Description : This class manage all the FlowGraph HUD effects related to a
//               material FX.


#ifndef __MATERIALFGMANAGER_H__
#define __MATERIALFGMANAGER_H__

#pragma once

#include <AzCore/std/string/string.h>
#include <AzCore/std/containers/vector.h>

#include <IFlowSystem.h>
#include "MaterialEffects/MFXFlowGraphEffect.h"

class CMaterialFGManager
{
public:
    CMaterialFGManager();
    virtual ~CMaterialFGManager();

    // load FlowGraphs from specified path
    bool LoadLibs(const char* path = "Libs/MaterialEffects/FlowGraphs");

    // reset (deactivate all FlowGraphs)
    void Reset(bool bCleanUp);

    // serialize
    void Serialize(TSerialize ser);

    bool StartFGEffect(const SMFXFlowGraphParams& fgParams, float curDistance);
    bool EndFGEffect(const string& fgName);
    bool EndFGEffect(IFlowGraphPtr pFG);
    bool IsFGEffectRunning(const string& fgName);
    void SetFGCustomParameter(const SMFXFlowGraphParams& fgParams, const char* customParameter, const SMFXCustomParamValue& customParameterValue);

    void ReloadFlowGraphs(bool editorReload = false);

    int GetFlowGraphCount() const;
    IFlowGraphPtr GetFlowGraph(int index, string* pFileName = NULL) const;
    bool LoadFG(const string& filename, IFlowGraphPtr* pGraphRet = NULL);

    void GetMemoryUsage(ICrySizer* s) const;

    void PreLoad();

protected:
    struct SFlowGraphData
    {
        SFlowGraphData()
        {
            m_startNodeId = InvalidFlowNodeId;
            m_endNodeId = InvalidFlowNodeId;
            m_bRunning = false;
        }

        void GetMemoryUsage(ICrySizer* pSizer) const
        {
            pSizer->AddObject(m_name);
        }
        string              m_name;
        string              m_fileName;
        IFlowGraphPtr   m_pFlowGraph;
        TFlowNodeId   m_startNodeId;
        TFlowNodeId   m_endNodeId;
        bool          m_bRunning;
    };

    SFlowGraphData* FindFG(const string& fgName);
    SFlowGraphData* FindFG(IFlowGraphPtr pFG);
    bool InternalEndFGEffect(SFlowGraphData* pFGData, bool bInitialize);

protected:

    typedef std::vector<SFlowGraphData> TFlowGraphData;
    TFlowGraphData m_flowGraphVector;           //List of FlowGraph Effects

    //! Recursive function collects the path of FlowGraphs to load.
    //! \param path The folder to search for FlowGraphs.
    //! \param[out] libs Collection of FlowGraphs paths.
    void GatherLibs(const char* path, AZStd::vector<AZStd::string>& libs);
};

#endif

