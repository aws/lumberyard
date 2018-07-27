import { Injectable } from '@angular/core';
import { Observable } from 'rxjs/Observable';
import { Subscription } from 'rxjs/Subscription';
import { Subscriber } from 'rxjs/Subscriber';
import { Subject } from 'rxjs/Subject';
import { AwsContext } from 'app/aws/context.class'
import { LoginAction } from './action/login.class'
import { LogoutAction } from './action/logout.class'
import { AssumeRoleAction } from './action/assume-role.class'
import { UpdateCredentialAction } from './action/update-credential.class'
import { UpdatePasswordAction } from './action/update-password.class'
import { ForgotPasswordAction } from './action/forgot-password.class'
import { ForgotPasswordConfirmNewPasswordAction } from './action/forgot-password-confirm-password.class'
import { EnumGroupName } from 'app/aws/user/user-management.class';
import { LyMetricService } from 'app/shared/service/index';
import { DateTimeUtil } from 'app/shared/class/index';
import { Router } from '@angular/router';

declare var AWS;
 
export enum EnumAuthState {
    INITIALIZED,
    LOGGED_IN,
    LOGGED_OUT,
    FORGOT_PASSWORD,    
    PASSWORD_CHANGE,
    PASSWORD_CHANGE_FAILED,
    PASSWORD_CHANGED,    
    LOGIN_FAILURE,    
    USER_CREDENTIAL_UPDATED,
    USER_CREDENTIAL_UPDATE_FAILED,
    LOGIN_CONFIRMATION_NEEDED,    
    LOGOUT_FAILURE,
    ASSUME_ROLE_SUCCESS,
    ASSUME_ROLE_FAILED,
    LOGIN_FAILURE_INCORRECT_USERNAME_PASSWORD,
    FORGOT_PASSWORD_CONFIRM_CODE,
    FORGOT_PASSWORD_FAILURE,
    FORGOT_PASSWORD_SUCCESS,
    FORGOT_PASSWORD_CONFIRMATION_FAILURE,
    FORGOT_PASSWORD_CONFIRMATION_SUCCESS,
    PASSWORD_RESET_BY_ADMIN_CONFIRM_CODE
}
 
export enum EnumAuthStateAction {
    LOGIN,
    LOGOUT,
    UPDATE_PASSWORD,        
    UPDATE_CREDENTIAL,    
    ASSUME_ROLE,
    FORGOT_PASSWORD,
    FORGOT_PASSWORD_CONFIRM_NEW_PASSWORD      
}
 
export interface AuthResponseObject {
    status: boolean
    message: string
}
 
export interface AuthStateActionContext {
    state: EnumAuthState,
    output?: any[]
}
 
export interface AuthStateAction {
    handle(subject: Subject<AuthStateActionContext>, ...args: any[]): void
}

export class Authentication {
 
    private _actionObservable: Subject<AuthStateActionContext>;
    // store the URL so we can redirect after logging in
    private _isInitialized: boolean;
    private _isSessionExpired: boolean;
    private _actions = [];
    private _session: any;
    private _identityId: string; 
    private _user: any    

    get change(): Observable<AuthStateActionContext> {
        return this._actionObservable.asObservable();
    }

    get user(): any {        
        return this._user
    }
 
    get accessToken(): string {
        if (this._session === undefined)
            return;
        return this._session.getAccessToken().getJwtToken();
    }
 
    get idToken(): string {
        if (this._session === undefined)
            return;
        return this._session.getIdToken().getJwtToken();
    }

    get refreshToken(): string {
        if (this._session === undefined)
            return;
        return this._session.getRefreshToken().getJwtToken();
    }

    get refreshCognitoToken(): string {
        if (this._session === undefined)
            return;
        return this._session.getRefreshToken();
    }

    get isLoggedIn(): boolean {
        return this.idToken ? true : false;
    }
 
    get session(): any {
        return this._session;
    }

    set session(value:any) {
        this._session = value;
    }

    get isSessionValid(): boolean {
        return this._session != null && this._session.isValid()
    }

    get identityId(): string {
        return this._identityId;
    }
    

    get isAdministrator(): boolean {        
        let obj = this.parseJwt(this.idToken)        
        let admin_group = obj['cognito:groups'].filter(group => group == EnumGroupName[EnumGroupName.ADMINISTRATOR].toLowerCase())        
        return admin_group.length > 0
    }
        
