import { AuthStateAction, AuthStateActionContext, EnumAuthState } from '../authentication.class';
import { Subject } from 'rxjs/Subject';
import { AwsContext } from 'app/aws/context.class';

declare var AWSCognito: any;

export class ForgotPasswordConfirmNewPasswordAction implements AuthStateAction {

    constructor(private context: AwsContext) {

    }

    public handle(subject: Subject<AuthStateActionContext>, ...args: any[]): void {
        let username = args[0];
        if (username === undefined || username === null || username === '') {
            subject.next(<AuthStateActionContext>{
                state: EnumAuthState.FORGOT_PASSWORD_CONFIRMATION_FAILURE,
                output: ["Cognito username was undefined."]
            });
            return;
        }

        let password = args[1];
        if (password === undefined || password === null || password === '') {
            subject.next(<AuthStateActionContext>{
                state: EnumAuthState.FORGOT_PASSWORD_CONFIRMATION_FAILURE,
                output: ["Cognito password was undefined."]
            });
            return;
        }

        let code = args[2];
        if (code === undefined || code === null || code === '') {
            subject.next(<AuthStateActionContext>{
                state: EnumAuthState.FORGOT_PASSWORD_CONFIRMATION_FAILURE,
                output: ["Cognito verification code was undefined."]
            });
            return;
        }

        var userdata = {
            Username: username.trim(),
            Pool: this.context.cognitoUserPool
        };

        let cognitouser = new AWSCognito.CognitoIdentityServiceProvider.CognitoUser(userdata)

        cognitouser.confirmPassword(code, password.trim(), {
            onSuccess: function () {
                subject.next(<AuthStateActionContext>{
                    state: EnumAuthState.FORGOT_PASSWORD_CONFIRMATION_SUCCESS,
                    output: []
                });
            },
            onFailure: function (err) {
                subject.next(<AuthStateActionContext>{
                    state: EnumAuthState.FORGOT_PASSWORD_CONFIRMATION_FAILURE,
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