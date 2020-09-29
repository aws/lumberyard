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

import boto3

import ban_handler
import cgf_lambda_settings
import cgf_service_client
import errors
import identity_validator
import service

UNKNOWN_PLAYER_ERROR_MESSAGE = "User '{}' is not registered with the PlayerAccount Gem or has not sent data to the Leaderboards Gem"


@service.api
def post(request, user=None):
    """
    Call PlayerAccount to ban the player.

    Player must be a registered uer in the PlayerAccount Gem and Leaderboards must have seen the player
    via a data request to have a mapping between the user name and the cognition identity (for get_id_from_user)
    """
    print("Handling player ban for {}".format(user))
    interface_url = cgf_lambda_settings.get_service_url("CloudGemPlayerAccount_banplayer_1_0_0")

    if not interface_url:
        return {
            "status": ban_handler.ban(user)
        }

    service_client = cgf_service_client.for_url(interface_url, verbose=True, session=boto3._get_default_session())
    navigation = service_client.navigate('playerban')
    cog_id = identity_validator.get_id_from_user(user)
    if cog_id is None:
        raise errors.ClientError(UNKNOWN_PLAYER_ERROR_MESSAGE.format(user))

    result = navigation.POST(
        {"id": cog_id}
    )
    return result.DATA


@service.api
def delete(request, user=None):
    """
    Call PlayerAccount to unban the player
    
    Player must be a registered uer in the PlayerAccount Gem and Leaderboards must have seen the player
    via a data request to have a mapping between the user name and the cognition identity (for get_id_from_user)
    """
    print("Handling player unban for {}".format(user))
    interface_url = cgf_lambda_settings.get_service_url("CloudGemPlayerAccount_banplayer_1_0_0")
    if not interface_url:
        return {
            "status": ban_handler.lift_ban(user)
        }

    service_client = cgf_service_client.for_url(interface_url, verbose=True, session=boto3._get_default_session())
    navigation = service_client.navigate('playerban')
    cog_id = identity_validator.get_id_from_user(user)
    if cog_id is None:
        raise errors.ClientError(UNKNOWN_PLAYER_ERROR_MESSAGE.format(user))

    result = navigation.DELETE(
        {"id": cog_id}
    )
    return result.DATA
