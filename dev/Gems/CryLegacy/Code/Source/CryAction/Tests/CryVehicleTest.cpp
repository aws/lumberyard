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
#include "CryLegacy_precompiled.h"
#include <AzTest/AzTest.h>
#include "CryAction/VehicleSystem/VehicleAnimation.h"
#include "CryAction/VehicleSystem/VehicleSeatGroup.h"


TEST(CryVehicleTests, EmptyAnimationStateTest_FT)
{
    CVehicleAnimation vehAnim;

    //vehicle animation with no states so we expect these to return false
    EXPECT_FALSE(vehAnim.ChangeState(InvalidVehicleAnimStateId));
    EXPECT_FALSE(vehAnim.ChangeState(0));

    //again, with no states we expect an empty string returned
    EXPECT_EQ(vehAnim.GetStateName(0), "");
}

TEST(CryVehicleTests, EmptySeatGroupTest_FT)
{
    CVehicleSeatGroup vehSeatGroup;

    //no seats available
    CVehicleSeat* seat = vehSeatGroup.GetSeatByIndex(0);
    EXPECT_TRUE(seat == NULL);
}