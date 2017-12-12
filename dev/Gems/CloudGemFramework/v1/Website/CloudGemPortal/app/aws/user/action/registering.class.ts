import { UserManagementAction, EnumGroupName } from '../user-management.class'
import { AwsContext } from 'app/aws/context.class'
import { BehaviorSubject } from 'rxjs/BehaviorSubject';

declare var AWSCognito: any;

export class RegisteringAction implements UserManagementAction {

    constructor(private context: AwsContext) {

    }

    public handle(success: Function, error: Function, subject: BehaviorSubject<any>, ...args: any[]): void {
        let username = args[0];
        if (username === undefined || username === null || username === '') {
            error("Cognito username was undefined.")            
            return;
        }

       
        let password = args[1];
        if (password === undefined || password === null || password === '') {
            error("Cognito password was undefined.")            
            return;
        }

        let email = args[2];
        if (email === undefined || email === null || email === '') {
            error("Cognito email was undefined.")            
            return;
        }

        let group = args[3];
        if (group === undefined || group === null || group === '') {
            error("The user group was undefined.")
            return;
        }

        var paramsForGroup = {
            'GroupName': EnumGroupName[group].toLowerCase(),
            'UserPoolId': this.context.userPoolId,
            'Username': username.trim()
        }

        var paramsForCreate = {            
            'UserPoolId': this.context.userPoolId,
            'Username': username.trim(),
            'UserAttributes': [{
                    'Name': 'email',
                    'Value': email.trim()
                },
                {
                    'Name': 'email_verified',
                    'Value': 'true'
                }
            ],
            'TemporaryPassword': password.trim(),
            'DesiredDeliveryMediums': ["EMAIL"]            
        }


        let cognitoIdentityService = this.context.cognitoIdentityService
        cognitoIdentityService.adminCreateUser(paramsForCreate, function (err, result) {            
            if (err) {
                error(err)
                subject.next(err)
                return;
            }
            cognitoIdentityService.adminAddUserToGroup(paramsForGroup, function (err, result) {
                if (err) {
                    error(err)
                    subject.next(err)
                    return;
                }
                success(result)
            })          
        });
    }

}