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
/**
 * Unity Build module! Used to generate smaller lib sizes, less code, compiler can optimize better, smaller exe/elf later, etc.
 * In addition we get a speed up (if we do a local build). The build machine (or a user) will define
 * AZ_UNITY_BUILD when needed.
 */
#ifdef AZ_UNITY_BUILD
#undef AZ_UNITY_BUILD // allow cpps to compile

#include "Carrier/SocketDriver.cpp"
#include "Carrier/Utils.cpp"
#include "Carrier/Carrier.cpp"
#include "Carrier/DefaultSimulator.cpp"
#include "Carrier/DefaultTrafficControl.cpp"
#include "Carrier/DefaultHandshake.cpp"
#include "Replica/DataSet.cpp"
#include "Replica/Replica.cpp"
#include "Replica/ReplicaChunk.cpp"
#include "Replica/ReplicaChunkDescriptor.cpp"
#include "Replica/ReplicaMgr.cpp"
#include "Replica/ReplicaStatus.cpp"
#include "Replica/ReplicaUtils.cpp"
#include "Replica/RemoteProcedureCall.cpp"
#include "Replica/Tasks/ReplicaMarshalTasks.cpp"
#include "Replica/Tasks/ReplicaUpdateTasks.cpp"
#include "Replica/Tasks/ReplicaTaskManager.cpp"
#include "Replica/Interest/InterestManager.cpp"
#include "Replica/Interest/InterestQueryResult.cpp"
#include "Replica/SystemReplicas.cpp"
#include "Session/LANSession.cpp"
#include "Session/Session.cpp"
#include "Serialize/Buffer.cpp"
#include "Serialize/CompressionMarshal.cpp"
#include "Online/LANOnlineMgr.cpp"
#include "Online/OnlineService.cpp"
#include "Drillers/CarrierDriller.cpp"
#include "Drillers/SessionDriller.cpp"
#include "Leaderboard/LeaderboardService.cpp"

#include "GridMate.cpp"

#define AZ_UNITY_BUILD
#endif // AZ_UNITY_BUILD