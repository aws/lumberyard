import { Http } from '@angular/http';
import { ViewChild, ElementRef, Input, Component, ViewContainerRef } from '@angular/core';
import { ModalComponent } from 'app/shared/component/index';
import { AwsService } from "app/aws/aws.service";
import { UserEntry, ChannelEntry, WebCommunicatorApi } from './index'
import { Subscription } from 'rxjs/rx';
import { AbstractCloudGemIndexComponent } from 'app/view/game/module/cloudgems/class/index';
import { ToastsManager } from 'ng2-toastr/ng2-toastr';
import { ActionItem } from "app/view/game/module/shared/class/index";
import { Paho as Paho1 } from 'ng2-mqtt/mqttws31';
import { LyMetricService, BreadcrumbService } from 'app/shared/service/index';

declare var AWS: any;

export enum Mode {
    List,
    BanUser,
    SendMessageViaChannel,
    SendMessageToUser
}

@Component({
    selector: 'web-communicator-index',
    templateUrl: 'node_modules/@cloud-gems/cloudgemwebcommunicator/index.component.html',
    styleUrls: ['node_modules/@cloud-gems/cloudgemwebcommunicator/index.component.css']
})

export class WebCommunicatorIndexComponent extends AbstractCloudGemIndexComponent {
    @Input() context: any;
    private subNavActiveIndex = 0;
    private _apiHandler: WebCommunicatorApi;

    // define types
    private mode: Mode;
    private webCommunicatorModes: any;

    private pageSize: number = 10;
    private userPages: number;
    private currentUserPage: number;

    private client: any;
    private isRegisteringClient: boolean;

    private channels: ChannelEntry[] = [];
    private simplifiedChannels: ChannelEntry[] = [];
    private isLoadingChannels: boolean;
    private currentChannel: ChannelEntry;
    private isListeningToChannel: boolean;
    private userType: string;

    private users: UserEntry[] = [];
    private filteredUsers: UserEntry[] = [];
    private usersOnCurrentPage: UserEntry[] = [];
    private currentUser: UserEntry;
    private isLoadingUsers: boolean;
    private isListeningToUser: boolean;
    private channelType: string;
    private filterCondition: string = "";

    private newMessage: string;
    private messageList: string[];
    private selectedChannel: string;
    private selectedUser: string;

    private listenToChannelAction = new ActionItem("Listen", this.initializeListeningToChannelPage);
    private broadcastAction = new ActionItem("Send message", this.sendMessageViaChannelModal);
    private channelActions: ActionItem[] = [
        this.listenToChannelAction,
        this.broadcastAction];

    private listenToUserAction = new ActionItem("Listen", this.initializeListeningToUserPage);
    private banUserAction = new ActionItem("Ban", this.banUserModal);
    private unbanUserAction = new ActionItem("Unban", this.unbanUser);
    private sendMessageToUserAction = new ActionItem("Send message", this.sendMessageToUserModal);
    private registeredUserActions: ActionItem[] = [
        this.listenToUserAction,
        this.banUserAction,
        this.sendMessageToUserAction];
    private registeredUserProfileActions: ActionItem[] = [
        this.banUserAction,
        this.sendMessageToUserAction];
    private bannedUserActions: ActionItem[] = [
        this.unbanUserAction];

    private Math: any;
    private ascOrder: boolean;

    @ViewChild(ModalComponent) modalRef: ModalComponent;
    @ViewChild("pagination") pagination;

    constructor(private http: Http, private aws: AwsService, private toastr: ToastsManager, vcr: ViewContainerRef, private metric: LyMetricService, private breadcrumbs: BreadcrumbService) {
        super()
        this.toastr.setRootViewContainerRef(vcr);
    }

