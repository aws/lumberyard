export class Fleet {
    launchConfigurationName: string;
    autoScalingGroupName: string;
    instanceNum: number;
    automaticallyTakeDown: boolean;

    constructor(fleetConfiguration: any) {
        this.launchConfigurationName = fleetConfiguration.launchConfigurationName;
        this.autoScalingGroupName = fleetConfiguration.autoScalingGroupName;
        this.instanceNum = fleetConfiguration.instanceNum;
        this.automaticallyTakeDown = fleetConfiguration.automaticallyTakeDown;
    }
}