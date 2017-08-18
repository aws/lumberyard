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

import lmbr_aws_test_support

class BaseStackTestCase(lmbr_aws_test_support.lmbr_aws_TestCase):
    
    def setup_base_stack(self):        
        self.runtest(self.__010_create_project_stack)        
        self.runtest(self.__020_create_deployment_stack_when_no_resource_groups)  

    def teardown_base_stack(self):
        self.runtest(self.__090_delete_deployment_stacks)
        self.runtest(self.__100_delete_project_stack)

    def __010_create_project_stack(self):
        self.lmbr_aws('create-project-stack', '--stack-name', self.TEST_PROJECT_STACK_NAME, '--confirm-aws-usage', '--confirm-security-change', '--region', lmbr_aws_test_support.REGION)
           
    def __020_create_deployment_stack_when_no_resource_groups(self):                
        self.lmbr_aws('create-deployment', '--deployment', self.TEST_DEPLOYMENT_NAME, '--confirm-aws-usage', '--confirm-security-change')            
             
    def __090_delete_deployment_stacks(self):
        self.lmbr_aws('delete-deployment', '--deployment', self.TEST_DEPLOYMENT_NAME, '--confirm-resource-deletion')
        
    def __100_delete_project_stack(self):
        self.lmbr_aws('delete-project-stack', '--confirm-resource-deletion')




 
