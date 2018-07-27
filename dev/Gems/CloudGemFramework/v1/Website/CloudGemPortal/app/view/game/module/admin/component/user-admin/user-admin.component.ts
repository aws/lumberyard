import { Component, OnInit, ChangeDetectorRef, ViewChild } from '@angular/core';
import { FormBuilder, FormGroup, Validators } from '@angular/forms';
import { EnumGroupName } from 'app/aws/user/user-management.class';
import { AwsService } from 'app/aws/aws.service'
import { ModalComponent } from 'app/shared/component/index';
import { ActionItem } from "app/view/game/module/shared/class/index";
import { ToastsManager } from 'ng2-toastr';
import { LyMetricService } from 'app/shared/service/index';
import * as environment from 'app/shared/class/index';

export enum EnumUserAdminModes {
    Default,
    AddUser,
    DeleteUser,
    EditRole, 
    ResetPassword
}

export enum EnumAccountStatus {
    ACTIVE,
    PENDING,
    UNCONFIRMED,
    ARCHIVED,
    CONFIRMED,
    COMPROMISED,
    RESET_REQUIRED,
    FORCE_CHANGE_PASSWORD,
    DISABLED       
}

@Component({
    selector: 'admin',
    templateUrl: 'app/view/game/module/admin/component/user-admin/user-admin.component.html',
    styles: [
        `
        .add-row {
            margin-bottom: 25px;
        }

        h2.user-count {
            margin-left: 10px;
        }
        `
    ]
})
export class UserAdminComponent implements OnInit {
    public get metricIdentifier(): string{
        return environment.metricWhiteListedFeature[environment.metricAdminIndex]
    }
    awsCognitoLink = "https://console.aws.amazon.com/cognito/home"
    missingEmailString = "n/a"

    currentTabIndex: number = 0;
    userSortDescending: boolean = true;
    roleSortDescending: boolean = true;
    searchDropdownOptions: { text: string }[];
    currentUserSearchDropdown: string;

    currentUser: any;
    currentRole: Role;
    rolesList: Role[] = [];

    currentMode: number = 0;
    userAdminModesEnum = EnumUserAdminModes;
    userState = EnumAccountStatus;

    allUsers: any[] = [];
    userList: any[] = [];
    original_allUsers: any[] = [];
    loadingUsers: boolean = false;

    action: ActionItem = new ActionItem("Reset Password", this.resetPasswordView)
    stubActions: ActionItem[] = [this.action];

    userValidationError: string = "";

    numUserPages: number = 0;
    currentStartIndex: number = 0;
    pageSize: number = 10;    

    enumUserGroup = EnumGroupName;

    @ViewChild(ModalComponent) modalRef: ModalComponent;

    userForm: FormGroup;
    constructor(fb: FormBuilder,
        private aws: AwsService,
        private cdr: ChangeDetectorRef,
        private toastr: ToastsManager, 
        private metric: LyMetricService
    ) {        
        this.userForm = fb.group({
            'username': [null, Validators.compose([Validators.required])],
            'email': [null, Validators.compose([Validators.required])],
            'password': [this.passwordGenerator(), Validators.compose([Validators.required])],            
            'role': EnumGroupName.USER
        })
    }

    ngOnInit() {

        // bind function scopes
        this.editRoleModal = this.editRoleModal.bind(this);
        this.dismissUserModal = this.dismissUserModal.bind(this);
        this.dismissRoleModal = this.dismissRoleModal.bind(this);
        this.addUser = this.addUser.bind(this);
        this.deleteUser = this.deleteUser.bind(this);
        this.updateUserList = this.updateUserList.bind(this);
        this.updateUserPage = this.updateUserPage.bind(this);
        this.resetPasswordView = this.resetPasswordView.bind(this);
        this.action.onClick = this.action.onClick.bind(this);
        this.resetPassword = this.resetPassword.bind(this);
        this.deleteUserView = this.deleteUserView.bind(this);
        // end of binding function scopes

        this.searchDropdownOptions = [{ text: "Username" }, { text: "Email" }]

        // alert change detection tree when form is updated
        this.userForm.valueChanges.subscribe(() => {
            this.cdr.detectChanges();
        })

        // Roles list isn't modifiable for now
        this.rolesList.push(new Role("Portal Admin", 1));
        this.rolesList.push(new Role("User", 1));
        
        this.updateUserList();
    }

    isRequiredEmpty(form: any, item: string): boolean {
        return (form.controls[item].hasError('required') && form.controls[item].touched)
    }

    isNotValid(form: any, item: string): boolean {
        return !form.controls[item].valid && form.controls[item].touched
    }  
      
    updateUserList() {
        this.cdr.detectChanges()
        this.currentMode = EnumUserAdminModes.Default;
        this.loadingUsers = true;
        let users = []
        this.aws.context.userManagement.getAdministrators(result => {
            for (let i = 0; i < result.Users.length; i++) {
                this.appendUser(result.Users[i], this.rolesList[0].name, users)
            }
            this.rolesList[0].numUsers = result.Users.length;
            this.aws.context.userManagement.getUsers(result => {
                for (let i = 0; i < result.Users.length; i++) {
                    this.appendUser(result.Users[i], this.rolesList[1].name, users)
                }
                this.rolesList[1].numUsers = result.Users.length;
                users.sort((user1, user2) => {
                    if (user1.Username > user2.Username)
                        return 1;
                    if (user1.Username < user2.Username)
                        return -1;
                    return 0;
                })
                this.setAllUsers(users);
                this.loadingUsers = false;
            }, err => {
                this.toastr.error(err.message);
            });

        },
            err => {
                this.toastr.error(err.message);
            });
    }

