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
#include "../CryEngine/Cry3DEngine/CGF/ChunkFile.h"
#include "StaticObjectCompiler.h"
#include "StatCGFPhysicalize.h"
#include "CGF/CGFSaver.h"
#include "CGF/CAFSaver.h"
#include "CGF/LoaderCAF.h"
#include "MathHelpers.h"
#include "StringHelpers.h"

#include "AssetWriter.h"
#include "CharacterCompiler.h"
#include "CGA/AnimationCompiler.h"

#include <SceneAPI/SceneCore/DataTypes/Groups/IAnimationGroup.h>

const bool AssetWriter::s_swapEndian;

bool AssetWriter::WriteCGF(CContentCGF* content)
{
    if (!content)
    {
        return false;
    }

    CChunkFile chunkFile;
    CSaverCGF cgfSaver(chunkFile);

#if defined(AZ_PLATFORM_WINDOWS)
    // _EM_INVALID is used to avoid Floating Point Exception inside CryPhysics
    MathHelpers::AutoFloatingPointExceptions autoFpe(~(_EM_INEXACT | _EM_UNDERFLOW | _EM_INVALID));
#endif
    
    CPhysicsInterface physicsInterface;
    CStaticObjectCompiler compiler(&physicsInterface, false);
    CContentCGF* const pCompiledCGF = compiler.MakeCompiledCGF(content);

    if (!pCompiledCGF)
    {
        return false;
    }

    const bool bNeedEndianSwap = false;
    const bool bUseQtangents = false;
    const bool bStorePositionsAsF16 = false;
    const bool bStoreIndicesAsU16 = false;
    cgfSaver.SaveContent(pCompiledCGF, bNeedEndianSwap, bStorePositionsAsF16, bUseQtangents, bStoreIndicesAsU16);

    chunkFile.Write(content->GetFilename());

    return true;
}

//
// Implemented by referencing to ColladaCompiler::CompileToCHR
//
bool AssetWriter::WriteCHR(CContentCGF* content, IConvertContext* convertContext)
{
    if (!content)
    {
        return false;
    }

    CChunkFile chunkFile;
    CSaverCGF cgfSaver(chunkFile);

    cgfSaver.SetContent(content);
    cgfSaver.SaveExportFlags(s_swapEndian);
    
    if (!PrepareSkeletonDataChunks(&cgfSaver))
    {
        return false;
    }

    // A second stage process on the chunk file by removing initial chunks and create new chunks
    OptimizeAndWriteCharacterAsset(content, &chunkFile, convertContext);

    return true;
}

bool AssetWriter::WriteSKIN(CContentCGF* content, IConvertContext* convertContext, bool exportMorphTargets)
{
    if (!content)
    {
        return false;
    }

    CChunkFile chunkFile;
    CSaverCGF cgfSaver(chunkFile);

    cgfSaver.SetContent(content);
    cgfSaver.SaveExportFlags(s_swapEndian);
    cgfSaver.SaveMaterials(s_swapEndian);
    cgfSaver.SaveUncompiledNodes();
    if (exportMorphTargets)
    {
        cgfSaver.SaveUncompiledMorphTargets();
    }

    if (!PrepareSkeletonDataChunks(&cgfSaver))
    {
        return false;
    }

    OptimizeAndWriteCharacterAsset(content, &chunkFile, convertContext);

    return true;
}

