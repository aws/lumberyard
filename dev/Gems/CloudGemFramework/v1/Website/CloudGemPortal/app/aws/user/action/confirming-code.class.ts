import { UserManagementAction } from '../user-management.class'
import { AwsContext } from 'app/aws/context.class'
import { BehaviorSubject } from 'rxjs/BehaviorSubject';

declare var AWSCognito: any;

export class ConfirmingCodeAction implements UserManagementAction {

    constructor(private context: AwsContext) {

    }

    public handle(success: Function, error: Function, subject: BehaviorSubject<any>, ...args: any[]): void {
        var code = args[0];       
        if (code === undefined || code === null || code === '') {
            error(["Cognito user code was undefined."]);
            return;
        }    

        var username = args[1];
        if (username === undefined || username === null || username === '') {
            error("Cognito username was undefined.");            
            return;
        }   

        var userdata = {
            Username: username,
            Pool: this.context.cognitoUserPool
        };

        let cognitouser = new AWSCognito.CognitoIdentityServiceProvider.CognitoUser(userdata)

        cognitouser.confirmRegistration(code, true, function (err, result) {
            if (err) {
                error(err)
                subject.next(err)                
                return;
            }
            success(result);
        });
    }

}