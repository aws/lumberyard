import { Component, OnInit, ViewChild } from '@angular/core';
import { AwsService } from 'app/aws/aws.service'
import { Authentication, EnumAuthState } from 'app/aws/authentication/authentication.class'
import { Router } from '@angular/router';
import { EnumGroupName } from 'app/aws/user/user-management.class'
import { FormBuilder, FormGroup, Validators } from '@angular/forms';
import { UrlService } from 'app/shared/service/index';
import { ModalComponent } from 'app/shared/component/index';

declare const toastr: any;
declare const cgpBootstrap: any;

export enum EnumLoginState {
    LOGIN = -1,
    FORGOT_PASSWORD = -2,
    ENTER_CODE = -3,
    RESEND_CODE = -4
}

@Component({
    selector: 'login',
    templateUrl: "app/view/authentication/component/authentication.component.html",
    styles: [`
        h1 {
            padding-top: 100px;
        }
       .margin-top-space-lg{
            margin-top: 75px;
        }
        .margin-top-space-sm{
            margin-top: 25px;
        }
    `]
})
export class AuthenticationComponent implements OnInit {    

    private authstates: any;
    private loginstates: any;
    private isFirstTimeLogin: boolean;
    private isFirstTimeModal: boolean;

    private state: number;    

    private userLoginForm: FormGroup;
    private usernameCodeForm: FormGroup;
    private userCodeForm: FormGroup;
    private userCodeResendForm: FormGroup;
    private usernameForm: FormGroup;
    private userChangePasswordForm: FormGroup;    
    private passwordResetForm: FormGroup;
    private userInitiatedWithOutLoginPasswordResetForm: FormGroup;

    private user: any;
    private output: any;

    private error: string;    

    private _subscription: any;

    private _storageKeyHasLoggedIn: string = "cgp-logged-in-user";

    @ViewChild(ModalComponent) modalRef: ModalComponent;


    constructor(private fb: FormBuilder, private aws: AwsService, private urlService: UrlService) {        
        this.userLoginForm = fb.group({            
            'username': [null, Validators.compose([Validators.required])],
            'password': [null, Validators.compose([Validators.required])]                             
        })

        this.userCodeForm = fb.group({            
            'code': [null, Validators.compose([Validators.required])]
        })

        this.usernameForm = fb.group({
            'username': [null, Validators.compose([Validators.required])]            
        })

        this.usernameCodeForm = fb.group({
            'username': [null, Validators.compose([Validators.required])],
            'code': [null, Validators.compose([Validators.required])]
        })

        this.userInitiatedWithOutLoginPasswordResetForm = fb.group({
            'username': [null, Validators.compose([Validators.required])],
            'code': [null, Validators.compose([Validators.required])],
            'password1': [null, Validators.compose([Validators.required])],
            'password2': [null, Validators.compose([Validators.required])]
        }, { validator: this.areEqual })

        this.passwordResetForm = fb.group({            
            'code': [null, Validators.compose([Validators.required])],
            'password1': [null, Validators.compose([Validators.required])],
            'password2': [null, Validators.compose([Validators.required])]
        }, { validator: this.areEqual })

        this.userChangePasswordForm = fb.group({
            'password1': [null, Validators.compose([Validators.required])],
            'password2': [null, Validators.compose([Validators.required])]
        }, { validator: this.areEqual });
    
    }

