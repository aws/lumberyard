import { Injectable } from '@angular/core';
import { Observable } from 'rxjs/Observable';
import { BehaviorSubject } from 'rxjs/BehaviorSubject';
import { AwsContext } from 'app/aws/context.class'
import { LoginAction } from './action/login.class'
import { LogoutAction } from './action/logout.class'
import { AssumeRoleAction } from './action/assume-role.class'
import { UpdateCredentialAction } from './action/update-credential.class'
import { UpdatePasswordAction } from './action/update-password.class'
import { ForgotPasswordAction } from './action/forgot-password.class'
import { ForgotPasswordConfirmNewPasswordAction } from './action/forgot-password-confirm-password.class'
import { EnumGroupName } from 'app/aws/user/user-management.class';
 
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
    handle(subject: BehaviorSubject<AuthStateActionContext>, ...args: any[]): void
}

export class Authentication {
 
    private _actionObservable: BehaviorSubject<AuthStateActionContext>;
    // store the URL so we can redirect after logging in
    private _isInitialized: boolean;
    private _isSessionExpired: boolean;
    private _actions = [];
    private _session: any;
    private _identityId: string;
     
    get change(): Observable<AuthStateActionContext> {
        return this._actionObservable.asObservable();
    }
 
    get isSessionExpired() {
        return this._isSessionExpired;
    }
 
    get user(): any {
        return this.context.cognitoUserPool.getCurrentUser();
    }
 
    get accessToken(): string {
        if (this._session === undefined)
            return;
        return this._session.getAccessToken().getJwtToken();
    }
 
    get refreshToken(): string {
        if (this._session === undefined)
            return;
        return this._session.getRefreshToken();
    }
 
    get idToken(): string {
        if (this._session === undefined)
            return;
        return this._session.getIdToken().getJwtToken();
    }
 
    set isSessionExpired(value: boolean) {
        this._isSessionExpired = value;
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

    get identityId(): string {
        return this._identityId;
    }

    get isAdministrator(): boolean {        
        let obj = this.parseJwt(this.idToken)        
        let admin_group = obj['cognito:groups'].filter(group => group == EnumGroupName[EnumGroupName.ADMINISTRATOR].toLowerCase())        
        return admin_group.length > 0
    }
        
    public constructor(private context: AwsContext) {
        this._actions[EnumAuthStateAction.LOGIN] = new LoginAction(this.context)
        this._actions[EnumAuthStateAction.LOGOUT] = new LogoutAction(this.context)
        this._actions[EnumAuthStateAction.UPDATE_CREDENTIAL] = new UpdateCredentialAction(this.context)        
        this._actions[EnumAuthStateAction.ASSUME_ROLE] = new AssumeRoleAction(this.context)
        this._actions[EnumAuthStateAction.UPDATE_PASSWORD] = new UpdatePasswordAction(this.context)        
        this._actions[EnumAuthStateAction.FORGOT_PASSWORD] = new ForgotPasswordAction(this.context)
        this._actions[EnumAuthStateAction.FORGOT_PASSWORD_CONFIRM_NEW_PASSWORD] = new ForgotPasswordConfirmNewPasswordAction(this.context)        

        this._actionObservable = new BehaviorSubject<AuthStateActionContext>(<AuthStateActionContext>{});
        this.change.subscribe(context => {            
            if (context.state === EnumAuthState.LOGGED_IN
                || context.state === EnumAuthState.PASSWORD_CHANGED               
                ) {
                this._session = context.output[0];                
                this.updateCredentials()
            } else if (context.state === EnumAuthState.LOGGED_OUT) {
                this._session = undefined;                
            } else if (context.state === EnumAuthState.USER_CREDENTIAL_UPDATED) {
                this._identityId = context.output[1];                
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

    public logout(): void {
        this.execute(EnumAuthStateAction.LOGOUT, this.user);
    }
 
    public assumeRole(rolearn: string): void {
        this.execute(EnumAuthStateAction.ASSUME_ROLE, rolearn, this.identityId, this.idToken);
    }
 
    public updatePassword(user: any, password: string): void {
        this.execute(EnumAuthStateAction.UPDATE_PASSWORD, user, password);
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