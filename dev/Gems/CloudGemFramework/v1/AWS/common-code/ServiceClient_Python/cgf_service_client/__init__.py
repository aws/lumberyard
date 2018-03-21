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
# $Revision: #4 $

from cgf_service_client.path import Path
from cgf_service_client.error import HttpError, ClientError, NotFoundError, NotAllowedError, ServerError
from cgf_utils.data_utils import Data

def for_url(url, **kwargs):
    '''Create a Path object that can be used to make requests using paths relative to the specified url.

    Arguments:

      url - The url.
      
      **kwargs - Used to configure the Path object.

    '''
    return Path(url, **kwargs)

