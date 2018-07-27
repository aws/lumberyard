import { Input, Component, ViewChild, ElementRef } from '@angular/core';
import { Fleet, CloudGemComputeFarmApi } from './index'
import { Http } from '@angular/http';
import { AwsService } from "app/aws/aws.service";
import { ModalComponent } from 'app/shared/component/index';
import { ToastsManager } from 'ng2-toastr';

export enum FleetMode {
    CreateLaunchConfig,
    CreateFleet,
    Show

}
@Component({
    selector: 'fleet-management',
    templateUrl: 'node_modules/@cloud-gems/cloudgemcomputefarm/fleet-management.component.html',
    styleUrls: ['node_modules/@cloud-gems/cloudgemcomputefarm/fleet-management.component.css']
})

export class FleetManagementComponent {
    @Input() context: any;
    private _apiHandler: CloudGemComputeFarmApi;

    private mode: FleetMode;
    private Modes: any;

    private amiList: Object[] = [];
    private isLoadingAmis: boolean;
    private imageId: string;

    private keyPairList: Array<string> = [];
    private isLoadingKeyPairs: boolean;
    private keyPair: string;

    //EC2 API does not provide a way to list all the EC2 instance types. Should list most common EC2 instance types here
    private instanceTypeList: string[] = [
        "m4.large",
        "m4.xlarge",
        "m4.2xlarge",
        "m4.4xlarge",
        "m4.10xlarge",
        "m4.16xlarge",
        "m5.large",
        "m5.xlarge",
        "m5.2xlarge",
        "m5.4xlarge",
        "m5.12xlarge",
        "m5.24xlarge",
        "c4.large",
        "c4.xlarge",
        "c4.2xlarge",
        "c4.4xlarge",
        "c4.8xlarge",
        "c5.large",
        "c5.xlarge",
        "c5.2xlarge",
        "c5.4xlarge",
        "c5.9xlarge",
        "c5.18xlarge"
    ];
    private instanceType: string;
    private launchConfigurationName: string;

    private currentFleet: Fleet;
    private newFleet: Fleet;
    private isLoadingFleet: boolean;

    @ViewChild(ModalComponent) modalRef: ModalComponent;

    constructor(private http: Http, private aws: AwsService, private toastr: ToastsManager) {
    }

    ngOnInit() {
        this._apiHandler = new CloudGemComputeFarmApi(this.context.ServiceUrl, this.http, this.aws);

        // bind proper scopes for callbacks that have a this reference
        this.createLaunchConfigurationModal = this.createLaunchConfigurationModal.bind(this);
        this.dismissModal = this.dismissModal.bind(this);

        this.getCurrentFleetConfiguration();
        this.Modes = FleetMode;
    }

    /**
     * List the user's private AMIs
	**/
    public getAmiList(): void {
        this.isLoadingAmis = true;
        this._apiHandler.getAmis().subscribe(response => {
            let obj = JSON.parse(response.body.text());
            this.amiList = obj.result;
            this.isLoadingAmis = false;
        }, err => {
            this.toastr.error("Failed to load all the AMIs. " + err.message);
            this.isLoadingAmis = false;
        });
    }

    /**
     * List the user's EC2 key pairs
	**/
    public getKeyPairList(): void {
        this.isLoadingKeyPairs = true;
        this._apiHandler.getKeyPairs().subscribe(response => {
            let obj = JSON.parse(response.body.text());
            this.keyPairList = obj.result;
            this.isLoadingKeyPairs = false;
        }, err => {
            this.toastr.error("Failed to load EC2 key pairs. " + err.message);
            this.isLoadingKeyPairs = false;
        });
    }


    /**
     * Define the Dismiss modal
	**/
    public dismissModal(): void {
        this.mode = FleetMode.Show;
    }

    /**
     * Define the createLaunchConfiguration modal
	**/
    public createLaunchConfigurationModal(): void {
        this.launchConfigurationName = "";
        this.imageId = "";
        this.keyPair = "";
        this.instanceType = "";

        this.getAmiList();
        this.getKeyPairList();
        this.newFleet = new Fleet({});
        this.mode = FleetMode.CreateLaunchConfig;
    }