    updateUserPage(pageNum?: number) {
        // return a subset of all users for the current page
        if (!pageNum) pageNum = 1;
        this.userList = this.allUsers.slice((pageNum - 1) * this.pageSize, ((pageNum - 1) * this.pageSize) + this.pageSize);
    }

    updateSubNav(tabIndex: number) {
        this.currentTabIndex = tabIndex;

    }

    editRoleModal(model) {
        this.currentMode = EnumUserAdminModes.EditRole;
        this.currentRole = model;
    }

    changeUserSortDirection() {
        this.userSortDescending = !this.userSortDescending
        this.setAllUsers(this.allUsers.reverse())        
    }

    changeRoleSortDirection() {
        this.roleSortDescending = !this.roleSortDescending;
        if (this.roleSortDescending) {
            this.rolesList = this.rolesList.sort(Role.sortByNameDescending);
        } else {
            this.rolesList = this.rolesList.sort(Role.sortByNameDescending).reverse();
        }
    }

    deleteUserView(user): void {
        this.currentUser = user;
        this.currentMode = EnumUserAdminModes.DeleteUser
    }

    resetPasswordView(user: any): void {
        this.currentUser = user;
        this.currentMode = EnumUserAdminModes.ResetPassword
    }

    resetPassword(user: any): void {        
        this.aws.context.userManagement.resetPassword(user.Username, result => {
            this.modalRef.close();
            this.toastr.success("The user '" + user.Username + "' password has been reset");            
            this.updateUserList();
        }, err => {
            this.modalRef.close();
            this.toastr.error(err.message);            
        })
    }

    setDefaultUserFormValues() {
        this.userForm.reset();
        // initialize form group values if needed        
        this.userForm.setValue({
            username: "",
            email: "",
            password: this.passwordGenerator(),         
            role: EnumGroupName.USER
        })
    }

    searchUsers(filter: any): void {
        if (!filter.value) {
            this.setAllUsers(this.original_allUsers.length > this.allUsers.length ? this.original_allUsers : this.allUsers)
            this.original_allUsers = []
            return;
        }
        if (this.original_allUsers.length == 0)
            this.original_allUsers = Array.from(this.allUsers)
        else
            this.setAllUsers(this.original_allUsers)

        this.setAllUsers(this.allUsers.filter(user => {
            let value = user[filter.id];            
            return user[filter.id].indexOf(filter.value) >= 0
        }))        
    }

    addUser(user) {
        // submit is done in the validation in this case so we don't have to do anything here.
        this.aws.context.userManagement.register(user.username, user.password, user.email, user.role, result => {
            this.modalRef.close();
            this.toastr.success("The user '" + user.username + "' has been added");            
            this.updateUserList();
        }, err => {
            // Display the error and remove the beginning chunk since it's not formatted well
            this.userValidationError = err.message;
        });
    }

    deleteUser(user) {
        this.aws.context.userManagement.deleteUser(user.Username, result => {
            this.modalRef.close();
            this.toastr.success("The user '" + user.Username + "' has been deleted");
            this.updateUserList();
        }, err => {
            // Display the error and remove the beginning chunk since it's not formatted well
            this.toastr.error(err.message);            
        });
    }

    dismissUserModal() {
        this.currentMode = EnumUserAdminModes.Default;
        this.userValidationError = "";        
        this.setDefaultUserFormValues()
    }
    dismissRoleModal() {
        this.currentMode = EnumUserAdminModes.Default;
    }

    private containsAccountStatus(user_status: string, target_status: string): boolean {
        if (!user_status)
            return false;
        let name = EnumAccountStatus[target_status]
        let index = user_status.toLowerCase().indexOf(name.toLowerCase())
        return user_status.toLowerCase().indexOf(name.toLowerCase()) >= 0
    }

    private setAllUsers(array): void {        
        this.allUsers = array;
        this.numUserPages = Math.ceil(this.allUsers.length / this.pageSize);
        this.updateUserPage();
    }

    private appendUser(user: any, rolename: string, array: Array<any>): void {
        let attributes = user.Attributes.filter(user => user.Name == 'email')
        user['Role'] = rolename
        user['Email'] = attributes.length > 0 ? attributes[0].Value : this.missingEmailString
        array.push(user)
    }

    private passwordGenerator(): string {                
        var upper = "ABCDEFGHIJKLMNOPQRSTUVWXYZ"        
        var lower = "abcdefghijklmnopqrstuvwxyz"        
        var number = "0123456789"
        var symbol = "!@#$%^&*()"
        var password = lower.charAt(Math.floor(Math.random() * lower.length)) +
            upper.charAt(Math.floor(Math.random() * upper.length)) +
            number.charAt(Math.floor(Math.random() * number.length)) + 
            symbol.charAt(Math.floor(Math.random() * symbol.length)) + 
            (Math.random() + 1).toString(36).substr(2, 8);

        var passwordArray = password.split('')
        for (let x = passwordArray.length - 1; x >= 0; x--) {
            let idx = Math.floor(Math.random() * (x + 1));
            let item = passwordArray[idx];

            passwordArray[idx] = passwordArray[x];
            passwordArray[x] = item;
        }

        return passwordArray.join('')

    }
}

export class Role {
    private _name: string;
    private _numUsers: number;

    get name(): string {
        return this._name
    }

    get numUsers(): number {
        return this._numUsers
    }

    set numUsers(numUsers: number) {
        this._numUsers = numUsers;
    }

    constructor(name: string, numUsers: number) {
        this._name = name;
        this._numUsers = numUsers;
    }

    static sortByNameDescending(a: Role, b: Role) {
        return a.name.localeCompare(b.name);
    }
}