    /**
     * Initialize values
	**/
    ngOnInit() {
        this._apiHandler = new WebCommunicatorApi(this.context.ServiceUrl, this.http, this.aws, this.metric, this.context.identifier);

        // bind proper scopes for callbacks that have a this reference
        this.dismissModal = this.dismissModal.bind(this);
        this.banUserModal = this.banUserModal.bind(this);
        this.listenToChannelAction.onClick = this.listenToChannelAction.onClick.bind(this);
        this.broadcastAction.onClick = this.broadcastAction.onClick.bind(this);
        this.listenToUserAction.onClick = this.listenToUserAction.onClick.bind(this);
        this.banUserAction.onClick = this.banUserAction.onClick.bind(this);
        this.unbanUserAction.onClick = this.unbanUserAction.onClick.bind(this);
        this.sendMessageToUserAction.onClick = this.sendMessageToUserAction.onClick.bind(this);

        this.webCommunicatorModes = Mode;
        this.registerCGPClient();       
    }

    /**
     * Update the content when switch to another tab
     * @param subNavItemIndex the index of the tab
    **/
    public getSubNavItem(subNavItemIndex: number): void {
        if (this.isListeningToChannel || this.isListeningToUser) {
            this.breadcrumbs.removeLastBreadcrumb();
            this.isListeningToChannel = false;
            this.isListeningToUser = false;
        }

        this.subNavActiveIndex = subNavItemIndex;
        if (subNavItemIndex == 0) {
            this.updateUsers();
        }
        if (subNavItemIndex == 1) {
            this.mode = Mode.List;
            this.updateChannels();
        }
    }

    /**
     * Update the user list
	**/
    public updateUsers(): void {
        this.isListeningToUser = false;
        this.isLoadingUsers = true;

        this.getAllUsers().then(function () {
            this.isLoadingUsers = false;
        }.bind(this), function() {
            this.isLoadingUsers = false;
        }.bind(this))
    }

    /**
     * Ban a specific user
     * @param user the user who needs to be banned
	**/
    public banUser(user: UserEntry): void {
        this.setUserStatus(user, 'BANNED');
        this.modalRef.close();
    }

    /**
     * Create a new OPENSSL user
	**/
    public createOPENSSLUser(): void {
        this._apiHandler.register("OPENSSL").subscribe(response => {
            let obj = JSON.parse(response.body.text());
            let registrationResult = obj.result;
            let PrivateKey = registrationResult.PrivateKey;
            let DeviceCert = registrationResult.DeviceCert;
            let deviceInfo = { "endpoint": registrationResult.Endpoint, "endpointPort": registrationResult.EndpointPort, "connectionType": registrationResult.ConnectionType}
            this.downloadCertificateFile(PrivateKey, "webcommunicatorkey.pem", 'text/csv');
            setTimeout(() => this.downloadCertificateFile(DeviceCert, "webcommunicatordevice.pem", 'text/csv'), 1000);
            setTimeout(() => this.downloadCertificateFile(JSON.stringify(deviceInfo), "deviceInfo.json", 'application/json'), 2000);

            this.updateUsers();
        }, err => {
            this.toastr.error("The user was not created successfully. " + err.message);
        });
    }

    /**
     * Unban a specific user
     * @param user the user who needs to be unbanned
	**/
    public unbanUser(user: UserEntry): void {
        this.setUserStatus(user, 'REGISTERED');
    }

    /**
     * Initialize the listening page of a user
     * @param user the user to listen to
	**/
    public initializeListeningToUserPage(user: UserEntry): void {
        this.currentUser = user;
        this.channelType = "allChannels";

        if (this.channels.length > 0) {
            this.listenToUser(this.currentUser.ClientID, this.channelType);
            this.isListeningToUser = true;
            this.breadcrumbs.addBreadcrumb(user.ClientID, null);
        }
        else {
            this.getAllChannels().then(function () {
                this.listenToUser(this.currentUser.ClientID, this.channelType);
                this.isListeningToUser = true;
                this.breadcrumbs.addBreadcrumb(user.ClientID, null);
            }.bind(this))
        }
    }

