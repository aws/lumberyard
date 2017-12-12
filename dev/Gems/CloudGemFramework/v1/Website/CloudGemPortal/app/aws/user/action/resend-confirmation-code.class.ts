import { UserManagementAction } from '../user-management.class'
import { AwsContext } from 'app/aws/context.class'
import { BehaviorSubject } from 'rxjs/BehaviorSubject';

declare var AWSCognito: any;

export class ResendConfirmationCodeAction implements UserManagementAction {

    constructor(private context: AwsContext) {

    }

    public handle(success: Function, error: Function, subject: BehaviorSubject<any>, ...args: any[]): void {
        var username = args[0];
        if (username === undefined || username === null || username === '') {
            error("Cognito username was undefined.")            
            return;
        }    
        
        var userdata = {
            Username: username.trim(),
            Pool: this.context.cognitoUserPool
        };

        let cognitouser = new AWSCognito.CognitoIdentityServiceProvider.CognitoUser(userdata)

        cognitouser.resendConfirmationCode(function (err, result) {
            if (err) {
                error(err)      
                subject.next(err)          
                return;
            }
            success(result)            
        });
    }

}