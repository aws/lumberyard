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
#include <PRT/SHFramework.h>
#include "ImageFactory.h"
#include "PrintTransferObserver.h"
#include "Transfer.h"
#include "SampleGenerator.h"
#include <PRT/SimpleIndexedMesh.h>
#include "RayCaster.h"
#include "RandomSampleGen.h"
#include <PRT/MeshCoefficientStreams.h>
#include "TransferConfiguratorFactory.h"

#if defined(USE_D3DX)
    #include "d3dx9.h"
    #pragma comment(lib,"d3dx9.lib")
    #pragma comment(lib,"d3d9.lib ")
#endif

//need to initialize somewhere
const float NSH::SCoeffUsage<NSH::BUMP_VIS_USAGE_LOOKUP>::cv0 = 0.28209479f;
const float NSH::SCoeffUsage<NSH::BUMP_VIS_USAGE_LOOKUP>::cv1 = 0.48860251f;
const float NSH::SCoeffUsage<NSH::BUMP_VIS_USAGE_LOOKUP>::cv2 = 1.09254843f;
const float NSH::SCoeffUsage<NSH::BUMP_VIS_USAGE_LOOKUP>::cv3 = 0.31539157f;
const float NSH::SCoeffUsage<NSH::BUMP_VIS_USAGE_LOOKUP>::cv4 = 0.54627422f;

const float NSH::SCoeffUsage<NSH::BUMP_VIS_USAGE_COS_CONVOLUTION>::cv0 = 0.28209479f;
const float NSH::SCoeffUsage<NSH::BUMP_VIS_USAGE_COS_CONVOLUTION>::cv1 = 1.02332671f * 0.31830988f;
const float NSH::SCoeffUsage<NSH::BUMP_VIS_USAGE_COS_CONVOLUTION>::cv2 = 0.85808553f * 0.31830988f;
const float NSH::SCoeffUsage<NSH::BUMP_VIS_USAGE_COS_CONVOLUTION>::cv3 = 0.24770796f * 0.31830988f;
const float NSH::SCoeffUsage<NSH::BUMP_VIS_USAGE_COS_CONVOLUTION>::cv4 = 0.42904277f * 0.31830988f;

//typedef NSH::NSampleGen::CSampleGenerator<NSH::NSampleGen::CLinearSampleOrganizer/*NSH::NSampleGen::CIsocahedronSampleOrganizer*/, NSH::NSampleGen::SSampleGenPolicyStratisfied/*NSH::NSampleGen::SSampleGenPolicyHammersley*/> TSampleGen;
typedef NSH::NSampleGen::CSampleGenerator<NSH::NSampleGen::CIsocahedronSampleOrganizer, /*NSH::NSampleGen::SSampleGenPolicyStratisfied*/ NSH::NSampleGen::SSampleGenPolicyHammersley> TSampleGen;

//!< singleton stuff
NSH::NFramework::CSHFrameworkManager* NSH::NFramework::CSHFrameworkManager::Instance(const NSH::NFramework::SFrameworkParameter& crParams, const bool cUseCurrent)
{
    static CSHFrameworkManager s_inst;
    if (!s_inst.m_InstanceActive)
    {
        //  _controlfp(0, _IC_PROJECTIVE|_IC_AFFINE|_EM_DENORMAL|_EM_INVALID|_EM_ZERODIVIDE|_EM_UNDERFLOW|_PC_64 );
        assert(!s_inst.m_pSampleGen);
        s_inst.m_Desc.Bands = crParams.supportedBands;
        s_inst.m_Desc.Coefficients = crParams.supportedBands * crParams.supportedBands;
        s_inst.m_pSampleGen = new TSampleGen(s_inst.m_Desc, crParams.sampleCount, crParams.minSampleCountToRetrieve);
        assert(s_inst.m_pSampleGen);
        s_inst.m_pSampleGen->GenerateSamples(crParams.sampleCount, s_inst.m_Desc);
        s_inst.m_InstanceActive = true;
    }
    else
    {
        if (!cUseCurrent && (crParams.sampleCount != s_inst.m_pSampleGen->OrderedSize() || crParams.supportedBands > s_inst.m_pSampleGen->SHDescriptor().Bands))
        {
            s_inst.m_Desc.Bands = crParams.supportedBands;
            s_inst.m_Desc.Coefficients = crParams.supportedBands * crParams.supportedBands;
            s_inst.m_pSampleGen->Restart(crParams.sampleCount, s_inst.m_Desc);
        }
    }

    return &s_inst;
}

inline NSH::NFramework::CSHFrameworkManager::~CSHFrameworkManager()
{
    delete m_pSampleGen;
    m_pSampleGen = 0;
}

inline const NSH::NSampleGen::ISampleGenerator& NSH::NFramework::CSHFrameworkManager::GetSampleGen() const
{
    assert(m_pSampleGen);
    return *m_pSampleGen;
}