    /**
     * Update the channel list
    **/
    public updateChannels(): void {
        this.isListeningToChannel = false;
        this.isLoadingChannels = true;

        this.getAllChannels().then(function () {
            this.isLoadingChannels = false;
        }.bind(this), function () {
            this.isLoadingChannels = false;
        }.bind(this))
    }

    /**
     * Initialize the listening page of a channel
     * @param channel the channel to listen to
    **/
    public initializeListeningToChannelPage(channel: ChannelEntry): void {
        this.currentChannel = channel;
        this.breadcrumbs.addBreadcrumb(channel.ChannelName, null);

        if (this.users.length > 0) {
            this.listenToDefaultChannel();
        }

        this.getAllUsers().then(function () {
            this.listenToDefaultChannel();
        }.bind(this))
    }

    /**
     * Send message via the current channel
    **/
    public sendMessageViaChannel(): void {
        let channel = JSON.parse(JSON.stringify(this.currentChannel));
        channel.CommunicationType = this.selectedUser == "allClients" ? "BROADCAST" : "PRIVATE";
        this.publishMessage(channel, this.selectedUser);
        this.modalRef.close();
    }

    /**
     * Send message to the current user
    **/
    public sendMessageToUser(): void {
        let privateChannel = new ChannelEntry({});
        for (let channel of this.channels) {
            if (channel.ChannelName == this.selectedChannel && channel.CommunicationType == "PRIVATE") {
                privateChannel = channel;
                break;
            }
        }
        this.publishMessage(privateChannel, this.currentUser.ClientID);
        this.modalRef.close();
    }

    /**
     * Define the sendMessageToUser modal
     * @param user the user to receive the message
	**/
    public sendMessageToUserModal(user: UserEntry): void {
        this.currentUser = user;
        this.newMessage = "";
        this.selectedChannel = "";

        if (this.channels.length > 0) {
            this.setDefaultChannel();
            this.mode = Mode.SendMessageToUser;
            return;
        }

        this.getAllChannels().then(function () {
            this.setDefaultChannel();
            this.mode = Mode.SendMessageToUser;
        }.bind(this))
    }

    /**
     * Define the sendMessageViaChannel modal
     * @param channel the channel used to send the message
	**/
    public sendMessageViaChannelModal(channel: ChannelEntry): void {
        this.currentChannel = channel;
        this.selectedChannel = this.currentChannel.ChannelName;
        this.newMessage = "";

        if (this.users.length > 0) {
            this.selectedUser = channel.CommunicationType == "PRIVATE" ? this.users[0].ClientID : "allClients";
            this.mode = Mode.SendMessageViaChannel;
            return;
        }

        this.getAllUsers().then(function () {
            this.selectedUser = channel.CommunicationType == "PRIVATE" ? this.users[0].ClientID : "allClients";
            this.mode = Mode.SendMessageViaChannel;
        }.bind(this))
    }

    /**
     * Define the disniss modal
	**/
    public dismissModal(): void {
        this.mode = Mode.List;
    }

    /**
     * Define the banUser modal
    * @param user the user to ban
	**/
    public banUserModal(user: UserEntry): void {
        this.currentUser = user;
        this.mode = Mode.BanUser;
    }

    /**
     * Sort the user list
     * @param key the key to sort by
     * @param ascOrder sort the list in ascending order
    **/
    public sortUsersByColumn(key, ascOrder): void {
        this.sort(this.users, key, ascOrder);
        this.sort(this.filteredUsers, key, ascOrder);
        this.updatePaginationInfo(this.currentUserPage);
    }

    /**
     * Update the pagination information on the users page
     * @param currentPageIndex the current page index on the users page
    **/
    public updatePaginationInfo(currentPageIndex: number): void {
        if (this.subNavActiveIndex == 0) {
            let numberEntries: number = this.filteredUsers.length;
            this.userPages = Math.ceil(numberEntries / this.pageSize);

            let startIndex = (currentPageIndex - 1) * this.pageSize;
            let endIndex = currentPageIndex * this.pageSize;

            this.usersOnCurrentPage = this.filteredUsers.slice(startIndex, endIndex);
            this.currentUserPage = currentPageIndex;

            if (this.pagination) {
                this.pagination.currentPage = currentPageIndex;
            }

        }
    }