bool AssetWriter::PrepareSkeletonDataChunks(CSaverCGF* cgfSaver)
{
    const CSkinningInfo* skinningInfo = cgfSaver->GetContent()->GetSkinningInfo();

    if (skinningInfo->m_arrBonesDesc.empty())
    {
        return false;
    }

    if (skinningInfo->m_arrBonesDesc.size() != skinningInfo->m_arrBoneEntities.size())
    {
        RCLogError("Bone description number and bone entity data number don't match.\n");
        return false;
    }

    // Save bone entity data to chunk
    DynArray<BONE_ENTITY> tempBoneEntities = skinningInfo->m_arrBoneEntities;
    for (int i = 0; i < tempBoneEntities.size(); ++i)
    {
        tempBoneEntities[i].phys.nPhysGeom = -1;
        StringHelpers::SafeCopyPadZeros(tempBoneEntities[i].prop, sizeof(tempBoneEntities[i].prop), skinningInfo->m_arrBoneEntities[i].prop);
    }

    const int numBones = skinningInfo->m_arrBonesDesc.size();
    cgfSaver->SaveBones(s_swapEndian, &tempBoneEntities[0], numBones, numBones*sizeof(BONE_ENTITY));

    // Save bone names
    std::vector<char> boneNames;
    static const int maxBoneNameLength = 32;
    boneNames.reserve(maxBoneNameLength * numBones);
    std::vector<SBoneInitPosMatrix> boneMatices(numBones);
    for (int boneId = 0; boneId < numBones; ++boneId)
    {
        const char* boneName = skinningInfo->m_arrBonesDesc[boneId].m_arrBoneName;
        size_t len = strlen(boneName);
        boneNames.insert(boneNames.end(), boneName, boneName + len);
        boneNames.push_back('\0');

        for (int x = 0; x < 4; ++x)
        {
            // Reference to ColladaCompiler::CompileToCHR
            // CryTek requires the bone matrix is converted from meter unit to centimeter unit
            // We have intentionally convert bone transform matrix to meter unit in FbxSceneSystem::ConvertBoneUnit
            // For example, a valid bone matrix would look like
            // 100 0   0   | 50
            // 0   100 0   | 0
            // 0   0   100 | 0
            // In which 50 means 50 centimeters
            Matrix34 m = Matrix34::CreateScale(Vec3(100.0f, 100.0f, 100.0f)) * skinningInfo->m_arrBonesDesc[boneId].m_DefaultB2W;
            Vec3 column = m.GetColumn(x);
            for (int y = 0; y < 3; ++y)
            {
                boneMatices[boneId][x][y] = column[y];
            }
        }
    }
    boneNames.push_back('\0');

    cgfSaver->SaveBoneNames(s_swapEndian, &boneNames[0], numBones, boneNames.size());
    cgfSaver->SaveBoneInitialMatrices(s_swapEndian, &boneMatices[0], numBones, sizeof(SBoneInitPosMatrix)*numBones);

    return true;
}

bool AssetWriter::OptimizeAndWriteCharacterAsset(CContentCGF* content, CChunkFile* chunkFile, IConvertContext* convertContext)
{
    CharacterCompiler characterCompiler(nullptr);

    string targetFilename = content->GetFilename();

    // Update the covert context for Character Compiler
    IConvertContext* const newContext = characterCompiler.GetConvertContext();
    convertContext->CopyTo(newContext);
    newContext->SetForceRecompiling(true);
    
    // Create a valid CContentCGF from chunk file
    CLoaderCGF cgfLoader;
    CharacterCompiler::Listener listener;
    CContentCGF* const newContentCGF = new CContentCGF(targetFilename.c_str());
    cgfLoader.LoadCGFWork(newContentCGF, targetFilename.c_str(), *chunkFile, &listener, 0);

    return characterCompiler.ProcessWork(&cgfLoader, newContentCGF, chunkFile, false, targetFilename, targetFilename, false);
}

//
// Implemented by referencing to ColladaCompiler::CompileToCAF
//
bool AssetWriter::WriteCAF(CContentCGF* content, const AZ::SceneAPI::DataTypes::IAnimationGroup* animationGroup, CInternalSkinningInfo* controllerSkinningInfo, IConvertContext* convertContext)
{
    AZ_Assert(content, "Null point of CContentCGF passed in to write CAF file.");
    AZ_Assert(controllerSkinningInfo, "Null pointer of CInternalSkinningInfo passed in to write CAF file.");
    if (!content || !controllerSkinningInfo)
    {
        return false;
    }

    CAnimationCompiler animationCompiler;

    const char* targetFilename = content->GetFilename();

    // Update the covert context for Animation Compiler
    IConvertContext* const newContext = animationCompiler.GetConvertContext();
    convertContext->CopyTo(newContext);
    newContext->SetForceRecompiling(true);

    return animationCompiler.ProcessInMemoryData(content, controllerSkinningInfo, targetFilename, animationGroup);
}
