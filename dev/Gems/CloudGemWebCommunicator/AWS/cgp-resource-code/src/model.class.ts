//View models
export class UserEntry {
    ClientID: string;
    RegistrationStatus: string;
    RegistrationDate: string[];
    CertificateARN: string;

    constructor(speechInfo: any) {
        this.ClientID = speechInfo.ClientID;
        this.RegistrationStatus = speechInfo.RegistrationStatus;
        this.RegistrationDate = speechInfo.RegistrationDate;
        this.CertificateARN = speechInfo.CertificateARN;
    }
}

export class ChannelEntry {
    ChannelName: string;
    CommunicationType: string;
    CommunicationChannel: string;
    Subscription: string;
    BroadcastChannelName: string;

    constructor(speechInfo: any) {
        this.ChannelName = speechInfo.ChannelName;
        this.CommunicationType = speechInfo.CommunicationType;
        this.CommunicationChannel = speechInfo.CommunicationChannel;
        this.Subscription = speechInfo.Subscription;
        this.BroadcastChannelName = speechInfo.Subscription;
    }
}
//end view models