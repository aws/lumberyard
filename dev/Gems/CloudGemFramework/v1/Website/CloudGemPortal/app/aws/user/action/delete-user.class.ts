import { UserManagementAction } from '../user-management.class'
import { AwsContext } from 'app/aws/context.class'
import { BehaviorSubject } from 'rxjs/BehaviorSubject';

export class DeleteUserAction implements UserManagementAction {

    constructor(private context: AwsContext) {

    }

    public handle(success: Function, error: Function, subject: BehaviorSubject<any>, ...args: any[]): void {
        var username = args[0];
        if (username === undefined || username === null || username === '') {
            error("Cognito username was undefined.")
            return;
        } 

        var params = {
            UserPoolId: this.context.userPoolId,
            Username: username.trim() /* required */
        };

        this.context.cognitoIdentityService.adminDeleteUser(params, function (err, data) {
            if (err) {
                error(err);
                subject.next(err)
            }
            else { success(data); }
        });
    }

}