import { AuthStateAction, AuthStateActionContext, EnumAuthState } from '../authentication.class'
import { Subject } from 'rxjs/Subject';
import { AwsContext } from 'app/aws/context.class'

declare var AWS: any;

export class UpdateCredentialAction implements AuthStateAction {

    constructor(private context: AwsContext) {

    }

    public handle(subject: Subject<AuthStateActionContext>, ...args: any[]): void {
        let idToken = args[0];

        if (idToken === undefined || idToken === null || idToken === '') {
            subject.next(<AuthStateActionContext>{
                state: EnumAuthState.USER_CREDENTIAL_UPDATE_FAILED,
                output: ["Cognito session token was undefined."]
            });
            return;
        }

        let logins = {}
        logins['cognito-idp.' + this.context.region + '.amazonaws.com/' + this.context.userPoolId] = idToken;
        let creds = new AWS.CognitoIdentityCredentials({
            IdentityPoolId: this.context.identityPoolId,
            Logins: logins
        });        

        AWS.config.credentials = creds;

        var callback = function (err) {
            if (err || AWS.config.credentials == null) {
                subject.next(<AuthStateActionContext>{
                    state: EnumAuthState.USER_CREDENTIAL_UPDATE_FAILED,
                    output: [err.code + " : " + err.message]
                });
                return;
            }            
            subject.next(<AuthStateActionContext>{
                state: EnumAuthState.USER_CREDENTIAL_UPDATED,
                output: [idToken, AWS.config.credentials.identityId, AWS.config.credentials]
            });
        }

        if (AWS.config.credentials == null) {
            subject.next(<AuthStateActionContext>{
                state: EnumAuthState.USER_CREDENTIAL_UPDATE_FAILED,
                output: ["Credentials no longer exist."]
            });
        } else {
            AWS.config.credentials.refresh((err) => {
                if (AWS.config.credentials.accessKeyId === undefined) {
                    AWS.config.credentials.refresh((err) => {
                        callback(err);
                    })
                    return;
                }
                callback(err)
            });
        }
    }

}