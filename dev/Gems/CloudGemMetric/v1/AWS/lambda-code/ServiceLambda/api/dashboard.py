#
# All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
# its licensors.
#
# For complete copyright and license terms please see the LICENSE at the root of this
# distribution (the "License"). All use of this software is governed by the License,
# or, if provided, by the license below or the license accompanying this file. Do not
# remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#

import service
import os
import util
import metric_constant as c
import json
from dynamodb import DynamoDb
from cgf_utils import custom_resource_response


@service.api
def get(request, facetid):    
    db = DynamoDb()
    result = db.get_key(facetid)    
    result = json.dumps(result, cls=util.DynamoDbDecoder)
    return result

@service.api
def post(request, facetid, meta):
    db = DynamoDb()    
    return db.set(facetid, meta)


def cli(context, args):
    util.set_logger(args.verbose)

    from resource_manager_common import constant
    credentials = context.aws.load_credentials()

    resources = util.get_resources(context)
    os.environ[c.ENV_REGION] = context.config.project_region
    os.environ[c.ENV_DB_TABLE_CONTEXT] = resources[c.RES_DB_TABLE_CONTEXT]
    os.environ["AWS_ACCESS_KEY"] = args.aws_access_key if args.aws_access_key else credentials.get(args.profile if args.profile else context.config.user_default_profile, constant.ACCESS_KEY_OPTION)
    os.environ["AWS_SECRET_KEY"] = args.aws_secret_key if args.aws_secret_key else credentials.get(args.profile if args.profile else context.config.user_default_profile, constant.SECRET_KEY_OPTION)
    print eval(args.function)({}, args.param )
