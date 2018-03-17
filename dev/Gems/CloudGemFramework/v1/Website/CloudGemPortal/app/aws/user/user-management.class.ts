import { Injectable} from '@angular/core';
import { AwsContext } from 'app/aws/context.class';
import { ResendConfirmationCodeAction } from './action/resend-confirmation-code.class'
import { GetUserRolesAction } from './action/get-identity-roles.class'
import { GetUserAttributesTransition } from './action/get-user-attributes.class'
import { RegisteringAction } from './action/registering.class'
import { GetUsersAction } from './action/list-users.class'
import { ConfirmingCodeAction } from './action/confirming-code.class'
import { UpdatePasswordAction } from './action/update-password.class'
import { GetUsersByGroupAction } from './action/list-users-in-group.class'
import { DeleteUserAction } from './action/delete-user.class'
import { ResetPasswordAction } from './action/reset-password.class'
import { Router } from "@angular/router";
import { Observable } from 'rxjs/Observable';
import { BehaviorSubject } from 'rxjs/BehaviorSubject';
import { LyMetricService, Event } from 'app/shared/service/index';

export enum EnumUserManagementAction {
    REGISTERING,
    GET_USER_ATTRIBUTE,
    RESEND_CONFIRMATION_CODE,
    GET_ROLES,
    GET_USERS,
    CONFIRM_CODE,
    UPDATE_PASSWORD,
    GET_USERS_BY_GROUP,
    DELETE_USER,
    RESET_PASSWORD
}

export enum EnumGroupName {
    ADMINISTRATOR,
    USER
}

export interface UserManagementAction {
    handle(success: Function, error: Function, subject: BehaviorSubject<any>, ...args: any[]): void
}

export class UserManagement {

    private _actions = [];
    private _error: BehaviorSubject<any>;
    private _identifier = "User Administration"

    constructor(private context: AwsContext, router: Router, private metric: LyMetricService = null) {
        this._error = new BehaviorSubject<any>(<any>{});
        this._error.subscribe(err => {            
            if (err.code == "CredentialsError") {               
                this.context.authentication.refreshSessionOrLogout();
            }
        })

        this._actions[EnumUserManagementAction.REGISTERING] = new RegisteringAction(this.context)
        this._actions[EnumUserManagementAction.GET_USER_ATTRIBUTE] = new GetUserAttributesTransition(this.context)        
        this._actions[EnumUserManagementAction.RESEND_CONFIRMATION_CODE] = new ResendConfirmationCodeAction(this.context)
        this._actions[EnumUserManagementAction.GET_ROLES] = new GetUserRolesAction(this.context)
        this._actions[EnumUserManagementAction.GET_USERS] = new GetUsersAction(this.context)
        this._actions[EnumUserManagementAction.CONFIRM_CODE] = new ConfirmingCodeAction(this.context)
        this._actions[EnumUserManagementAction.UPDATE_PASSWORD] = new UpdatePasswordAction(this.context)
        this._actions[EnumUserManagementAction.GET_USERS_BY_GROUP] = new GetUsersByGroupAction(this.context)
        this._actions[EnumUserManagementAction.DELETE_USER] = new DeleteUserAction(this.context)
        this._actions[EnumUserManagementAction.RESET_PASSWORD] = new ResetPasswordAction(this.context)
    }   

    public getUserRoles(success: Function, error: Function): void {
        this.execute(EnumUserManagementAction.GET_ROLES, success, error, this.context.authentication.identityId);
    }

    public getUserAttributes(user: any, success: Function, error: Function): void {
        this.execute(EnumUserManagementAction.GET_USER_ATTRIBUTE, success, error, user);
    }

    public resendConfirmationCode(username: string, success: Function, error: Function): void {
        this.execute(EnumUserManagementAction.RESEND_CONFIRMATION_CODE, success, error, username);
    }

    public register(username: string, password: string, email: string, group: EnumGroupName, success: Function, error: Function): void {
        this.execute(EnumUserManagementAction.REGISTERING, success, error, username, password, email, group);
    }

    public getAllUsers(success: Function, error: Function, attributestoget: string[] = [], filter?: string, limit?: number, paginationtoken?: string): void {
        this.execute(EnumUserManagementAction.GET_USERS, success, error, this.context.userPoolId, attributestoget, filter, limit, paginationtoken);
    }

    public updatePassword(username: string, password: string, userattributes: any, success: Function, error: Function): void {
        this.execute(EnumUserManagementAction.UPDATE_PASSWORD, success, error, username, password, userattributes);
    }

    public confirmCode(code: string, username: string, success: Function, error: Function): void {
        this.execute(EnumUserManagementAction.CONFIRM_CODE, success, error, code, username);
    }

    public getAdministrators(success: Function, error: Function, limit?: number, paginationtoken?: string): void {
        this.execute(EnumUserManagementAction.GET_USERS_BY_GROUP, success, error, this.context.userPoolId, EnumGroupName[EnumGroupName.ADMINISTRATOR].toLowerCase(), limit, paginationtoken);
    }

    public getUsers(success: Function, error: Function, limit?: number, paginationtoken?: string): void {
        this.execute(EnumUserManagementAction.GET_USERS_BY_GROUP, success, error, this.context.userPoolId, EnumGroupName[EnumGroupName.USER].toLowerCase(), limit, paginationtoken);
    }

    public getUserByGroupName(groupName: string, success: Function, error: Function, limit?: number, paginationtoken?: string): void {
        this.execute(EnumUserManagementAction.GET_USERS_BY_GROUP, success, error, this.context.userPoolId, groupName, limit, paginationtoken);
    }

    public deleteUser(username: string, success: Function, error: Function): void {
        this.execute(EnumUserManagementAction.DELETE_USER, success, error, username);
    }

    public resetPassword(username: string, success: Function, error: Function): void {
        this.execute(EnumUserManagementAction.RESET_PASSWORD, success, error, username);
    }

    private execute(transition: EnumUserManagementAction, success_callback: Function, error_callback: Function, ...args: any[]): void {
        
        this.recordEvent("ApiServiceRequested", {
            "Identifier": this._identifier ,
            "Verb": "GET",
            "Path": EnumUserManagementAction[transition].toString()
        }, null);
        this._actions[transition].handle((result) => {
            this.recordEvent("ApiServiceSuccess", {
                "Identifier": this._identifier,
                "Path": EnumUserManagementAction[transition].toString()
            }, null);
            success_callback(result)
        }, (err)=>{
            this.recordEvent("ApiServiceError", {
                "Message": err.message,
                "Identifier": this._identifier,
                "Path": EnumUserManagementAction[transition].toString()
            }, null);
            error_callback(err)
        }, this._error, ...args);
    }

    public recordEvent(eventName: Event, attribute: { [name: string]: string; } = {}, metric: { [name: string]: number; } = null) {
        if (this.metric !== null)
            this.metric.recordEvent(eventName, attribute, metric);
    }
   
}