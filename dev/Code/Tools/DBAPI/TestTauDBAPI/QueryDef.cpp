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

#include "platform.h"
#include "../DBAPI.h"
#include "QueryDef.h"

#undef QUERY_DEF_START
#undef QUERY_DEF_END
#undef QUERY_DEF
#define QUERY_DEF_START(QUERY_GROUP)    QUERY_DEF_IMPL_START(QUERY_GROUP)
#define QUERY_DEF_END                   QUERY_DEF_IMPL_END
#define QUERY_DEF(QueryID, QueryStr)    QUERY_DEF_IMPL_REGISTER(QueryID, QueryStr)
#undef __QUERY_DEF_H_
#include "QueryDef.h"