    /**
     * Filter the client list
    **/
    public filterClientList(): void {
        this.filteredUsers = this.users.filter(user => user.ClientID.indexOf(this.filterCondition) === 0);
        this.updatePaginationInfo(1);
    }
    /**
     * register the CGP Client
    **/
    private registerCGPClient(): void {
        this.isRegisteringClient = true;
        this._apiHandler.register("WEBSOCKET").subscribe(response => {
            let obj = JSON.parse(response.body.text());
            var credentials = AWS.config.credentials;
            this.createMqttClient(obj.result.Endpoint, credentials);
            this.isRegisteringClient = false;
            this.updateUsers();
        }, err => {
            this.toastr.error("The client did not register properly. " + err.message);
            this.isRegisteringClient = false;
        });
    }

    /**
     * Create an MQTT client and connect to the AWS IoT endpoint
     * @param endpoint the endpoint to connect to
     * @param AWSCredentials the aws credentials of the current client
    **/
    private createMqttClient(endpoint, AWSCredentials): void {
        var url = this.getSignedUrl(endpoint, this.aws.context.region, AWSCredentials);
        this.client = new Paho1.MQTT.Client(url, this.aws.context.authentication.user.username + this.generateUuid());
        this.connect();
    }

    /**
     * Connect the MQTT client to the server
    **/
    private connect(): any {
        var promise = new Promise(function (resolve, reject) {
            if (!this.client.isConnected()) {
                let connectOptions = {
                    onSuccess: function () {
                        resolve();
                    }.bind(this),
                    useSSL: true,
                    timeout: 60,
                    mqttVersion: 4,
                    onFailure: function () {
                        this.toastr.error("Connection failed.");
                        reject();
                    }.bind(this)
                };
                this.client.connect(connectOptions);
            }
            else {
                resolve();
            }
        }.bind(this));
        return promise;
    }

    /**
     * Subscribe to a channel
     * @param channel the channel to subscribe to
     * @param clientID the client ID for the private channel
    **/
    private subscribeToChannel(channel: ChannelEntry, clientID: string): any {
        let channelSubscription = this.getChannelSubscription(channel, clientID);
        var promise = new Promise(function (resolve, reject) {
            this.connect().then(function () {
                this.client.subscribe(channelSubscription, {
                    onSuccess: function () {
                        resolve();
                    }.bind(this),
                    onFailure: function () {
                        this.toastr.error("Failed to subscribe to the channel " + channel.ChannelName + " .");
                        reject();
                    }.bind(this)
                })
            }.bind(this))
        }.bind(this))
        return promise;
    }

    /**
     * Generate an UUID
     * @returns an UUID
    **/
    private generateUuid(): string {
        let uuid = this.generateRandonmCharacters() + this.generateRandonmCharacters();

        for (let i = 0; i < 4; ++i) {
            uuid += '-';
            uuid += this.generateRandonmCharacters();
        }

        uuid += this.generateRandonmCharacters() + this.generateRandonmCharacters();
        return uuid;
    }

    /**
     * Generate a string which contains 4 random characters
     * @returns a string which contains 4 random characters
    **/
    private generateRandonmCharacters(): string {
        return Math.floor((1 + Math.random()) * 0x10000)
            .toString(16)
            .substring(1);
    }

    /**
     * Get all the existing users
    **/
    private getAllUsers(): any {
        let promise = new Promise(function (resolve, reject) {
            this._apiHandler.listAllUsers().subscribe(response => {
                let obj = JSON.parse(response.body.text());
                this.updateUsersInfo(obj.result.UserInfoList);
                resolve();
            }, err => {
                this.toastr.error("The user list did not refresh properly. " + err.message);
                reject();
            });
        }.bind(this));

        return promise;
    }

