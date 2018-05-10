import { ApiHandler } from 'app/shared/class/index';
import { Http } from '@angular/http';
import { AwsService } from "app/aws/aws.service";
import { Measurable, TackableStatus } from 'app/view/game/module/cloudgems/class/index';
import { Observable } from 'rxjs/Observable'
import { LyMetricService } from 'app/shared/service/index';
/**
 * API Handler for the Message of the Day RESTful services defined in the cloud gem swagger.json
*/
export class PlayerAccountApi extends ApiHandler {
    constructor(serviceBaseURL: string, http: Http, aws: AwsService, metric: LyMetricService = null, metricIdentifier: string = "") {
        super(serviceBaseURL, http, aws, metric, metricIdentifier);
    }
   
    public searchCognitoEmail(email: string): Observable<any> {
        return super.get("admin/accountSearch?CognitoIdentityId=&CognitoUsername=&Email=" + encodeURIComponent(email) + "&StartPlayerName=");
    }

    public searchCognitoUsername(name: string): Observable<any> {
        return super.get("admin/accountSearch?CognitoIdentityId=&CognitoUsername=" + encodeURIComponent(name) + "&Email=&StartPlayerName=");
    }

    public searchCognitoId(id: string): Observable<any> {
        return super.get("admin/accountSearch?CognitoIdentityId=" + encodeURIComponent(id) + "&CognitoUsername=&Email=&StartPlayerName=");
    }

    public searchPlayerName(name: string, paginationToken?: string): Observable<any> {
        if (paginationToken) {
            return super.get("admin/accountSearch?Email=&CognitoIdentityId=&CognitoUsername=&StartPlayerName=" + encodeURIComponent(name) + "&PageToken=" + encodeURIComponent(paginationToken));
        } else {
            return super.get("admin/accountSearch?Email=&CognitoIdentityId=&CognitoUsername=&StartPlayerName=" + encodeURIComponent(name));
        }
    }

    public searchAccountId(id: string): Observable<any> {
        if (id != "") {
            return super.get("admin/accounts/" + encodeURIComponent(id));
        }
        else {
            return super.get("admin/accountSearch");
        }
    }

    public searchBannedPlayers(id: string): Observable<any> {
        // TODO: Call search API for banned players here
        return super.get("admin/accounts/" + encodeURIComponent(id));
    }

    public emptySearch(): Observable<any> {
        return super.get("admin/accountSearch");
    }

    public editAccount(id: string, body: any) {
        return super.put("admin/accounts/" + encodeURIComponent(id), body);
    }
    public createAccount(body: any) {
        return super.post("admin/accounts", body);
    }
    public resetUserPassword(name: string) {
        return super.post("admin/identityProviders/Cognito/users/" + name + "/resetUserPassword");
    }
    public confirmUser(name: string) {
        return super.post("admin/identityProviders/Cognito/users/" + name + "/confirmSignUp");
    }
}
//end rest api handler