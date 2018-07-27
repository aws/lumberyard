import { AuthStateAction, AuthStateActionContext, EnumAuthState } from '../authentication.class'
import { Subject } from 'rxjs/Subject';
import { AwsContext } from 'app/aws/context.class'

declare var AWS: any;

export class LogoutAction implements AuthStateAction {

    constructor(private context: AwsContext) {

    }

    public handle(subject: Subject<AuthStateActionContext>, ...args: any[]): void {
        let user = args[0];
        if (user === undefined || user === null || user === '') {
            subject.next(<AuthStateActionContext>{
                state: EnumAuthState.LOGOUT_FAILURE,
                output: ["Cognito user was undefined."]
            });
            return;
        }

        user.signOut();
        if (AWS.config.credentials != null) {
            AWS.config.credentials.clearCachedId()
            AWS.config.credentials.expired = true;
            AWS.config.credentials = null;
        }
        subject.next(<AuthStateActionContext>{
            state: EnumAuthState.LOGGED_OUT,
            output: [args[0]]
        });        
    }

}