import { Injectable } from '@angular/core';
import { ApiHandler } from 'app/shared/class/index';
import { Http } from '@angular/http';
import { Observable } from 'rxjs/Observable';
import { AwsService } from 'app/aws/aws.service';
import * as environment from 'app/shared/class/index'

declare var AMA
declare var bootstrap
declare var AWS

export type Event = "CloudGemsLoaded"    
    | "ProjectUrlSharedSuccess"
    | "ProjectUrlSharedFailed"
    | "DeploymentChanged"
    | "ProjectSettingsInitialized"
    | "ModalOpened"
    | "FeatureOpened"
    | "FacetOpened"
    | "ApiServiceError"
    | "ApiServiceSuccess"
    | "ApiServiceRequested"
    | "LoggedOut"

@Injectable()
export class LyMetricService {    

    private _service: any = undefined;

    constructor() {
        let creds = new AWS.CognitoIdentityCredentials({
            IdentityPoolId: environment.isProd ? "us-east-1:966bee25-4446-4681-84e5-891fe7b3fbd0" : "us-east-1:5e212aee-a4c4-4670-ae69-18c3d7044199"
        }, {
                region: 'us-east-1'
            })        
        this._service = new AMA.Manager({
            appTitle: "Cloud Gem Portal",
            appId: environment.isProd ? "1f52bbba6ceb434daca8aa30f6d1b8f2" : "e1e41196e1c74f88ae970266d68a8ecc",
            appVersionCode: "v1.0",
            platform: navigator.appName,
            platformVersion: navigator.appVersion,
            model: navigator.userAgent,
            autoSubmitEvents: true,
            autoSubmitInterval: 5000,
            provider: creds,
            globalAttributes: {
                "Region": bootstrap.region,
                "UserAgent": navigator.userAgent ? navigator.userAgent.toLowerCase() : navigator.userAgent,
                "CognitoDev": bootstrap.cognitoDev,
                "CognitoTest": bootstrap.cognitoTest,
                "CognitoProd": bootstrap.cognitoProd
            },
            globalMetrics: {

            },

            clientOptions: {
                region: 'us-east-1',
                credentials: creds
            }
        }
        )
    }
    
    public recordEvent(eventName: Event, attribute: { [name: string]: string; } = {}, metric: { [name: string]: number; } = null) {
        if (this.isAuthorized(eventName, attribute))
            this._service.recordEvent(eventName, attribute, metric);        
    }

    private isAuthorized(eventName: Event, attribute: any) {
        if (eventName == "FeatureOpened") {
            let featureRoute = attribute.Name.toLowerCase()            
            for (let feature in environment.metricWhiteListedFeature) {
                let normalizedName = environment.metricWhiteListedFeature[feature].trim().replace(' ', '').toLowerCase()
                if (featureRoute.indexOf(normalizedName) >= 0) {
                    return true;
                }
            }
            return false;
        }

        if (eventName == "ModalOpened"
            || eventName == "FacetOpened"
            || eventName == "ApiServiceError"
            || eventName == "ApiServiceSuccess"
            || eventName == "ApiServiceRequested") { 
            if (!attribute.Identifier)
                return false;
            let identifier = attribute.Identifier.trim().replace(' ', '').toLowerCase()
            for (let index in environment.metricWhiteListedCloudGem) {
                let normalizedName = environment.metricWhiteListedCloudGem[index].trim().replace(' ', '').toLowerCase()
                if (identifier.indexOf(normalizedName) >= 0) {
                    return true;
                }
            }
            for (let index in environment.metricWhiteListedFeature) {
                let normalizedName = environment.metricWhiteListedFeature[index].trim().replace(' ', '').toLowerCase()
                if (identifier.indexOf(normalizedName) >= 0) {
                    return true;
                }
            }
            return false;
        }

        if (eventName == "CloudGemsLoaded") {
            let cloudGemsInDeployment = attribute.CloudGemsInDeployment.toLowerCase()
            
            attribute.CloudGemsInDeployment = environment.metricWhiteListedCloudGem.filter(gem => {                
                return cloudGemsInDeployment.indexOf(gem.trim().replace(' ', '').toLowerCase()) >= 0
            }).join(',')
            return true;
        }

        return true;
    }
}
