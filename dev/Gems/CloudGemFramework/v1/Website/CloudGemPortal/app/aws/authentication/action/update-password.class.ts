import { AuthStateAction, AuthStateActionContext, EnumAuthState } from '../authentication.class'
import { BehaviorSubject } from 'rxjs/BehaviorSubject';
import { AwsContext } from 'app/aws/context.class'

declare var AWSCognito: any;

export class UpdatePasswordAction implements AuthStateAction {

    constructor(private context: AwsContext) {

    }

    public handle(subject: BehaviorSubject<AuthStateActionContext>, ...args: any[]): void {
        let user = args[0];
        if (user === undefined || user === null || user === '') {
            subject.next(<AuthStateActionContext>{
                state: EnumAuthState.PASSWORD_CHANGE_FAILED,
                output: ["Cognito user was undefined."]
            });
            return; 
        }

        let password = args[1]
        if (password === undefined || password === null || password === '') {
            subject.next(<AuthStateActionContext>{
                state: EnumAuthState.PASSWORD_CHANGE_FAILED,
                output: ["Cognito password was undefined."]
            });
            return;
        }

       // let userattributes = args[2];        
        
       // delete userattributes.email_verified;

        // get these details and call
        user.completeNewPasswordChallenge(password, [], {
            onSuccess: function (result) {                       
                subject.next(<AuthStateActionContext>{
                    state: EnumAuthState.PASSWORD_CHANGED,
                    output: [result]
                });     
            },

            onFailure: function (err) {                
                subject.next(<AuthStateActionContext>{
                    state: EnumAuthState.PASSWORD_CHANGE_FAILED,
                    output: [err, user]
                });     
            }
        });
        
    }

}