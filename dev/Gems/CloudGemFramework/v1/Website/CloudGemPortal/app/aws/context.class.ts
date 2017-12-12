import { Injectable } from '@angular/core';
import { BehaviorSubject } from 'rxjs/BehaviorSubject';
import { Observable } from 'rxjs/Observable';
import { AwsDeployment } from './deployment.class';
import { AwsProject } from './project.class';
import { Authentication } from './authentication/authentication.class';
import { UserManagement } from './user/user-management.class';

//Ref to loose javascript AWS SDK
declare var AWS: any;
//Ref to loose AWSCognito SDK's:  This extends the AWS SDK to handle SRP_A authentication
declare var AmazonCognitoIdentity: any;
declare var AWSCognito: any;

export enum EnumContextState {
    INITIALIZED = 0,
    CLIENT_UPDATED = 1,        
    PROJECT_UPDATED = 2,
    DEPLOYMENT_UPDATED = 4,
    CREDENTIAL_UPDATED = 8    
}

export interface ContextChange {
    state: EnumContextState,
    context: AwsContext
}

export class AwsContext {
    private _configBucket: string;
    private _region: string;    
    private _userPoolId: string;
    private _identityPoolId: string;
    private _cognitoClientId: string;
    private _awsClient: Function;                
    private _s3: any;
    private _cloudFormation: any;
    private _project: AwsProject;
    private _deployment: AwsDeployment;
    private _change: BehaviorSubject<ContextChange>;
    private _authentication: Authentication; 
    private _usermanagement: UserManagement; 
    private _cognitoIdentity: any;
    private _cognitoIdentityService: any;

    get cognitoUserPool(): any {
        return new AWSCognito.CognitoIdentityServiceProvider.CognitoUserPool({
            UserPoolId: this._userPoolId,
            ClientId: this._cognitoClientId
        });
    }

    get clientId(): string {
        return this._cognitoClientId;
    }

    get cognitoIdentity(): any {
        return this._cognitoIdentity;
    }

    set cognitoIdentity(value: any) {
        this._cognitoIdentity = value;       
    }

    get cognitoIdentityService(): any {
        return this._cognitoIdentityService;
    }

    set cognitoIdentityService(value: any) {
        this._cognitoIdentityService = value;
    }

    get awsClient(): any {
        return this._awsClient;
    }

    get configBucket(): string {
        return this._configBucket;
    }

    get region(): string {
        return this._region;
    }

    set config(value: any) {        
        AWS.config.update(value);
        this.transition(EnumContextState.CLIENT_UPDATED);
    }

    set s3(value: any) {        
        this._s3 = value;
        this.transition(EnumContextState.CLIENT_UPDATED);
    }

    get s3(): any {
        return this._s3;
    }   

    set authentication(value: Authentication) {
        this._authentication = value;        
    }

    get authentication(): Authentication {
        return this._authentication;
    }

    set userManagement(value: UserManagement) {
        this._usermanagement = value;
    }

    get userManagement(): UserManagement {
        return this._usermanagement;
    }

    set cloudFormation(value: any) {        
        this._cloudFormation = value;
        this.transition(EnumContextState.CLIENT_UPDATED);
    }

    get cloudFormation(): any {        
        return this._cloudFormation;
    }

    set deployment(value: AwsDeployment) {        
        this._deployment = value;
        this.transition(EnumContextState.DEPLOYMENT_UPDATED);
    }

    get deployment(): AwsDeployment {
        return this._deployment;
    }

    set project(value: AwsProject) {                
        this._project = value;
        this.transition(EnumContextState.PROJECT_UPDATED);
    }

    get project(): AwsProject {
        return this._project;
    }   
    
    get change(): Observable<ContextChange> {
        return this._change.asObservable();
    }

    get userPoolId(): string {
        return this._userPoolId;
    }

    get identityPoolId(): string {
        return this._identityPoolId;
    }


    constructor() {        
        this._change = new BehaviorSubject<ContextChange>(<ContextChange>{});
    }

    public init(userPoolId: string, clientId: string, identityPoolId: string, configBucket: string, region: string) {
        this._configBucket = configBucket;
        this._region = region;
        this._userPoolId = userPoolId;
        this._cognitoClientId = clientId;
        this._identityPoolId = identityPoolId;        
        //let creds = { accessKeyId: accessKey, secretAccessKey: secretKey, region: region, sessionToken: sessionToken };
        let config = {
            region: region,
            credentials: new AWS.CognitoIdentityCredentials({
                IdentityPoolId: identityPoolId
            }),
            sslEnabled: true,
            signatureVersion: 'v4'
        };

        this.config = config;
        this._awsClient = function (serviceName: string, apiversion: string) {
                return new AWS[serviceName]({
                    apiVersion: apiversion
                });            }       

    }

    private transition(to: EnumContextState): void {
        this._change.next(<ContextChange>{
            state: to,
            context : this
        });
    }
    
}
