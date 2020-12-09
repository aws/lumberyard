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
#pragma once

#include <SOP/SOP_Node.h>

#include <NvBlastGlobals.h>

#include <unordered_map>

class OP_Context;
class OP_Network;
class OP_Operator;
class OP_TemplatePair;
class OP_VariablePair;
class PRM_Template;

class SOP_BlastRenameGroups : public SOP_Node
{
public:
    static OP_TemplatePair* getTemplatePair();
    static OP_VariablePair* getVariablePair();
    static OP_Node* myConstructor(OP_Network* net, const char* name, OP_Operator* op);

protected:
    SOP_BlastRenameGroups(OP_Network* net, const char* name, OP_Operator* op);
    ~SOP_BlastRenameGroups() override;

    OP_ERROR cookMySop(OP_Context& context) override;
    const char* inputLabel(unsigned int index) const override;

    std::unordered_map<std::string, std::string> getGroupNamesToChunkNames(OP_Context& context);

    static PRM_Template* getTemplates();
};
