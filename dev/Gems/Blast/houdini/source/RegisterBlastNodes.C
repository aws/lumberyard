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
#include <ROP_Blast.h>
#include <SOP_BlastRenameGroups.h>

#include <OP/OP_OperatorTable.h>

#include <UT/UT_DSOVersion.h>
#include <UT/UT_StringHolder.h>

// Adds the operators for ROP networks
void newDriverOperator(OP_OperatorTable* table)
{
    table->addOperator(
        new OP_Operator(
            "rop_blast",
            "ROP Blast",
            ROP_Blast::myConstructor,
            ROP_Blast::getTemplatePair(),
            0,
            1,
            ROP_Blast::getVariablePair(),
            OP_FLAG_UNORDERED | OP_FLAG_GENERATOR));
}

// Adds the operator for SOP networks
void newSopOperator(OP_OperatorTable* table)
{
    table->addOperator(
        new OP_Operator(
            "rop_blast_sop",
            "ROP Blast",
            ROP_Blast::myConstructor,
            ROP_Blast::getTemplatePair(),
            0,
            1,
            ROP_Blast::getVariablePair(),
            OP_FLAG_GENERATOR | OP_FLAG_MANAGER));

    table->addOperator(
        new OP_Operator(
            "sop_blastrenamegroups",
            "Blast Rename Groups",
            SOP_BlastRenameGroups::myConstructor,
            SOP_BlastRenameGroups::getTemplatePair(),
            0,
            1,
            SOP_BlastRenameGroups::getVariablePair()));
}