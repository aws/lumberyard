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
#include <SOP_BlastRenameGroups.h>

#include <BlastHelpers.h>

#include <GA/GA_Detail.h>
#include <GA/GA_Primitive.h>
#include <GU/GU_Detail.h>
#include <GU/GU_DetailHandle.h>
#include <OP/OP_AutoLockInputs.h>
#include <OP/OP_Network.h>
#include <OP/OP_OperatorPair.h>
#include <UT/UT_StringHolder.h>

#include <NvBlast.h>

#include <iomanip>
#include <sstream>

static const char* rootNameKey = "rootName";
static const char* groupPrefixKey = "groupPrefix";
static const char* leadingZerosKey = "leadingZeros";

PRM_Template* SOP_BlastRenameGroups::getTemplates()
{
    static std::vector<PRM_Template> templates;
    if (templates.empty())
    {
        static PRM_Name rootName(rootNameKey, "Root Name");
        static PRM_Default rootNameDefault(0, "root");

        static PRM_Name groupPrefix(groupPrefixKey, "Group Prefix");
        static PRM_Default groupPrefixDefault(0, "chunk");

        static PRM_Name leadingZeros(leadingZerosKey, "Leading Zeros");
        static PRM_Default leadingZerosDefault(6);

        templates.emplace_back(PRM_STRING, 1, &rootName, &rootNameDefault);
        templates.emplace_back(PRM_STRING, 1, &groupPrefix, &groupPrefixDefault);
        templates.emplace_back(PRM_INT, 1, &leadingZeros, &leadingZerosDefault);
        templates.emplace_back();
    }

    return templates.data();
}

OP_TemplatePair* SOP_BlastRenameGroups::getTemplatePair()
{
    static OP_TemplatePair* sopPair = nullptr;
    if (sopPair == nullptr)
    {
        sopPair = new OP_TemplatePair(getTemplates());
    }
    return sopPair;
}

OP_VariablePair* SOP_BlastRenameGroups::getVariablePair()
{
    return nullptr;
}

OP_Node* SOP_BlastRenameGroups::myConstructor(OP_Network *net, const char *name, OP_Operator *op)
{
    return new SOP_BlastRenameGroups(net, name, op);
}

SOP_BlastRenameGroups::SOP_BlastRenameGroups(OP_Network *net, const char *name, OP_Operator *entry)
    : SOP_Node(net, name, entry)
{
}

SOP_BlastRenameGroups::~SOP_BlastRenameGroups()
{
}

OP_ERROR SOP_BlastRenameGroups::cookMySop(OP_Context& context)
{
    // We must lock our inputs before we try to access their geometry
    // OP_AutoLockInputs will automatically unlock our inputs when we return
    // NOTE: Don't call unlockInputs yourself when using this!
    OP_AutoLockInputs inputs(this);
    if (inputs.lock(context) >= UT_ERROR_ABORT)
    {
        addError(SOP_MESSAGE, "Couldn't lock our inputs");
        return error();
    }

    // Duplicate incoming geometry
    duplicateSource(0, context);

    // Get the renaming map
    std::unordered_map<std::string, std::string> groupNamesToChunkNames = getGroupNamesToChunkNames(context);
    if (groupNamesToChunkNames.empty())
    {
        addError(SOP_MESSAGE, "Couldn't create chunk rename map");
        return error();
    }

    for (const auto& groupNameToChunkName : groupNamesToChunkNames)
    {
        UT_StringHolder oldName(groupNameToChunkName.first);
        UT_StringHolder newName(groupNameToChunkName.second);
        gdp->getElementGroupTable(GA_ATTRIB_PRIMITIVE).renameGroup(oldName, newName);
    }

    // NOTE: No data IDs need to be bumped when just renaming an attribute, because the data is considered to exclude the name
    return error();
}

const char* SOP_BlastRenameGroups::inputLabel(unsigned int index) const
{
    return "Blast geometry to rename";
}

std::unordered_map<std::string, std::string> SOP_BlastRenameGroups::getGroupNamesToChunkNames(OP_Context& context)
{
    // Create a map from old group names to new chunk names
    std::unordered_map<std::string, std::string> groupNamesToChunkNames;

    // Get the chunk descriptions for reordering
    BlastHelpers::ChunkInfos chunkInfos;
    std::vector<std::string> errors;
        
    UT_String utRootName;
    evalString(utRootName, rootNameKey, 0, context.getTime());
    
    const int leadingZeros = evalInt(leadingZerosKey, 0, context.getTime());

    if (BlastHelpers::createChunkInfos(gdp, context, utRootName.toStdString(), chunkInfos, errors))
    {
        size_t chunkCount = chunkInfos.chunkDescs.size();
        auto chunkDescs = chunkInfos.chunkDescs.data();
        std::vector<uint32_t> chunkReorderMap(chunkCount);
        std::vector<char> scratch(chunkCount * sizeof(NvBlastChunkDesc));

        NvBlastEnsureAssetExactSupportCoverage(chunkDescs, chunkCount, scratch.data(), Nv::Blast::logLL);
        NvBlastBuildAssetDescChunkReorderMap(chunkReorderMap.data(), chunkDescs, chunkCount, scratch.data(), Nv::Blast::logLL);

        UT_String utGroupPrefix;
        evalString(utGroupPrefix, groupPrefixKey, 0, context.getTime());
        std::string groupPrefix = utGroupPrefix.toStdString();

        for (size_t index = 0; index < chunkReorderMap.size(); ++index)
        {
            std::ostringstream chunkNum;
            chunkNum << std::setw(leadingZeros) << std::setfill('0') << chunkReorderMap[index];
            groupNamesToChunkNames.emplace(chunkInfos.chunkNames[index], groupPrefix + chunkNum.str());
        }

    }
    else
    {
        for (const auto& error : errors)
        {
            addError(SOP_MESSAGE, error.c_str());
        }
        addError(SOP_MESSAGE, "Unable to create chunk infos");
    }

    return groupNamesToChunkNames;
}
