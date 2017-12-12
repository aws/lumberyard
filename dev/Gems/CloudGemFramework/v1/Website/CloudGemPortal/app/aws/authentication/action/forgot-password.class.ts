import { AuthStateAction, AuthStateActionContext, EnumAuthState } from '../authentication.class';
import { Subject } from 'rxjs/Subject';
import { AwsContext } from 'app/aws/context.class';

declare var AWSCognito: any;

export class ForgotPasswordAction implements AuthStateAction {

    constructor(private context: AwsContext) {

    }

    public handle(subject: Subject<AuthStateActionContext>, ...args: any[]): void {
        let username = args[0];
        if (username === undefined || username === null || username === '') {
            subject.next(<AuthStateActionContext>{
                state: EnumAuthState.FORGOT_PASSWORD_FAILURE,
                output: ["Cognito username was undefined."]
            });
            return;
        }

        var userdata = {
            Username: username.trim(),
            Pool: this.context.cognitoUserPool
        };

        let cognitouser = new AWSCognito.CognitoIdentityServiceProvider.CognitoUser(userdata)

        cognitouser.forgotPassword({
            onSuccess: function (result) {
                subject.next(<AuthStateActionContext>{
                    state: EnumAuthState.FORGOT_PASSWORD_SUCCESS,
                    output: [result]
                });
            },
            onFailure: function (err) {
                subject.next(<AuthStateActionContext>{
                    state: EnumAuthState.FORGOT_PASSWORD_FAILURE,
                    output: [err]
                });
            },
            inputVerificationCode(result) {
                subject.next(<AuthStateActionContext>{
                    state: EnumAuthState.FORGOT_PASSWORD_CONFIRM_CODE,
                    output: [result]
                });
            }
        });
        
    }

}