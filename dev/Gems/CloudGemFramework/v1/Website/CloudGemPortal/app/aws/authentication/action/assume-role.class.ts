import { AuthStateAction, AuthStateActionContext, EnumAuthState } from '../authentication.class'
import { Subject } from 'rxjs/Subject';
import { AwsContext } from 'app/aws/context.class'

declare var AWS: any;

export class AssumeRoleAction implements AuthStateAction {

    constructor(private context: AwsContext) {

    }

    public handle(subject: Subject<AuthStateActionContext>, ...args: any[]): void {
        var rolearn = args[0];
        if (rolearn === undefined || rolearn === null || rolearn === '') {
            subject.next(<AuthStateActionContext>{
                state: EnumAuthState.ASSUME_ROLE_FAILED,
                output: ["Cognito role ARN was undefined."]
            });
            return;
        }    
        
        var identityId = args[1];
        if (identityId === undefined || identityId === null || identityId === '') {
            subject.next(<AuthStateActionContext>{
                state: EnumAuthState.ASSUME_ROLE_FAILED,
                output: ["Cognito identity Id was undefined."]
            });
            return;
        }  

        var idToken = args[2];
        if (idToken === undefined || idToken === null || idToken === '') {
            subject.next(<AuthStateActionContext>{
                state: EnumAuthState.ASSUME_ROLE_FAILED,
                output: ["Cognito identity Id was undefined."]
            });
            return;
        }  

        let logins = {}
        logins['cognito-idp.' + this.context.region + '.amazonaws.com/' + this.context.userPoolId] = idToken;

        var params = {
            IdentityId: identityId,
            CustomRoleArn: rolearn,
            Logins: {
                ['cognito-idp.' + this.context.region + '.amazonaws.com/' + this.context.userPoolId]: idToken
            }
        };

        console.log(params)
        var cognitoidentity = new AWS.CognitoIdentity();
        cognitoidentity.getCredentialsForIdentity(params, function (err, data) {
            if (err) {
                subject.next(<AuthStateActionContext>{
                    state: EnumAuthState.ASSUME_ROLE_FAILED,
                    output: [err]
                });
                return;
            }
            console.log(data);
            subject.next(<AuthStateActionContext>{
                state: EnumAuthState.ASSUME_ROLE_SUCCESS,
                output: [data]
            });    
        });
    }

}