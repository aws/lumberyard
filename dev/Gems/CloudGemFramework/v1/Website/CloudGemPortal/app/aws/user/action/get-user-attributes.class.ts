import { UserManagementAction } from '../user-management.class'
import { AwsContext } from 'app/aws/context.class'
import { BehaviorSubject } from 'rxjs/BehaviorSubject';

export class GetUserAttributesTransition implements UserManagementAction {

    constructor(private context: AwsContext) {

    }

    public handle(success: Function, error: Function, subject: BehaviorSubject<any>, ...args: any[]): void {
        let cognitouser = args[0];
        if (cognitouser === undefined || cognitouser === null || cognitouser === '') {
            error("Cognito user was undefined.")
            return;
        }

        cognitouser.getSession(function (err, session) {
            if (err) {
                error(err)
                subject.next(err)
                return;
            }
            cognitouser.getUserAttributes(function (err, attributes) {
                if (err) {
                    error(err);
                    subject.next(err)
                    return;
                }
                success(attributes);
            }
            );
        }
        );
    }

}