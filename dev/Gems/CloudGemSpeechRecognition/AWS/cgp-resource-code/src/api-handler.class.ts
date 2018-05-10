import { ApiHandler } from 'app/shared/class/index';
import { Http } from '@angular/http';
import { AwsService } from "app/aws/aws.service";
import { Measurable, TackableStatus } from 'app/view/game/module/cloudgems/class/index';
import { Observable } from 'rxjs/rx';
import { LyMetricService } from 'app/shared/service/index';

/**
 * API Handler for the Speech to Text RESTful services defined in the cloud gem swagger.json
*/
export class SpeechToTextApi extends ApiHandler {
    constructor(serviceBaseURL: string, http: Http, aws: AwsService, metric: LyMetricService, metricIdentifier: string) {
        super(serviceBaseURL, http, aws, metric, metricIdentifier);
    }

    public getBotCount(): Observable<any> {
        return super.get("admin/numbots");
    }

    public getBots(next_token): Observable<any> {
        return super.get("admin/listbots/" + next_token);
    }

    public getDesc(name, version): Observable<any> {
        return super.get("admin/botdesc/" + name + "/" + version);
    }

    public putDesc(body: any): Observable<any> {
        return super.put("admin/botdesc", body);
    }

    public publishBot(name, version): Observable<any> {
        return super.put("admin/publishbot/" + name + "/" + version);
    }

    public buildBot(name): Observable<any> {
        return super.put("admin/buildbot/" + name);
    }

    public getBotStatus(name): Observable<any> {
        return super.get("admin/botstatus/" + name);
    }

    public getBuiltinIntents(next_token): Observable<any> {
        return super.get("admin/listbuiltinintents/" + next_token);
    }

    public getCustomIntents(next_token): Observable<any> {
        return super.get("admin/listcustomintents/" + next_token);
    }

    public getBuiltinSlotTypes(next_token): Observable<any> {
        return super.get("admin/listbuiltinslottypes/" + next_token);
    }

    public getCustomSlotTypes(next_token): Observable<any> {
        return super.get("admin/listcustomslottypes/" + next_token);
    }

    public deleteBot(name): Observable<any> {
        return super.delete("admin/bot/" + name);
    }

    public deleteBotVersion(name, version): Observable<any> {
        return super.delete("admin/bot/version/" + name + "/" + version);
    }

    public deleteIntent(name): Observable<any> {
        return super.delete("admin/intent/" + name);
    }

    public deleteIntentVersion(name, version): Observable<any> {
        return super.delete("admin/intent/version/" + name + "/" + version);
    }

    public deleteSlotType(name): Observable<any> {
        return super.delete("admin/slottype/" + name);
    }

    public deleteSlotTypeVersion(name, version): Observable<any> {
        return super.delete("admin/slottype/version/" + name + "/" + version);
    }

    public getBot(name, version): Observable<any> {
        return super.get("admin/bot/" + name + "/" + version);
    }

    public updateBot(body): Observable<any> {
        return super.put("admin/bot", body);
    }

    public updateBotVersion(name): Observable<any> {
        return super.put("admin/bot/" + name);
    }

    public createIntent(body): Observable<any> {
        return super.put("admin/intent", body);
    }

    public updateIntentVersion(name): Observable<any> {
        return super.put("admin/intent/" + name);
    }

    public createSlotType(body): Observable<any> {
        return super.put("admin/slottype", body);
    }

    public updateSlotTypeVersion(name): Observable<any> {
        return super.put("admin/slottype/" + name);
    }

    public getIntent(name, version): Observable<any> {
        return super.get("admin/intent/" + name + "/" + version);
    }

    public getSlotType(name, version): Observable<any> {
        return super.get("admin/slottype/" + name + "/" + version);
    }

    public getBotAliases(name, next_token): Observable<any> {
        return super.get("admin/listbotaliases/" + name + "/" + next_token);
    }

    public updateBotAlias(body): Observable<any> {
        return super.put("admin/bot/alias", body);
    }

    public deleteBotAlias(name, bot_name): Observable<any> {
        return super.delete("admin/bot/alias/" + name + "/" + bot_name);
    }

    public getBotVersions(name, next_token): Observable<any> {
        return super.get("admin/bot/versions/" + name + "/" + next_token);
    }

    public getBotAlias(name, bot_name): Observable<any> {
        return super.get("admin/bot/alias/" + name + "/" + bot_name);
    }

    public getIntentVersions(name, next_token): Observable<any> {
        return super.get("admin/intent/versions/" + name + "/" + next_token);
    }

    public getSlotTypeVersions(name, next_token): Observable<any> {
        return super.get("admin/slottype/versions/" + name + "/" + next_token);
    }

    public getIntentDenendency(): Observable<any> {
        return super.get("admin/intentdependency");
    }

    public getBuiltinIntent(name): Observable<any> {
        return super.get("admin/builtinintent/" + name);
    }
}
//end rest api handler