    ngOnInit() {        
        this.authstates = EnumAuthState;
        this.loginstates = EnumLoginState;
        this.state = EnumLoginState.LOGIN;
        this.dismissFirstTimeModal = this.dismissFirstTimeModal.bind(this);                     
        this.isFirstTimeModal = this.isFirstTimeLogin = (cgpBootstrap.firstTimeUse) && (localStorage.getItem(this._storageKeyHasLoggedIn) === null)         
        this.user = this.aws.context.authentication.user
        
        if (this.user != null)
            this.validateSession(this.aws.context.authentication);           

        this._subscription = this.aws.context.authentication.change.subscribe(context => {            
            if (!(context.state === EnumAuthState.PASSWORD_CHANGE_FAILED || 
                context.state === EnumAuthState.LOGIN_FAILURE ||                 
                context.state === EnumAuthState.ASSUME_ROLE_FAILED || 
                context.state === EnumAuthState.LOGIN_FAILURE_INCORRECT_USERNAME_PASSWORD || 
                context.state === EnumAuthState.FORGOT_PASSWORD_FAILURE || 
                context.state === EnumAuthState.FORGOT_PASSWORD_CONFIRMATION_FAILURE)) {
                this.state = context.state
            }            
            this.output = context.output
            this.error = null            
            if (context.state === EnumAuthState.LOGIN_CONFIRMATION_NEEDED) {
                this.user = context.output[0];
                toastr.success("Enter the confirmation code sent by email.");
            } else if (context.state === EnumAuthState.PASSWORD_CHANGE) {
                this.user = context.output[0];
            } else if (context.state === EnumAuthState.FORGOT_PASSWORD_SUCCESS || context.state === EnumAuthState.FORGOT_PASSWORD_CONFIRMATION_SUCCESS) {
                toastr.success("The password has been updated successfully.");
            } else if (context.state === EnumAuthState.FORGOT_PASSWORD_CONFIRM_CODE) {
                toastr.success("The password reset code has sent.");
            } else if (context.state === EnumAuthState.PASSWORD_CHANGE_FAILED) {
                this.error = context.output[0].message
            } else if (context.state === EnumAuthState.LOGIN_FAILURE) {
                this.error = context.output[0].message
            } else if (context.state === EnumAuthState.FORGOT_PASSWORD_FAILURE) {
                this.error = context.output[0].message
            } else if (context.state === EnumAuthState.FORGOT_PASSWORD_CONFIRMATION_FAILURE) {
                this.error = context.output[0].message
            } else if (context.state === EnumAuthState.LOGGED_IN) {
                localStorage.setItem(this._storageKeyHasLoggedIn, "1")
            } else if (context.output !== undefined && context.output.length == 1 && context.output[0].message) {
                toastr.error(context.output[0].message);
            } 
        })
    }

    ngOnDestroy() {
        this._subscription.unsubscribe();        
    }

    isRequiredEmpty(form: any, item: string): boolean {        
        return (form.controls[item].hasError('required') && form.controls[item].touched) 
    }    

    isNotValid(form: any, item: string): boolean {                        
        return !form.controls[item].valid && form.controls[item].touched
    }    

    login(userform) {        
        this.aws.context.authentication.login(userform.username, userform.password)
    }

    forgotPasswordSendCode(username:string) {
        this.aws.context.authentication.forgotPassword(username)
    }

    confirmNewPassword(username: string, code: string, password: string) {
        this.aws.context.authentication.forgotPasswordConfirmNewPassword(username, code, password)
    }

    setUsername(form: any): boolean {
        if (this.userLoginForm.value.username && form.controls['username']) {
            form.controls['username'].setValue(this.userLoginForm.value.username)
            return true
        }   
        return false
    }

    confirmCode(code) {
        this.aws.context.userManagement.confirmCode(code, this.user.username,
            (result) => {
                toastr.success("Thank-you.  The code has been confirmed.")
                this.state = EnumLoginState.LOGIN;
                this.userLoginForm.value.username = this.user.username
                this.aws.context.authentication.logout();
            },
            (err) => { toastr.error(err.message) }
        )
    }

    dismissFirstTimeModal() {       
        this.isFirstTimeModal = false; 
        localStorage.setItem(this._storageKeyHasLoggedIn, "1")
    }

    updatePassword(password: string) {
        this.aws.context.authentication.updatePassword(this.user, password);        
    }

    resendConfirmationCode(username:string) {
        this.aws.context.userManagement.resendConfirmationCode(username, (result) => {
            toastr.success("A new confirmation code has been sent by email.")
        },
            (err) => { toastr.error(err.message) }
        )
    }   

    areEqual(group: any): void {        
        let pass1 = group.controls['password1']
        let pass2 = group.controls['password2']
        pass1.setErrors(null, true);
        if (pass1.value == pass2.value) {            
            return
        }        
        pass2.setErrors({ error: "invalid" }, true)                
    }

    resetErrorMsg(): void {
        this.error = null
    }

    validateSession(authModule: Authentication ): void {
        this.user.getSession(function (err, session) {
            if (err) {                
                return;
            }            
            if (session.isValid()) {
                authModule.session = session;
                authModule.updateCredentials();
            }

        });
    }
}
