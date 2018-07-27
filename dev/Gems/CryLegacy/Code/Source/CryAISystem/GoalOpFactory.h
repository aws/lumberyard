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

// Description : Goal Op Factory interface and management classes


#ifndef CRYINCLUDE_CRYAISYSTEM_GOALOPFACTORY_H
#define CRYINCLUDE_CRYAISYSTEM_GOALOPFACTORY_H
#pragma once

#include <IGoalPipe.h>

/** Interface for classes than can create and return a goalop instance */
struct IGoalOpFactory
{
    virtual ~IGoalOpFactory(){}
    /**
    * Create a goalop instance
    * Attempts to create a goalop instance from the goalop name and script arguments.
    * Should return NULL if not recognised.
    * @return A new goalop instance or NULL, and correct parameters
    */
    virtual IGoalOp* GetGoalOp(const char* sGoalOpName, IFunctionHandler* pH, int nFirstParam, GoalParameters& params) const = 0;

    /**
    * Create a goalop instance
    * Attempts to create a goalop instance from the goalop id and parameters.
    * Should return NULL if not recognised.
    * @return A new goalop instance or NULL
    */
    virtual IGoalOp* GetGoalOp(EGoalOperations name, GoalParameters& params) const = 0;
};

/**
* An ordering of goalop factories.
* Attempts to delegate to a series of factories in the order they were added and returns the first result.
* Should return NULL if none of them recognize the goalop.
* An empty ordering will always return NULL;
* @return A new goalop instance or NULL
*/
class CGoalOpFactoryOrdering
    : public IGoalOpFactory
{
public:
    IGoalOp* GetGoalOp(const char* sGoalOpName, IFunctionHandler* pH, int nFirstParam, GoalParameters& params) const;

    IGoalOp* GetGoalOp(EGoalOperations op, GoalParameters& params) const;

    /** Add a factory to the ordering. */
    void AddFactory(IGoalOpFactory* pFactory);

    /** Reserve space for factories. */
    void PrepareForFactories(size_t count);

    /**
    * Destroy all factories in the ordering.
    * Result is is an empty ordering.
    */
    void DestroyAll(void);

protected:
    typedef std::vector<IGoalOpFactory*> TFactoryVector;
    TFactoryVector m_Factories;
};

#endif // CRYINCLUDE_CRYAISYSTEM_GOALOPFACTORY_H