    /**
     * Delete the existing auto scaling group and launch configuration if exist. Then create a new launch configuration
    **/
    public overwriteCurrentFleet(): void {
        if (this.currentFleet.autoScalingGroupName) {
            this.deleteAutoscalingGroup().then(function () {
                this.deleteLaunchConfiguration().then(
                    function () {
                        this.currentFleet = new Fleet({});
                        this.createLaunchConfiguration();
                    }.bind(this))
            }.bind(this))
        }
        else {
            this.createLaunchConfiguration();
        }
    }

    /**
     *Create or update a autoscaling group
    * @param fleet the fleet to create or update
	**/
    public createOrUpdateFleet(fleet: Fleet): void {
        if (this.modalRef) {
            this.modalRef.close();
        }

        let body = JSON.parse(JSON.stringify(fleet));
        body["maxSize"] = fleet["instanceNum"];
        body["minSize"] = fleet["instanceNum"];
        this._apiHandler.createOrUpdateAutoscalingGroup(body).subscribe(response => {
            let obj = JSON.parse(response.body.text());
            if (obj.result.status === 'SUCCEED') {
                this.toastr.success("The fleet " + fleet.autoScalingGroupName + " was updated.");
                this.currentFleet = JSON.parse(JSON.stringify(fleet));
                this.saveCurrentFleetConfiguration();
            }
            else {
                this.toastr.error("Failed to update the fleet. " + obj.result.status);
            }
        }, err => {
            this.toastr.error("Failed to update the fleet. " + err.message);
        });
    }

    /**
     * Get the current fleet configuration stored in S3
	**/
    private getCurrentFleetConfiguration(): void {
        this.isLoadingFleet = true;
        this._apiHandler.getFleetConfiguration().subscribe(response => {
            let obj = JSON.parse(response.body.text());
            this.currentFleet = obj.result;
            this.isLoadingFleet = false;
        }, err => {
            this.toastr.error("Failed to retrieve the existing fleet. " + err.message);
            this.isLoadingFleet = false;
        });
    }

    /**
     * Save the current fleet configuration to S3
	**/
    private saveCurrentFleetConfiguration(): void {
        this._apiHandler.postFleetConfiguration(this.currentFleet).subscribe(response => {
            let obj = JSON.parse(response.body.text());
            if (obj.result.status !== 'SUCCEED') {
                this.toastr.error("Failed to save the fleet configuration. " + obj.result.status);
            }
        }, err => {
            this.toastr.error("Failed to save the fleet configuration. " + err.message);
        });
    }

    /**
     *Create the autoscaling launch configuration
    **/
    private createLaunchConfiguration(): void {
        let body = { "launchConfigurationName": this.launchConfigurationName, "imageId": this.imageId, "instanceType": this.instanceType, "keyPair": this.keyPair };
        this._apiHandler.createAutoscalingLaunchConfiguration(body).subscribe(response => {
            let obj = JSON.parse(response.body.text());
            if (obj.result.status === 'SUCCEED') {
                this.toastr.success("The launch configuration " + this.launchConfigurationName + " was created.");
                this.newFleet.launchConfigurationName = this.launchConfigurationName;
                this.mode = FleetMode.CreateFleet;
            }
            else {
                this.toastr.error("Failed to create the launch configuration. " + obj.result.status);
            }
        }, err => {
            this.toastr.error("Failed to create the launch configuration. " + err.message);
        });
    }

    /**
     * Delete the current fleet
    **/
    private deleteAutoscalingGroup(): any {
        var promise = new Promise(function (resolve, reject) {
            this._apiHandler.deleteAutoscalingGroup(this.currentFleet.autoScalingGroupName).subscribe(response => {
                let obj = JSON.parse(response.body.text());
                if (obj.result.status === 'SUCCEED') {
                    resolve();
                }
                else {
                    this.toastr.error("Failed to delete auto scaling group " + this.currentFleet.autoScalingGroupName + ". " + obj.result.status);
                    reject();
                }
            })
        }.bind(this));
        return promise;
    }

    /**
     * Delete the current launch configuration
    **/
    private deleteLaunchConfiguration(): any {
        var promise = new Promise(function (resolve, reject) {
            this._apiHandler.deleteAutoscalingLaunchConfiguration(this.currentFleet.launchConfigurationName).subscribe(response => {
                let obj = JSON.parse(response.body.text());
                if (obj.result.status === 'SUCCEED') {
                    resolve();
                }
                else {
                    this.toastr.error("Failed to delete launch configuration " + this.currentFleet.launchConfigurationName + ". " + obj.result.status);
                    reject();
                }
            })
        }.bind(this));
        return promise;
    }
}