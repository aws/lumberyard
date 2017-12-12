import { BehaviorSubject } from 'rxjs/BehaviorSubject';
import { Observable } from 'rxjs/Observable'
import { AwsDeployment } from './deployment.class'
import { AwsService, ProjectSettings, DeploymentSettings, Deployment } from './aws.service'
import { AwsContext } from './context.class'
import { Authentication, AuthStateActionContext, EnumAuthState } from './authentication/authentication.class'
import { Http } from '@angular/http';
import { LyMetricService } from 'app/shared/service/index'
import 'rxjs/add/operator/map'
import 'rxjs/add/operator/catch'

export class AwsProject {

    public static get BUCKET_CONFIG_NAME(): string { return "config_bucket" }
    public static get PROJECT_SETTING_FILE(): string { return "project-settings.json" }

    private _settings: BehaviorSubject<ProjectSettings>;
    private _isInitialized: boolean;
    private _deployments: Deployment[]
    private _activeDeployment: BehaviorSubject<Deployment>;

    get isInitialized() {
        return this._isInitialized;
    }

    set isInitialized(value: boolean) {
        this._isInitialized = value;
    }

    get deployments(): Deployment[] {
        return this._deployments;
    }

    get activeDeployment(): Observable<Deployment> {
        return this._activeDeployment.asObservable();
    }

    public setDeployment(value: Deployment) {
        this._activeDeployment.next(value);
    }

    get settings(): Observable<ProjectSettings> {
        return this._settings.asObservable();
    }

    constructor(private context: AwsContext, private http: Http) {        
        this._settings = new BehaviorSubject<ProjectSettings>(null);
        this._activeDeployment = new BehaviorSubject<Deployment>(null);
        this._deployments = [];

        this.activeDeployment.subscribe(deployment => {
            if (deployment)
                deployment.update();
        })

        //initialize the deployments array
        this.settings.subscribe(d => {
            if (d == null)
                return;

            if (d.DefaultDeployment)
                this.isInitialized = true;

            //clear the array but don't generate a new reference
            this._deployments.length = 0;

            let default_deployment: string = d.DefaultDeployment;
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
            }
            this._activeDeployment.next(awsdeployment);
        });

        let proj = this;
        var signedRequest = this.context.s3.getSignedUrl('getObject', { Bucket: this.context.configBucket, Key: AwsProject.PROJECT_SETTING_FILE, Expires: 60 })
        this.http.request(signedRequest).map(response => {
            return response;
        }).catch((error: any) => {
                return Observable.throw(new Error("Status: " + error.status + ", Message:" + error.statusText));
            }).subscribe(response => {                        
                var body = response._body.toString('utf-8');

                let settings = JSON.parse(body);

                var deployments = settings.deployment;
                //Filter out our 'wildcard' settings and emit our deployment list
                delete deployments["*"];

                proj._settings.next(settings);
            }, (err) => {
                console.error(err)
            })
    }

}
