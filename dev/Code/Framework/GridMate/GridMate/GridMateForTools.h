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
#ifndef GRIDMATE_FOR_TOOLS_H
#define GRIDMATE_FOR_TOOLS_H

/**
* This header drives GridMate's tool mode compilation. When compiled in tool mode, memory tracking is disabled
* and all GridMate symbols are put in an alternate namespace to avoid collisions with the regular GridMate.
*
* Tool mode is intended to be used for remote debugging tools to minimize interference with normal operations.
*
* To enable tool mode compilation, define GRIDMATE_FOR_TOOLS
*/

#ifdef GRIDMATE_FOR_TOOLS

//! This define will remap GridMate namespace to GridMateForTools.
#define GridMate GridMateForTools

#endif  // GRIDMATE_FOR_TOOLS

#endif  // GRIDMATE_FOR_TOOLS_H