    /**
     * Generate the user list and sort the users by Client ID
     * @param userInfoList the existing user information list
    **/
    private updateUsersInfo(userInfoList): void {
        this.users = [];
        this.ascOrder = true;

        for (let user of userInfoList) {
            this.users.push(new UserEntry(user));
        }
        this.sort(this.users, "ClientID", this.ascOrder);

        this.filterClientList();
    }

    /**
     * Set the status of a user
     * @param user the user to change
     * @param status the new status of the user
    **/
    private setUserStatus(user: UserEntry, status: string): void {
        let body = {
            "ClientID": user.ClientID,
            "RegistrationStatus": status,
            "CGPUser": true
        };

        this._apiHandler.setUserStatus(body).subscribe(response => {
            let obj = JSON.parse(response.body.text());
            user.RegistrationStatus = obj.result.SetUserStatusResult.RegistrationStatus;
        }, err => {
            this.toastr.error("The user status did not change properly. " + err.message);
        });
    }

    /**
     * Download the certificate file
    * @param content the content of the certificate file
    * @param name the name of the certificate file
    **/
    private downloadCertificateFile(content: string, name: string, fileType: string): void {
        var blob = new Blob([content], { type: fileType });
        if (window.navigator.msSaveOrOpenBlob) //Edge
        {
            window.navigator.msSaveBlob(blob, name);
        }
        else //Chrome & FireFox
        {
            let a = document.createElement("a");
            let fileURL = URL.createObjectURL(blob);
            a.href = fileURL;
            a.download = name;
            window.document.body.appendChild(a);
            a.click();
            window.document.body.removeChild(a);
            URL.revokeObjectURL(fileURL);
        }
    }

    /**
     * Listen to the message sent to a specific user
     * @param clientID the user to listen to
     * @param channelName the name of the channel to listen to.
    **/
    private listenToUser(clientID: string, channelName: string): void {
        this.messageList = [];
        let communicationChannels = [];
        for (let channel of this.channels) {
            if (channelName == "allChannels" || channel.ChannelName == channelName) {
                let channelSubscription = this.getChannelSubscription(channel, clientID);
                if (communicationChannels.indexOf(channelSubscription) == -1) {
                    communicationChannels.push(this.getChannelSubscription(channel, clientID));
                }
                this.subscribeToChannel(channel, clientID);
            }
        }

        this.client.onMessageArrived = (message) => {
            if (communicationChannels.indexOf(message.destinationName) > -1) {
                let messageType = "(BROADCAST) ";
                if (message.destinationName.indexOf("/client/") > -1) {
                    messageType = "(PRIVATE) ";
                }
                let messageContent = JSON.parse(message.payloadString);
                if (channelName == "allChannels" || messageContent.ChannelName == channelName) {
                    this.messageList.push(messageType + messageContent.Message);
                }
            }
        }
    }

    /**
     * Publish a new message to a user through a channel
     * @param channel the channel to publish the message through
     * @param clientID the user to receive the message
    **/
    private publishMessage(channel: ChannelEntry, clientID: string): void {
        this.subscribeToChannel(channel, clientID).then(function () {
            let message = new Paho1.MQTT.Message(JSON.stringify({ "ChannelName": channel.ChannelName, "Message": this.newMessage }));
            message.destinationName = this.getChannelSubscription(channel, clientID);
            this.client.send(message);
        }.bind(this));
    }

    /**
     * Set the default private channel when sending a message to the current user
    **/
    private setDefaultChannel(): void {
        for (let channel of this.simplifiedChannels) {
            if (channel.CommunicationType != "BROADCAST") {
                this.selectedChannel = channel.ChannelName;
                return;
            }
        }
        this.selectedChannel = "";
    }

    /**
     * Get all the existing channels
    **/
    private getAllChannels(): any {
        var promise = new Promise(function (resolve, reject) {
            this._apiHandler.listAllChannels().subscribe(response => {
                let obj = JSON.parse(response.body.text());
                this.updateChannelsInfo(obj.result);
                resolve();
            }, err => {
                this.toastr.error("The channel list did not refresh properly. " + err.message);
                reject();
            });
        }.bind(this));
        return promise;
    }

