import { UserManagementAction } from '../user-management.class'
import { AwsContext } from 'app/aws/context.class'
import { BehaviorSubject } from 'rxjs/BehaviorSubject';

declare var AWSCognito: any;

export class ResetPasswordAction implements UserManagementAction {

    constructor(private context: AwsContext) {

    }

    public handle(success: Function, error: Function, subject: BehaviorSubject<any>, ...args: any[]): void {
        let username = args[0];
        if (username === undefined || username === null || username === '') {
            error("Cognito username was undefined.")            
            return;
        }

        var params = {
            UserPoolId: this.context.userPoolId,
            Username: username.trim()
        };

        // get these details and call
        this.context.cognitoIdentityService.adminResetUserPassword(params, function (err, result){
            if (err) {
                error(err)
                subject.next(err)
                return;
            }
            success(result)        
        });
        
    }

}