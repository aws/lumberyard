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
        proj_stack_created =  not self.has_project_stack()
        self.runtest(self.base_create_project_stack)

        deploy_stack_created = not self.has_deployment_stack()
        self.runtest(self.base_create_deployment_stack)

        return proj_stack_created, deploy_stack_created

    def teardown_base_stack(self):
        self.unregister_for_shared_resources()
        self.runtest(self.base_delete_deployment_stack)
        self.runtest(self.base_delete_project_stack)

    def base_create_project_stack(self):
        self.save_context_to_disk()
        self.wait_for_project(self.project_transitions.create)
 
    def base_create_deployment_stack(self):
        self.wait_for_deployment(self.deployment_transitions.create)

    def base_update_project_stack(self):
        self.wait_for_project(self.project_transitions.update)

    def base_update_deployment_stack(self):
        self.wait_for_deployment(self.deployment_transitions.update)

    def base_delete_deployment_stack(self):
        self.wait_for_deployment(self.deployment_transitions.delete)
        
    def base_delete_project_stack(self):
        self.wait_for_project(self.project_transitions.delete)





 