    /**
     * Update the channel information and generate the simplified channel list shown in CGP
     * @param channelList the channels to update
    **/
    private updateChannelsInfo(channelList: ChannelEntry[]): void {
        this.channels = [];
        this.simplifiedChannels = [];
        this.ascOrder = true;
        let channelDir = {};

        for (let channel of channelList) {
            this.channels.push(new ChannelEntry(channel));
            if (channelDir[channel.ChannelName]) {
                channelDir[channel.ChannelName].CommunicationType = "BROADCAST & PRIVATE";
            }
            else {
                channelDir[channel.ChannelName] = JSON.parse(JSON.stringify(channel));
            }
        }

        for (let key of Object.keys(channelDir)) {
            this.simplifiedChannels.push(new ChannelEntry(channelDir[key]));
        }
        this.sort(this.simplifiedChannels, "ChannelName", this.ascOrder);
    }

    /**
     * Listen to the default channel when users enter the listening page
    **/
    private listenToDefaultChannel(): void {
        this.messageList = [];
        let channelSubscriptions = [];

        if (this.currentChannel.CommunicationType.indexOf("BROADCAST") !== -1) {
            let channelSubscription = this.subscribeToActualChannel(this.currentChannel, "BROADCAST", "allClients");
            channelSubscriptions.push(channelSubscription);
        }

        if (this.currentChannel.CommunicationType.indexOf("PRIVATE") !== -1) {
            for (let user of this.users) {
                let channelSubscription = this.subscribeToActualChannel(this.currentChannel, "PRIVATE", user.ClientID);
                channelSubscriptions.push(channelSubscription);
            }
        }

        this.client.onMessageArrived = (message) => {
            if (channelSubscriptions.indexOf(message.destinationName) !== -1) {
                let messageContent = JSON.parse(message.payloadString);
                let messageType = message.destinationName.indexOf("/client/") === -1 ? "(BROADCAST) " : "(PRIVATE) ";
                if (messageContent.ChannelName == this.currentChannel.ChannelName) {
                    this.messageList.push(messageType + messageContent.Message);
                }
            }
        }

        this.isListeningToChannel = true;
    }

    /**
     * Subscribe to the actual channel
     * @param simplifiedChannel the simplified channel to listen to
    * @param communicationType the communication type of the channel
     * @param clientID the user to listen to
     * @returns subscription of the channel
    **/
    private subscribeToActualChannel(simplifiedChannel: ChannelEntry, communicationType: string, clientID: string): string {
        let channel = JSON.parse(JSON.stringify(simplifiedChannel));
        channel.CommunicationType = communicationType;
        this.subscribeToChannel(channel, clientID);
        let channelSubscription = this.getChannelSubscription(channel, clientID);
        return channelSubscription;
    }

    /**
     * Get the subcsription property of a specific channel
     * @param channel the channel to get the subscription property for
     * @param clientID the user for the private channel
    **/
    private getChannelSubscription(channel: ChannelEntry, clientID: string): string {
        let communicationChannel = this.getCommunicationChannel(channel);
        let channelSubscription = communicationChannel.Subscription;
        if (channel.CommunicationType == "PRIVATE") {
            channelSubscription = "";
            let parsedSubscription = communicationChannel.Subscription.split("/");
            for (let index = 0; index < parsedSubscription.length - 1; index++) {
                channelSubscription += parsedSubscription[index];
                channelSubscription += "/";
            }
            channelSubscription += clientID;
        }
        return channelSubscription;
    }

    /**
     * Get the communication channel property of a specific channel
     * @param channel the channel to get the communication channel property for
    **/
    private getCommunicationChannel(channel: ChannelEntry): ChannelEntry {
        for (let channelEntry of this.channels) {
            let findCommunicationChannel = !channel.CommunicationChannel && channel.ChannelName == channelEntry.ChannelName && channel.CommunicationType == channelEntry.CommunicationType;
            findCommunicationChannel = findCommunicationChannel || (channel.CommunicationChannel == channelEntry.ChannelName && channel.CommunicationType == channelEntry.CommunicationType);
            if (findCommunicationChannel) {
                return channelEntry;
            }
        }
        return new ChannelEntry({});
    }

