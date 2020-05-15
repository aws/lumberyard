#
# All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
# its licensors.
#
# For complete copyright and license terms please see the LICENSE at the root of this
# distribution (the 'License'). All use of this software is governed by the License,
# or, if provided, by the license below or the license accompanying this file. Do not
# remove or modify any license notices. This file is distributed on an 'AS IS' BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#
# $Revision: #1 $

import six


def empty():
    return {}


def ok_deployment_stack_empty(*args):
    return {
        'StackStatus': 'CREATE_COMPLETE',
        'StackResources': {
            'EmptyDeployment': {
                'ResourceType': 'Custom::EmptyDeployment'
            }
        }
    }


def ok_project_stack(permissions=None, admin_roles=True):
    if permissions is None:
        permissions = {}

    result = {
        'StackStatus': 'UPDATE_COMPLETE',
        'StackResources': {
            'AccessControl': {
                'ResourceType': 'Custom::AccessControl'
            },
            'CloudGemPortal': {
                'ResourceType': 'AWS::S3::Bucket'
            },
            'CloudGemPortalAdministratorRole': {
                'ResourceType': 'AWS::IAM::Role'
            },
            'CloudGemPortalBucketPolicy': {
                'ResourceType': 'AWS::S3::BucketPolicy'
            },
            'CloudGemPortalUserAccess': {
                'ResourceType': 'AWS::IAM::ManagedPolicy'
            },
            'CloudGemPortalUserRole': {
                'ResourceType': 'AWS::IAM::Role'
            },
            'Configuration': {
                'ResourceType': 'AWS::S3::Bucket'
            },
            'ConfigurationBucketPolicy': {
                'ResourceType': 'AWS::S3::BucketPolicy'
            },
            'CoreResourceTypes': {
                'ResourceType': 'Custom::ResourceTypes'
            },
            'Logs': {
                'ResourceType': 'AWS::S3::Bucket'
            },
            'Helper': {
                'ResourceType': 'Custom::Helper'
            },
            'PlayerAccessTokenExchange': {
                'ResourceType': 'AWS::Lambda::Function'
            },
            'PlayerAccessTokenExchangeExecution': {
                'ResourceType': 'AWS::IAM::Role'
            },
            'ProjectIdentityPool': {
                'ResourceType': 'Custom::CognitoIdentityPool'
            },
            'ProjectIdentityPoolAuthenticatedRole': {
                'ResourceType': 'AWS::IAM::Role'
            },
            'ProjectIdentityPoolUnauthenticatedRole': {
                'ResourceType': 'AWS::IAM::Role'
            },
            'ProjectResourceHandler': {
                'ResourceType': 'AWS::Lambda::Function'
            },
            'ProjectResourceHandlerExecution': {
                'ResourceType': 'AWS::IAM::Role'
            },
            'ProjectUserPool': {
                'ResourceType': 'Custom::CognitoUserPool'
            },
            'ServiceApi': {
                'ResourceType': 'Custom::ServiceApi'
            },
            'ServiceLambda': {
                'ResourceType': 'AWS::Lambda::Function'
            },
            'ServiceLambdaConfiguration': {
                'ResourceType': 'Custom::LambdaConfiguration'
            },
            'ServiceLambdaExecution': {
                'ResourceType': 'AWS::IAM::ManagedPolicy'
            },
            'DescribeStacksAndResourcesPolicy': {
                'ResourceType': 'AWS::IAM::ManagedPolicy'
            },
            'ProjectResourceHandlerLogEventsPolicy': {
                'ResourceType': 'AWS::IAM::Policy'
            },
            'PlayerAccessTokenExchangeLogEventsPolicy': {
                'ResourceType': 'AWS::IAM::Policy'
            }
        }
    }

    if admin_roles:
        result['StackResources']['ProjectAccess'] = {'ResourceType': 'AWS::IAM::ManagedPolicy'}
        result['StackResources']['ProjectAdmin'] = {'ResourceType': 'AWS::IAM::Role'}
        result['StackResources']['ProjectOwner'] = {'ResourceType': 'AWS::IAM::Role'}
        result['StackResources']['ProjectOwnerAccess'] = {'ResourceType': 'AWS::IAM::ManagedPolicy'}

    for k, v in six.iteritems(permissions):
        result[k]['Permissions'] = v

    return result


def ok_deployment_access_stack(permissions=None):
    if permissions is None:
        permissions = {}

    result = {
        'StackStatus': 'CREATE_COMPLETE',
        'StackResources': {
            'Player': {
                'ResourceType': 'AWS::IAM::Role'
            },
            'AuthenticatedPlayer': {
                'ResourceType': 'AWS::IAM::Role'
            },
            'Server': {
                'ResourceType': 'AWS::IAM::Role'
            },
            'PlayerAccessIdentityPool': {
                'ResourceType': 'Custom::CognitoIdentityPool'
            },
            'PlayerLoginIdentityPool': {
                'ResourceType': 'Custom::CognitoIdentityPool'
            },
            'PlayerLoginRole': {
                'ResourceType': 'AWS::IAM::Role'
            },
            'DeploymentAdmin': {
                'ResourceType': 'AWS::IAM::Role'
            },
            'DeploymentOwner': {
                'ResourceType': 'AWS::IAM::Role'
            },
            'DeploymentAccess': {
                'ResourceType': 'AWS::IAM::ManagedPolicy'
            },
            'DeploymentOwnerAccess': {
                'ResourceType': 'AWS::IAM::ManagedPolicy'
            },
            'DeploymentAdminRestrictions': {
                'ResourceType': 'AWS::IAM::ManagedPolicy'
            },
            'Helper': {
                'ResourceType': 'Custom::Helper'
            }
        }
    }

    for k, v in six.iteritems(permissions):
        result[k]['Permissions'] = v

    return result
