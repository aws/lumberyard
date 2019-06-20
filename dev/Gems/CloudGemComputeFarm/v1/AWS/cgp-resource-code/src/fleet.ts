export class Fleet {
    launchConfigurationName: string;
    autoScalingGroupName: string;
    instanceNum: number;
    automaticallyTakeDown: boolean;
    inactivityShutdown: number = 30;
    updateTime: string;
    
    constructor(fleetConfiguration ?: any) {
        this.launchConfigurationName = fleetConfiguration.launchConfigurationName;
        this.autoScalingGroupName = fleetConfiguration.autoScalingGroupName;
        this.instanceNum = fleetConfiguration.instanceNum;
        this.automaticallyTakeDown = fleetConfiguration.automaticallyTakeDown;
        this.inactivityShutdown = fleetConfiguration.inactivityShutdown |= 30;
        this.updateTime = fleetConfiguration.updateTime;
    }


}