/****************************************************MAIN FRAMEWORK FUNCTIONS*********************************************************************/

#if defined(OFFLINE_COMPUTATION)

const bool NSH::NFramework::ComputeSingleMeshTransferCompressed
(
    CSimpleIndexedMesh& rMesh,
    const NTransfer::STransferParameters& crParameters,
    SMeshCompressedCoefficients& rCompressedCoeffs
)
{
    //get singleton for framework functionality
    CSHFrameworkManager* pFrameworkMan = CSHFrameworkManager::Instance(SFrameworkParameter(), true);//default values suffice here
    assert(pFrameworkMan);

    NTransfer::STransferParameters transferParameters = crParameters;
    transferParameters.compressToByte = true; //forced

    GetSHLog().LogTime();
    GetSHLog().Log("\nComputeSingleMeshTransfer: called for one mesh\n\n");
    transferParameters.Log();

    NTransfer::CInterreflectionTransfer interreflectionTransfer;
    CPrintTransferObserver observer(&interreflectionTransfer);

    TGeomVec simpleMeshes;
    simpleMeshes.push_back(&rMesh);

    NSH::NTransfer::CTransferConfiguratorFactory* pTransferConfiguratorFactory = NSH::NTransfer::CTransferConfiguratorFactory::Instance();
    assert(pTransferConfiguratorFactory);
    ITransferConfiguratorPtr pConfig(pTransferConfiguratorFactory->GetTransferConfigurator(crParameters));

    if (!interreflectionTransfer.Process(pFrameworkMan->GetSampleGen(), simpleMeshes, pFrameworkMan->m_Desc, transferParameters, *pConfig))
    {
        return false;
    }
    const char* cpMeshName = rMesh.GetMeshName();
    //save coefficients
    TScalarCoeffVecVec& rMeshCoefficients               = interreflectionTransfer.GetDirectCoefficients();
    TScalarCoeffVec& rCoeffsToStore = rMeshCoefficients[0];

    CMeshCoefficientStreams<TScalarCoeff> meshCoeffStreams(pFrameworkMan->m_Desc, transferParameters, rMesh, rCoeffsToStore, *pConfig);

    meshCoeffStreams.RetrieveCompressedCoeffs(rCompressedCoeffs);

    uint32 storageBytes = rMesh.GetExportPolicy().coefficientsPerSet * rMesh.GetVertexCount();
    GetSHLog().Log("additional storage costs for mesh %s: %.2f KB\n", cpMeshName, (float)storageBytes * 1.f / 1024.f);
    GetSHLog().LogTime();
    GetSHLog().Log("ComputeSingleMeshTransfer: finished\n\n");

    return true;
}

/************************************************************************************************************************************************/

#endif

//inline to be able to use stl
const uint32 NSH::NFramework::QuantizeSamplesAzimuth
(
    CQuantizedSamplesHolder& rSampleHolder,
    const bool cOnlyUpperHemisphere
)
{
    const size_t cAzimuthSteps = rSampleHolder.Discretizations();
    NSH::NFramework::SFrameworkParameter frameworkParams;
    frameworkParams.sampleCount = (uint32)(cAzimuthSteps * cAzimuthSteps);
    frameworkParams.supportedBands = 3;
    frameworkParams.minSampleCountToRetrieve = (uint32)cAzimuthSteps;
    const NSH::NFramework::CSHFrameworkManager& crFrameMan = *NFramework::CSHFrameworkManager::Instance(frameworkParams, false /*restart sample gen*/);

    uint32 numSamples = 0;
    //walk through all samples and quantize them
    NSH::TScalarVecVec samples;
    crFrameMan.GetSampleGen().GetSamples(samples);

    const float cInvAzimuthFactor = (float)cAzimuthSteps / (float)(2. * g_cPi);
    const float cAzimuthGenFactor = 1.f / cInvAzimuthFactor;

    const NSH::TScalarVecVec::iterator cEnd = samples.end();
    for (NSH::TScalarVecVec::iterator iter = samples.begin(); iter != cEnd; ++iter)
    {
        if (cOnlyUpperHemisphere && (*iter).GetCartesianPos().z < 0)
        {
            continue;
        }

        TPolarCoord polCoord = (*iter).GetPolarPos();
        //quantize polar angle
        const uint32 cAzimuthIndex = (uint32)(polCoord.phi * cInvAzimuthFactor);
        polCoord.phi = (float)((float)cAzimuthIndex + 0.5f) * cAzimuthGenFactor;//quantize
        assert(cAzimuthIndex < cAzimuthSteps);
        rSampleHolder[cAzimuthIndex].push_back(TSample(crFrameMan.GetSampleGen().SHDescriptor(), polCoord));
        numSamples++;
    }
    return numSamples;
}

