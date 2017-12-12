import { AuthStateAction, AuthStateActionContext, EnumAuthState } from '../authentication.class'
import { BehaviorSubject } from 'rxjs/BehaviorSubject';
import { AwsContext } from 'app/aws/context.class'

declare var AWS: any;

export class LogoutAction implements AuthStateAction {

    constructor(private context: AwsContext) {

    }

    public handle(subject: BehaviorSubject<AuthStateActionContext>, ...args: any[]): void {
        let user = args[0];
        if (user === undefined || user === null || user === '') {
            subject.next(<AuthStateActionContext>{
                state: EnumAuthState.LOGOUT_FAILURE,
                output: ["Cognito user was undefined."]
            });
            return;
        }

        user.signOut();
        AWS.config.credentials.clearCachedId();
        subject.next(<AuthStateActionContext>{
            state: EnumAuthState.LOGGED_OUT,
            output: [args[0]]
        });        
    }

}