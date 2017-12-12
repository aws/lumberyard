import { UserManagementAction } from '../user-management.class'
import { AwsContext } from 'app/aws/context.class'
import { BehaviorSubject } from 'rxjs/BehaviorSubject';

declare var AWSCognito: any;

export class UpdatePasswordAction implements UserManagementAction {

    constructor(private context: AwsContext) {

    }

    public handle(success: Function, error: Function, subject: BehaviorSubject<any>, ...args: any[]): void {
        let username = args[0];
        if (username === undefined || username === null || username === '') {
            error("Cognito username was undefined.")            
            return;
        }

        let password = args[1]
        if (password === undefined || password === null || password === '') {
            error("Cognito password was undefined.")            
            return;
        }

        var authenticationData = {
            Username: username.trim(),
            Password: password.trim()
        };
        var authenticationDetails = new AWSCognito.CognitoIdentityServiceProvider.AuthenticationDetails(authenticationData);

        var userdata = {
            Username: authenticationData.Username,
            Pool: this.context.cognitoUserPool
        };

        let cognitouser = new AWSCognito.CognitoIdentityServiceProvider.CognitoUser(userdata)
                
        let userattributes = args[2];        
        
        delete userattributes.email_verified;

        // get these details and call
        cognitouser.completeNewPasswordChallenge(password, userattributes, {
            onSuccess: function (result) {
                success(cognitouser)                
            },

            onFailure: function (err) {
                error(err, cognitouser)
                subject.next(err);
            }
        });
        
    }

}