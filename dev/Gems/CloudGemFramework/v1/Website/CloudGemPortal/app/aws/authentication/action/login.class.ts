import { AuthStateAction, AuthStateActionContext, EnumAuthState } from '../authentication.class'
import { Subject } from 'rxjs/Subject';
import { AwsContext } from 'app/aws/context.class'

declare var AWSCognito: any;

export class LoginAction implements AuthStateAction {

    constructor(private context: AwsContext) {

    }

    public handle(subject: Subject<AuthStateActionContext>, ...args:any[]): void {
        let username = args[0];
        if (username === undefined || username === null || username === '') {
            subject.next(<AuthStateActionContext>{
                state: EnumAuthState.LOGIN_FAILURE,
                output: ["Cognito username was undefined."]
            });
            return;
        }

        let password = args[1]
        if (password === undefined || password === null || password === '') {
            subject.next(<AuthStateActionContext>{
                state: EnumAuthState.LOGIN_FAILURE,
                output: ["Cognito password was undefined."]
            });
            return;
        }
        
        var authenticationData = {
            Username: username.trim(),
            Password: password.trim()
        };
        var authenticationDetails = new AWSCognito.CognitoIdentityServiceProvider.AuthenticationDetails(authenticationData);

        var userdata = {
            Username: authenticationData.Username,
            Pool: this.context.cognitoUserPool
        };

        let cognitouser = new AWSCognito.CognitoIdentityServiceProvider.CognitoUser(userdata)
        
        cognitouser.authenticateUser(authenticationDetails, {
            onSuccess: function (result) {
                subject.next(<AuthStateActionContext>{
                    state: EnumAuthState.LOGGED_IN,
                    output: [result]
                });
            },

            onFailure: function (err) {
                if (err.code == "UserNotConfirmedException" && err.name == "NotAuthorizedException") {
                    subject.next(<AuthStateActionContext>{
                        state: EnumAuthState.LOGIN_FAILURE_INCORRECT_USERNAME_PASSWORD,
                        output: [cognitouser]
                    });
                    return;
                }

                if (err.code == "PasswordResetRequiredException") {
                    subject.next(<AuthStateActionContext>{
                        state: EnumAuthState.PASSWORD_RESET_BY_ADMIN_CONFIRM_CODE,
                        output: [cognitouser]
                    });
                    return;
                }

                if (err.code == "UserNotConfirmedException") {
                    subject.next(<AuthStateActionContext>{
                        state: EnumAuthState.LOGIN_CONFIRMATION_NEEDED,
                        output: [cognitouser]
                    });
                    return; 
                }
                subject.next(<AuthStateActionContext>{
                    state: EnumAuthState.LOGIN_FAILURE,
                    output: [err]
                });
            },

            newPasswordRequired: function (userattributes, requiredattributes) {
                subject.next(<AuthStateActionContext>{
                    state: EnumAuthState.PASSWORD_CHANGE,
                    output: [cognitouser, userattributes, requiredattributes]
                });
            }
        });
    }

}