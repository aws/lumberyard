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

def ok_project_stack(permissions = {}):
    result = {
        'StackStatus': 'UPDATE_COMPLETE',
        'StackResources': {
            'Configuration': {
                'ResourceType': 'AWS::S3::Bucket'
            },
            'ConfigurationBucketPolicy': {
                'ResourceType': 'AWS::S3::BucketPolicy'
            },
            'Logs': {
                'ResourceType': 'AWS::S3::Bucket'
            },
            'CloudGemPortal': {
                'ResourceType': 'AWS::S3::Bucket'
            },
            'CloudGemPortalBucketPolicy': {
                'ResourceType': 'AWS::S3::BucketPolicy'
            },
            'CloudGemPortalAccessControl': {
                'ResourceType': 'Custom::AccessControl'
            },
            'CloudGemPortalServiceApi': {
                'ResourceType': 'Custom::ServiceApi'
            },
            'CloudGemPortalServiceLambda': {
                'ResourceType': 'AWS::Lambda::Function'
            },
            'ProjectUserPool': {
                'ResourceType': 'Custom::CognitoUserPool'
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
            'CloudGemPortalUserRole': {
                'ResourceType': 'AWS::IAM::Role'
            },
            'CloudGemPortalAdministratorRole': {
                'ResourceType': 'AWS::IAM::Role'
            },
            'CloudGemPortalUserAccess': {
                'ResourceType': 'AWS::IAM::ManagedPolicy'
            },
            'PlayerAccessTokenExchange': {
                'ResourceType': 'AWS::Lambda::Function'
            },
            'PlayerAccessTokenExchangeExecution': {
                'ResourceType': 'AWS::IAM::Role'
            },
            'ProjectResourceHandler': {
                'ResourceType': 'AWS::Lambda::Function'
            },
            'ProjectResourceHandlerExecution': {
                'ResourceType': 'AWS::IAM::Role'
            },
            'AccessControl': {
                'ResourceType': 'Custom::AccessControl'
            },
            'ProjectAdmin': {
                'ResourceType': 'AWS::IAM::Role'
            },
            'ProjectOwner': {
                'ResourceType': 'AWS::IAM::Role'
            },
            'ProjectAccess': {
                'ResourceType': 'AWS::IAM::ManagedPolicy'
            },
            'ProjectOwnerAccess': {
                'ResourceType': 'AWS::IAM::ManagedPolicy'
            },
            'ProjectAdminRestrictions': {
                'ResourceType': 'AWS::IAM::ManagedPolicy'
            },
            'Helper': {
                'ResourceType': 'Custom::Helper'
            },
            'ProjectServiceLambda': {
                'ResourceType': 'AWS::Lambda::Function'
            },
            'ProjectServiceLambdaExecution': {
                'ResourceType': 'AWS::IAM::Role'
            }
        }
    }

    for k,v in permissions.iteritems():
        result[k]['Permissions'] = v

    return result

def ok_deployment_access_stack(permissions = {}):
    result = {
        'StackStatus': 'CREATE_COMPLETE',
        'StackResources': {
            'Player': {
                'ResourceType': 'AWS::IAM::Role'
            },
            'AccessControl': {
                'ResourceType': 'Custom::AccessControl'
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

    for k,v in permissions.iteritems():
        result[k]['Permissions'] = v

    return result


def ok_deployment_stack_sample_only(stack_status, sample_group_name, group_stack_status):
    return {
                'StackStatus': stack_status,
                'StackResources': {
                    sample_group_name+'Configuration': {
                        'ResourceType': 'Custom::ResourceGroupConfiguration'
                    },
                    sample_group_name: {
                        'ResourceType': 'AWS::CloudFormation::Stack',
                        'StackStatus': group_stack_status,
                        'StackResources': {
                            'Messages': {
                                'ResourceType': 'AWS::DynamoDB::Table'
                            },
                            'SayHelloConfiguration': {
                                'ResourceType': 'Custom::LambdaConfiguration'
                            },
                            'SayHello': {
                                'ResourceType': 'AWS::Lambda::Function',
                                'Permissions': [
                                    {
                                        'Resources': [
                                            '$Messages$'
                                        ],
                                        'Allow': [
                                            'dynamodb:PutItem'
                                        ]
                                    }
                                ]
                            },
                            'AccessControl': {
                                'ResourceType': 'Custom::AccessControl'
                            }
                        }
                    }
                }
            }
