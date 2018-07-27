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
    private _cognitoUserPool: any;
    private _apigateway: any;
    private _cloudWatchLogs: any;
    private _projectName: string = '';

    get name() {
        return this._projectName;
    }

    set name(value: string) {
        if (value === undefined || value === null)
            return;

        let configBucketParts = value.split('-configuration-');

        this._projectName = configBucketParts[0];
    }

    get cognitoUserPool(): any {
        return this._cognitoUserPool
    }

    get clientId(): string {
        return this._cognitoClientId;
    }

    get apiGateway(): any {
        return this._apigateway;
    }

    set apiGateway(value: any) {
        this._apigateway = value;
    }

    get cognitoIdentity(): any {
        return this._cognitoIdentity;
    }

    set cognitoIdentity(value: any) {
        this._cognitoIdentity = value;
    }

    get cloudWatchLogs(): any {
        return this._cloudWatchLogs;
    }

    set cloudWatchLogs(value: any) {
        this._cloudWatchLogs = value;
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

    get config(): any {
        return AWS.config
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
        this.name = configBucket;
        this._configBucket = configBucket;
        this._region = region;
        this._userPoolId = userPoolId;
        this._cognitoClientId = clientId;
        this._identityPoolId = identityPoolId;
        this._cognitoUserPool = new AWSCognito.CognitoIdentityServiceProvider.CognitoUserPool({
            UserPoolId: this._userPoolId,
            ClientId: this._cognitoClientId
        });
        let config = {
            region: region,
            credentials: new AWS.CognitoIdentityCredentials({
                IdentityPoolId: identityPoolId
            }, {
                    region: region
                }),
            sslEnabled: true,
            signatureVersion: 'v4'
        };

        this.config = config;
        this._awsClient = function (serviceName: string, options: any) {
            return new AWS[serviceName](options);
        }
    }

    public initializeServices(): void {
        this.s3 = this.awsClient("S3", { apiVersion: "2006-03-01", region: this.region, signatureVersion: "v4" });
        this.cloudFormation = this.awsClient("CloudFormation", { apiVersion: "2010-05-15", region: this.region });
        this.cognitoIdentityService = this.awsClient("CognitoIdentityServiceProvider", { apiVersion: "2016-04-18", region: this.region });
        this.cognitoIdentity = this.awsClient("CognitoIdentity", { apiVersion: "2014-06-30", region: this.region });
        this.apiGateway = this.awsClient("APIGateway", { apiVersion: "2015-07-09", region: this.region });
        this.cloudWatchLogs = this.awsClient("CloudWatchLogs", { apiVersion: "2014-03-28", region: this.region });
    }

    private transition(to: EnumContextState): void {
        this._change.next(<ContextChange>{
            state: to,
            context : this
        });
    }

}
