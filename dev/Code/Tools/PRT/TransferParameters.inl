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


/*
	transfer parameter implementations
*/
inline NSH::NTransfer::STransferParameters::STransferParameters() : 
				backlightingColour(0,0,0),
				groundPlaneBlockValue(1.0f),
				rayTracingBias(0.05f), 
				minDirectBumpCoeffVisibility(0.f),
				sampleCountPerVertex(-1), 
				configType(NSH::NTransfer::TRANSFER_CONFIG_DEFAULT),
				bumpGranularity(false), 
				supportTransparency(false),
				transparencyShadowingFactor(1.0f),
				matrixTargetCS(MATRIX_HEURISTIC_MAX),
				rayCastingThreads(1),
				compressToByte(false)
{}	

