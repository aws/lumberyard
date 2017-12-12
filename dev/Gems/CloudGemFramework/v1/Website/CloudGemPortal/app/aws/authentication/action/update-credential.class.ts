import { AuthStateAction, AuthStateActionContext, EnumAuthState } from '../authentication.class'
import { BehaviorSubject } from 'rxjs/BehaviorSubject';
import { AwsContext } from 'app/aws/context.class'

declare var AWS: any;

export class UpdateCredentialAction implements AuthStateAction {

    constructor(private context: AwsContext) {

    }

    public handle(subject: BehaviorSubject<AuthStateActionContext>, ...args: any[]): void {
        let sessionToken = args[0];

        if (sessionToken === undefined || sessionToken === null || sessionToken === '') {
            subject.next(<AuthStateActionContext>{
                state: EnumAuthState.USER_CREDENTIAL_UPDATE_FAILED,
                output: ["Cognito session token was undefined."]
            });
            return;
        }

        let logins = {}
        logins['cognito-idp.' + this.context.region + '.amazonaws.com/' + this.context.userPoolId] = sessionToken;
        let creds = new AWS.CognitoIdentityCredentials({
            IdentityPoolId: this.context.identityPoolId,
            Logins: logins
        });

        //CustomRoleArn: "arn:aws:iam::374408220943:role/delta/delta-ProjectOwner-10COF7Z0WUP54",
        AWS.config.credentials.clearCachedId()        
        AWS.config.credentials = creds;
        AWS.config.credentials.expired = true;
        

        AWS.config.credentials.refresh(function (err) {
            if (err) {                
                subject.next(<AuthStateActionContext>{
                    state: EnumAuthState.USER_CREDENTIAL_UPDATE_FAILED,
                    output: [err.code + " : " + err.message]
                });   
                return;
            }
            // Configure the credentials provider to use your identity pool            
            subject.next(<AuthStateActionContext>{
                state: EnumAuthState.USER_CREDENTIAL_UPDATED,
                output: [sessionToken, AWS.config.credentials.identityId]
            });        
        });
    }

}