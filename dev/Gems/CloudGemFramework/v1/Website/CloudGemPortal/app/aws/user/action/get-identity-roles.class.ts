import { UserManagementAction } from '../user-management.class'
import { AwsContext } from 'app/aws/context.class'
import { BehaviorSubject } from 'rxjs/BehaviorSubject';

export class GetUserRolesAction implements UserManagementAction {

    constructor(private context: AwsContext) {

    }

    public handle(success: Function, error: Function, subject: BehaviorSubject<any>, ...args: any[]): void {
        var identityId = args[0];
        if (identityId === undefined || identityId === null || identityId === '') {
            error("Cognito identity Id was undefined.")            
            return;
        } 

        var params = {
            IdentityId: identityId           
        };

        this.context.cognitoIdentity.getIdentityPoolRoles(params, function (err, data) {
            if (err) {
                error(err);
                subject.next(err)
                return;
            }            
            success(data);            
        });
    }

}