    public constructor(private context: AwsContext, private router: Router, private metric: LyMetricService) {
        this._actions[EnumAuthStateAction.LOGIN] = new LoginAction(this.context)
        this._actions[EnumAuthStateAction.LOGOUT] = new LogoutAction(this.context)
        this._actions[EnumAuthStateAction.UPDATE_CREDENTIAL] = new UpdateCredentialAction(this.context)        
        this._actions[EnumAuthStateAction.ASSUME_ROLE] = new AssumeRoleAction(this.context)
        this._actions[EnumAuthStateAction.UPDATE_PASSWORD] = new UpdatePasswordAction(this.context)        
        this._actions[EnumAuthStateAction.FORGOT_PASSWORD] = new ForgotPasswordAction(this.context)
        this._actions[EnumAuthStateAction.FORGOT_PASSWORD_CONFIRM_NEW_PASSWORD] = new ForgotPasswordConfirmNewPasswordAction(this.context)        
        this.refreshSessionOrLogout = this.refreshSessionOrLogout.bind(this)         
        this.hookUpdateCredentials = this.hookUpdateCredentials.bind(this)
        this.updateCredentials = this.updateCredentials.bind(this)        
        this._actionObservable = new Subject<AuthStateActionContext>();
        this.change.subscribe(context => {            
            if (context.state === EnumAuthState.LOGGED_IN
                || context.state === EnumAuthState.PASSWORD_CHANGED               
            ) {                
                this._session = context.output[0];                                
                this.updateCredentials()
            } else if (context.state === EnumAuthState.LOGGED_OUT ) {
                this._session = undefined;                
                this.router.navigate(["/login"]);
            } else if (context.state === EnumAuthState.USER_CREDENTIAL_UPDATED) {
                this._identityId = context.output[1];               
                this._user = this.context.cognitoUserPool.getCurrentUser(); 
                this.user.refreshSession = this.user.refreshSession.bind(this) 
            }
        })
    }    

    public updateCredentials(): void {
        this.execute(EnumAuthStateAction.UPDATE_CREDENTIAL, this._session.getIdToken().getJwtToken());       
    }

    public login(username: string, password: string): void {
        this.execute(EnumAuthStateAction.LOGIN, username, password);
    }

    public forgotPassword(username: string): void {
        this.execute(EnumAuthStateAction.FORGOT_PASSWORD, username);
    }

    public forgotPasswordConfirmNewPassword(username: string, code:string, password: string): void {
        this.execute(EnumAuthStateAction.FORGOT_PASSWORD_CONFIRM_NEW_PASSWORD, username, password, code);
    }

    public logout(isforced: boolean = false): void {
        this.metric.recordEvent("LoggedOut", {
            "ForcedLogout": isforced.toString(),
            "Class": this.constructor["name"]
        }, null);        
        this.execute(EnumAuthStateAction.LOGOUT, this.user);
    }
 
    public assumeRole(rolearn: string): void {
        this.execute(EnumAuthStateAction.ASSUME_ROLE, rolearn, this.identityId, this.idToken);
    }
 
    public updatePassword(user: any, password: string): void {
        this.execute(EnumAuthStateAction.UPDATE_PASSWORD, user, password);
    }

    public refreshSessionOrLogout(observable: Observable<any> = null): Observable<any> {                
        let auth = this
        return new Observable<any>(observer => {
            //no current user is present
            //BUG: currently the local cookies for the user are deleted somewhere after 30s when logged in.
            if (!auth.user) {
                auth.logout(true);                
            } else {
                // validate the session                           
                if (auth.isSessionValid) {
                    auth.executeObservableOrLogout(auth, observer, observable)                    
                } else {
                    //this will cause the session to generate new tokens for expired ones
                    let context = auth.context; 
                    let auth2 = auth
                    try {
                        auth.user.refreshSession(auth.refreshCognitoToken, function (err, session) {
                            if (err) {
                                console.error(err)
                                auth2.logout(true)
                                return;
                            }
                            auth2.session = session;

                            auth2.hookUpdateCredentials(auth2, (response) => {
                                auth2.context.initializeServices();
                                auth2.executeObservableOrLogout(auth2, observer, observable)
                            }, (error) => {
                                console.error(error)
                                auth2.logout(true);
                            })
                            auth2.updateCredentials();
                        })
                    } catch(e) {
                        auth2.logout(true)
                    }
                }
            }
        })
    }

    private hookUpdateCredentials(auth, success, failure): void {
        let subscription = auth.change.subscribe(context => {
            if (context.state === EnumAuthState.USER_CREDENTIAL_UPDATED || 
                context.state === EnumAuthState.USER_CREDENTIAL_UPDATE_FAILED)
                subscription.unsubscribe();

            if (context.state === EnumAuthState.USER_CREDENTIAL_UPDATED) {
                success(context.output)
            } else if (context.state === EnumAuthState.USER_CREDENTIAL_UPDATE_FAILED) {
                failure(context.output)
            }
        });
    }

    private executeObservableOrLogout(auth: Authentication, observer: Subscriber<any>, observable: Observable<any>): void {
        if (auth.isSessionValid && observer && observable) {
            observer.next(observable)            
        } else {
            auth.logout(true);
        }   
    }

    private execute(transition: EnumAuthStateAction, ...args: any[]): void {
        this._actions[transition].handle(this._actionObservable, ...args);
    }

    private parseJwt(token) {
        var base64Url = token.split('.')[1];
        var base64 = base64Url.replace('-', '+').replace('_', '/');
        return JSON.parse(atob(base64));
    };
}