    /**
     * Get the communication type property of a channel
     * @param channelName the name of the channel to get the communication type property for
    **/
    private getChannelCommunicationType(channelName: string): string {
        for (let channel of this.simplifiedChannels) {
            if (channel.ChannelName == channelName) {
                return channel.CommunicationType;
            }
        }
        return "";
    }

    /**
     * Sort a list of items
     * @param list the list to sort
     * @param key the key to sort by
     * @param ascOrder sort the list in ascending order
    **/
    private sort(list, key, ascOrder): void {
        this.ascOrder = ascOrder;
        list.sort((itemA, itemB): number => {
            if (itemA[key] < itemB[key]) {
                return this.ascOrder ? -1 : 1;
            }
            if (itemA[key] > itemB[key]) {
                return this.ascOrder ? 1 : -1;
            }
            return 0;
        });
    }

    /**
     * Add the signing information to the request. http://docs.aws.amazon.com/iot/latest/developerguide/protocols.html#mqtt-ws
     * @param host the IoT endpoint
     * @param region the region of the endpoint
     * @param credentials the AWS credentials of the client
    **/
    private getSignedUrl(host, region, credentials): any {
        var datetime = AWS.util.date.iso8601(new Date()).replace(/[:\-]|\.\d{3}/g, '');
        var date = datetime.substr(0, 8);

        var method = 'GET';
        var protocol = 'wss';
        var uri = '/mqtt';
        var service = 'iotdevicegateway';
        var algorithm = 'AWS4-HMAC-SHA256';

        var credentialScope = date + '/' + region + '/' + service + '/' + 'aws4_request';
        var canonicalQuerystring = 'X-Amz-Algorithm=' + algorithm;
        canonicalQuerystring += '&X-Amz-Credential=' + encodeURIComponent(credentials.accessKeyId + '/' + credentialScope);
        canonicalQuerystring += '&X-Amz-Date=' + datetime;
        canonicalQuerystring += '&X-Amz-SignedHeaders=host';

        var canonicalHeaders = 'host:' + host + '\n';
        var payloadHash = AWS.util.crypto.sha256('', 'hex')
        var canonicalRequest = method + '\n' + uri + '\n' + canonicalQuerystring + '\n' + canonicalHeaders + '\nhost\n' + payloadHash;

        var stringToSign = algorithm + '\n' + datetime + '\n' + credentialScope + '\n' + AWS.util.crypto.sha256(canonicalRequest, 'hex');
        var signingKey = this.getSignatureKey(credentials.secretAccessKey, date, region, service);
        var signature = AWS.util.crypto.hmac(signingKey, stringToSign, 'hex');

        canonicalQuerystring += '&X-Amz-Signature=' + signature;
        if (credentials.sessionToken) {
            canonicalQuerystring += '&X-Amz-Security-Token=' + encodeURIComponent(credentials.sessionToken);
        }

        var requestUrl = protocol + '://' + host + uri + '?' + canonicalQuerystring;
        return requestUrl;
    };

    /**
     * Generate a signing key
     * @param key the secret access key from the AWS credentials
     * @param date the date used to generate the signing key
     * @param region the region of the endpoint
     * @param service the service to use the signing key
    **/
    private getSignatureKey(key, date, region, service): any {
        var kDate = AWS.util.crypto.hmac('AWS4' + key, date, 'buffer');
        var kRegion = AWS.util.crypto.hmac(kDate, region, 'buffer');
        var kService = AWS.util.crypto.hmac(kRegion, service, 'buffer');
        var kCredentials = AWS.util.crypto.hmac(kService, 'aws4_request', 'buffer');
        return kCredentials;
    };
}