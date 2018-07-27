import { BehaviorSubject } from 'rxjs/BehaviorSubject';
import { Observable } from 'rxjs/Observable'
import { Deployment, DeploymentSettings, AwsService } from './aws.service'
import { AwsResourceGroup, ResourceGroup, ResourceOutputEnum  } from './resource-group.class'
import { Schema } from './schema.class'
import { AwsContext } from './context.class'
import { LyMetricService } from 'app/shared/service/index'

export class AwsDeployment implements Deployment {

    public static get DEPLOYMENT_TEMPLATE_FILE(): string { return "deployment-template.json" }

    private _deploymentSettings: DeploymentSettings;    
    private _resourceGroups: BehaviorSubject<ResourceGroup[]>;       
    protected subscribedResourceGroups: string[]; 
    private _resourceGroupList: ResourceGroup[]
    private _isLoading: boolean = false

    get resourceGroup(): Observable<ResourceGroup[]> {
        return this._resourceGroups.asObservable();
    }

    get resourceGroupList(): ResourceGroup[] {
        return this._resourceGroupList;
    }

    get settings(): DeploymentSettings {
        return this._deploymentSettings;
    }

    get configEndpointURL(): string {
        return this._deploymentSettings.ConfigurationBucket + '/' + this._deploymentSettings.ConfigurationKey;
    }

    get resourceGroupConfigEndpointURL(): string {
        return this.configEndpointURL + "/resource-group"
    }

    get isLoading(): boolean {
        return this._isLoading
    }

    public constructor(
        private context: AwsContext,        
        private deploymentsettings: DeploymentSettings        
    ) {
        this._deploymentSettings = deploymentsettings;
        this._resourceGroups = new BehaviorSubject<ResourceGroup[]>(undefined);        
        this._resourceGroupList = []
    }

    public update(): void {
        if (!this.settings || !this.settings.DeploymentStackId) {
            this._resourceGroups.next([]);     
            return;
        }

        let region = AwsService.getRegionFromArn(this.settings.DeploymentStackId);
        this._isLoading = true;       
        this.context.cloudFormation.describeStacks({
            StackName: this.settings.DeploymentStackId
        }, (err: any, data: any) => {
            if (err) {                
                console.log(err);
                this._isLoading = false; 
                return;
            }
            //finish the deployment settings configuration
            this.updateSettings(data)

            //we have the endpoint to get the resourcegroup configuration
            //get the configuration from the S3 bucket
            this.getResourceGroups();
        });
    }

    private updateSettings(data: any) {
        for (var i = 0; i < data.Stacks.length; i++) {
            for (var x = 0; x < data.Stacks[i].Parameters.length; x++) {
                let parameter = data.Stacks[i].Parameters[x];
                let key = parameter['ParameterKey'];
                let value = parameter['ParameterValue'];
                if (key == "ConfigurationBucket") {
                    this.settings.ConfigurationBucket = value;
                } else if (key == "ConfigurationKey") {
                    this.settings.ConfigurationKey = value;
                }

            }
        }
    }

    private getResourceGroups(): void {             
        this.context.cloudFormation.describeStackResources({
            StackName: this.settings.DeploymentStackId            
        }, (err: any, data: any) => {
            if (err) {                
                console.log(err);
                this._resourceGroupList = []
                this._resourceGroups.next(this._resourceGroupList);
                this._isLoading = false;
                return;
            }
            let rgs: AwsResourceGroup[] = [];            
            for (var i = 0; i < data.StackResources.length; i++) {
                let spec = data.StackResources[i];
                let logicalResourceId = spec[Schema.CloudFormation.LOGICAL_RESOURCE_ID]

                //Are we a resource group or resource
                if (spec[Schema.CloudFormation.RESOURCE_TYPE.NAME] == Schema.CloudFormation.RESOURCE_TYPE.TYPES.CLOUD_FORMATION) {                    
                    //we are a resource group
                    let rg = new AwsResourceGroup(this.context, spec);
                    rgs.push(rg);                    
                }
            }

            let updatedCount = 0;
            for (var i = 0; i < rgs.length; i++) {
                let rg = rgs[i];
                rg.updating.subscribe(isUpdating => {
                    this._isLoading = false;
                    if (isUpdating) {                        
                        return;
                    }

                    updatedCount++;                    
                    if (updatedCount == rgs.length) {
                        this._resourceGroupList = rgs
                        this._resourceGroups.next(this._resourceGroupList);                                         
                    }                    
                })              
                rg.setOutputs();      
            }

            if (rgs.length == 0) {
                this._isLoading = false;
                this._resourceGroupList = []
                this._resourceGroups.next(this._resourceGroupList);                
            }
            
        });
    }
}
