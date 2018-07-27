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
#include "StdAfx.h"
#include <AzTest/AzTest.h>

#include "Material.h"

// CMatInfo depends on CrySystem being initialized, so these must be integration tests
AZ_INTEG_TEST_HOOK();

INTEG_TEST(MaterialTests, SetSubMtl_OutOfRange)
{
    _smart_ptr<IMaterial> materialGroup = new CMatInfo();
    _smart_ptr<IMaterial> validSubMaterial = new CMatInfo();
    _smart_ptr<IMaterial> outOfRangeSubMaterial = new CMatInfo();

    // Make materialGroup into an actual material group
    materialGroup->SetSubMtlCount(1);
    materialGroup->SetSubMtl(0, validSubMaterial);
    
    // SetSubMtl should fail because the index is beyond the range of material's vector of sub-materials
    materialGroup->SetSubMtl(2, outOfRangeSubMaterial);
    materialGroup->SetSubMtl(-1, outOfRangeSubMaterial);

    // Material should still have a 1-size vector of sub-materials, with 'subMaterial' as its only sub-material
    EXPECT_TRUE(materialGroup->IsMaterialGroup());
    EXPECT_EQ(materialGroup->GetSubMtlCount(), 1);
    EXPECT_EQ(materialGroup->GetSubMtl(0), validSubMaterial);
    EXPECT_EQ(materialGroup->GetSubMtl(1), nullptr);
}

INTEG_TEST(MaterialTests, SetSubMtl_InvalidSubMaterial)
{
    _smart_ptr<IMaterial> materialGroup = new CMatInfo();
    _smart_ptr<IMaterial> inValidSubMaterial = new CMatInfo();
    _smart_ptr<IMaterial> validSubMaterial0 = new CMatInfo();
    _smart_ptr<IMaterial> validSubMaterial1 = new CMatInfo();

    // Make the two materials into material groups by inserting sub-materials
    materialGroup->SetSubMtlCount(2);
    materialGroup->SetSubMtl(0, validSubMaterial0);
    materialGroup->SetSubMtl(1, validSubMaterial1);

    inValidSubMaterial->SetSubMtlCount(2);
    inValidSubMaterial->SetSubMtl(0, validSubMaterial0);
    inValidSubMaterial->SetSubMtl(1, validSubMaterial1);
    
    // SetSubMtl should fail because subMaterial is a material group, and material groups cannot be sub-materials
    materialGroup->SetSubMtl(1, inValidSubMaterial);

    // Check that subMaterial is not the material at index 1, since SetSubMtl should have failed
    EXPECT_EQ(materialGroup->GetSubMtl(1), validSubMaterial1);
    EXPECT_NE(materialGroup->GetSubMtl(1), inValidSubMaterial);
}

INTEG_TEST(MaterialTests, SetSubMtlCount_SetsMaterialGroupFlag)
{
    _smart_ptr<IMaterial> material = new CMatInfo();

    // Check to ensure the material group flag is being set
    material->SetSubMtlCount(1);
    EXPECT_TRUE(material->IsMaterialGroup());

    // Check to ensure the material group flag is being un-set
    material->SetSubMtlCount(0);
    EXPECT_FALSE(material->IsMaterialGroup());
}