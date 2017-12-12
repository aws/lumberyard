import { UserManagementAction } from '../user-management.class'
import { AwsContext } from 'app/aws/context.class'
import { BehaviorSubject } from 'rxjs/BehaviorSubject';

export class GetUsersByGroupAction implements UserManagementAction {

    constructor(private context: AwsContext) {

    }

    public handle(success: Function, error: Function, subject: BehaviorSubject<any>, ...args: any[]): void {
        var userPoolId = args[0];
        if (userPoolId === undefined || userPoolId === null || userPoolId === '') {
            error("Cognito user pool Id was undefined.")
            return;
        } 
        var groupName = args[1];
        if (groupName === undefined || groupName === null || groupName === '') {
            error("Cognito group name was undefined.")
            return;
        } 

        var limit = args[2];
        var paginationToken = args[3];

        var params = {
            UserPoolId: userPoolId,
            GroupName: groupName
        };
      
        if (limit)
            params['Limit'] = limit

        if (paginationToken)
            params['NextToken'] = paginationToken
                

        this.context.cognitoIdentityService.listUsersInGroup(params, function (err, data) {
            if (err) {
                error(err);
                subject.next(err)
            }
            else { success(data); }
        });
    }

}