import { UserManagementAction } from '../user-management.class'
import { AwsContext } from 'app/aws/context.class'
import { BehaviorSubject } from 'rxjs/BehaviorSubject';

export class GetUsersAction implements UserManagementAction {

    constructor(private context: AwsContext) {

    }

    public handle(success: Function, error: Function, subject: BehaviorSubject<any>, ...args: any[]): void {
        var userPoolId = args[0];
        if (userPoolId === undefined || userPoolId === null || userPoolId === '') {
            error("Cognito user pool Id was undefined.")
            return;
        } 

        var attributes = args[1];
        var filter = args[2];
        var limit = args[3];
        var paginationToken = args[4];

        var params = {
            UserPoolId: userPoolId       
        };

        if (attributes)
            params['AttributesToGet'] = attributes

        if (filter)
            params['Filter'] = filter

        if (limit)
            params['Limit'] = limit

        if (paginationToken)
            params['PaginationToken'] = paginationToken
                

        this.context.cognitoIdentityService.listUsers(params, function (err, data) {
            if (err) {
                error(err);
                subject.next(err)
            }
            else { success(data); }
        });
    }

}