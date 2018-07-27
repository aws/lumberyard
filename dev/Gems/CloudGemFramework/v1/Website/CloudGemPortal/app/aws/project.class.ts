import { Subject } from 'rxjs/Subject';
import { BehaviorSubject } from 'rxjs/BehaviorSubject';
import { Observable } from 'rxjs/Observable'
import { AwsDeployment } from './deployment.class'
import { AwsService, ProjectSettings, DeploymentSettings, Deployment } from './aws.service'
import { AwsContext } from './context.class'
import { Authentication, AuthStateActionContext, EnumAuthState } from './authentication/authentication.class'
import { Http } from '@angular/http';
import { LyMetricService } from 'app/shared/service/index'
import { Schema } from './schema.class'
import { ResourceOutput  } from './resource-group.class'
import 'rxjs/add/operator/map'
import 'rxjs/add/operator/catch'

export class AwsProject {

    public static get BUCKET_CONFIG_NAME(): string { return "config_bucket" }
    public static get PROJECT_SETTING_FILE(): string { return "project-settings.json" }

    private _settings: Subject<ProjectSettings>;
    private _isInitialized: boolean;
    private _deployments: Deployment[]
    private _activeDeployment: AwsDeployment;
    private _activeDeploymentSubscription: Subject<AwsDeployment>;
    private _outputs: Map<string, ResourceOutput>;
    private _settingsCache: ProjectSettings;

    get isInitialized() {
        return this._isInitialized;
    }

    set isInitialized(value: boolean) {
        this._isInitialized = value;
    }

    get deployments(): Deployment[] {
        return this._deployments;
    }

    get activeDeployment(): AwsDeployment {
        return this._activeDeployment;
    }

    set activeDeployment(activeDeployment: AwsDeployment) {
        this._activeDeployment = activeDeployment;
        this._activeDeploymentSubscription.next(activeDeployment);
    }

    get activeDeploymentSubscription(): Observable<AwsDeployment> {
        return this._activeDeploymentSubscription.asObservable();
    }

    get settings(): Observable<ProjectSettings> {
        return this._settings.asObservable();
    }

    get outputs(): any {
        return this._outputs
    }

    constructor(private context: AwsContext, private http: Http) {        
        this._settings = new Subject<ProjectSettings>();
        this._activeDeploymentSubscription = new Subject<AwsDeployment>();
        this._activeDeployment = null;
        this._deployments = [];
        this._outputs = new Map<string, ResourceOutput>();


        this._activeDeploymentSubscription.subscribe(deployment => { 
            if (deployment) {
                this._activeDeployment = deployment
                deployment.update();
            }
        })

        //initialize the deployments array
        this.settings.subscribe(d => {
            if (!d) {
                this.isInitialized = true;
                this._activeDeploymentSubscription.next(undefined);                
                return;
            }

            if (d.DefaultDeployment || Object.keys(d.deployment).length == 0)
                this.isInitialized = true;
            
            //clear the array but don't generate a new reference
            this._deployments.length = 0;

            let default_deployment: string = d.DefaultDeployment;
            let default_found = false;
            for (let entry in d.deployment) {
                let obj = d.deployment[entry];
                var deploymentSetting = <DeploymentSettings>({
                    name: entry,
                    DeploymentAccessStackId: obj['DeploymentAccessStackId'],
                    DeploymentStackId: obj['DeploymentStackId'],
                    ConfigurationBucket: obj['ConfigurationBucket'],
                    ConfigurationKey: obj['ConfigurationKey']
                })
                var awsdeployment = new AwsDeployment(this.context, deploymentSetting);
                this._deployments.push(awsdeployment);
                //set the default deployment
                if (entry == d.DefaultDeployment) {
                    this._activeDeploymentSubscription.next(awsdeployment);
                    default_found = true
                }
            }
            if (!default_found)
                this._activeDeploymentSubscription.next(awsdeployment);
                            
        });

        let proj = this;
        var signedRequest = this.context.s3.getSignedUrl('getObject', { Bucket: this.context.configBucket, Key: AwsProject.PROJECT_SETTING_FILE, Expires: 120 })
        
        this.http.request(signedRequest)
            .subscribe(response => {
                let settings = response.json();

                var deployments = settings.deployment;
                //Filter out our 'wildcard' settings and emit our deployment list
                delete deployments["*"];
                proj._settingsCache = settings
                proj._settings.next(settings);
            },
            err => {
                console.error(err)
            })
        
    }
}
