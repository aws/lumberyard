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
#ifndef CRYINCLUDE_CRYCOMMON_ICOLLISIONAVOIDANCESYSTEM_H
#define CRYINCLUDE_CRYCOMMON_ICOLLISIONAVOIDANCESYSTEM_H
#pragma once

struct ICollisionAvoidanceSystem
{
    typedef size_t AgentID;
    typedef size_t ObstacleID;

    //Agent is any object that actively perticipates in collision avoidance, they act as both an
    //obstacle to other agents, and also avoid other agents and obstacles
    struct Agent
    {
        Agent()
            : radius(0.4f)
            , maxSpeed(4.5f)
            , maxAcceleration(0.1f)
            , currentLocation(ZERO)
            , currentLookDirection(ZERO)
            , currentVelocity(ZERO)
            , desiredVelocity(ZERO)
        {
        };

        Agent(float _radius, float _maxSpeed, float _maxAcceleration, const Vec2& _currentLocation, const Vec2& _currentVelocity,
            const Vec2& _currentLookDirection, const Vec2& _desiredVelocity)
            : radius(_radius)
            , maxSpeed(_maxSpeed)
            , maxAcceleration(_maxAcceleration)
            , currentLocation(_currentLocation)
            , currentVelocity(_currentVelocity)
            , currentLookDirection(_currentLookDirection)
            , desiredVelocity(_desiredVelocity)
        {
        };

        float radius;
        float maxSpeed;
        float maxAcceleration;

        Vec3 currentLocation;
        Vec2 currentVelocity;
        Vec2 currentLookDirection;
        Vec2 desiredVelocity;
    };

    //An obstacle is something other agents should avoid, but does not do any collision avoidance itself
    struct Obstacle
    {
        Obstacle()
            : radius(1.0f)
            , currentLocation(ZERO)
        {
        }

        float radius;

        Vec3 currentLocation;
        Vec2 currentVelocity;
    };


    virtual AgentID CreateAgent(tAIObjectID objectID) = 0;
    virtual ObstacleID CreateObstable() = 0;

    virtual void RemoveAgent(AgentID agentID) = 0;
    virtual void RemoveObstacle(ObstacleID obstacleID) = 0;

    virtual void SetAgent(AgentID agentID, const Agent& params) = 0;
    virtual const Agent& GetAgent(AgentID agentID) const = 0;

    virtual void SetObstacle(ObstacleID obstacleID, const Obstacle& params) = 0;
    virtual const Obstacle& GetObstacle(ObstacleID obstacleID) const = 0;

    virtual const Vec2& GetAvoidanceVelocity(AgentID agentID) = 0;

    virtual void Reset(bool bUnload = false) = 0;
    virtual void Update(float updateTime) = 0;

    virtual void DebugDraw() = 0;
};

#endif // CRYINCLUDE_CRYCOMMON_ICOLLISIONAVOIDANCESYSTEM_H