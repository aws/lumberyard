import { ApiHandler } from 'app/shared/class/index';
import { Http } from '@angular/http';
import { AwsService } from "app/aws/aws.service";
import { Measurable, TackableStatus } from 'app/view/game/module/cloudgems/class/index';
import { Observable } from 'rxjs/Observable';
import { LyMetricService } from 'app/shared/service/index';

/**
 * API Handler for the Text to Speech RESTful services defined in the cloud gem swagger.json
*/
export class TextToSpeechApi extends ApiHandler {
    constructor(serviceBaseURL: string, http: Http, aws: AwsService, metric: LyMetricService, metricIdentifier: string) {
        super(serviceBaseURL, http, aws, metric, metricIdentifier);
    }

    public preview(body: any): Observable<any> {
        return super.post("speechlib/preview", body);
    }

    public getSpeechLibrary(): Observable<any> {
        return super.get("speechlib");
    }

    public getCharacters(): Observable<any> {
        return super.get("characterlib");
    }

    public getCharacter(name): Observable<any> {
        return super.get("character/" + name);
    }

    public createCharacter(body): Observable<any> {
        return super.post("character", body);
    }

    public createSpeech(body): Observable<any> {
        return super.post("speechlib", body);
    }

    public deleteCharacter(name): Observable<any> {
        return super.delete("character/" + name);
    }

    public deleteSpeech(body): Observable<any> {
        return super.delete("speechlib", body);
    }
    public filter(body): Observable<any> {
        return super.post("speechlib/filter", body);
    }

    public getCharacterNames(): Observable<any> {
        return super.get("characters");
    }

    public generateZip(body): Observable<any> {
        return super.post("tts/exporter", body);
    }

    public getPackageUrl(name, inLib): Observable<any> {
        if (inLib)
        {
            return super.get("tts/exporter/library/" + name);
        }
        return super.get("tts/exporter/" + name);
    }

    public getVoices(): Observable<any> {
        return super.get("voicelist");
    }

    public importSpeeches(body): Observable<any> {
        return super.post("speechlib/import", body);
    }

    public enableRuntimeCapabilities(body): Observable<any> {
        return super.post("tts/runtimecapabilities", body);
    }

    public getRuntimeCapabilitiesStatus(): Observable<any> {
        return super.get("tts/runtimecapabilities");
    }

    public enableRuntimeCache(body): Observable<any> {
        return super.post("tts/runtimecache", body);
    }

    public getCacheRuntimeGeneratedFiles(): Observable<any> {
        return super.get("tts/runtimecache")
    }

    public saveCustomMappings(body): Observable<any> {
        return super.post("tts/custommappings", body);
    }

    public getCustomMappings(): Observable<any> {
        return super.get("tts/custommappings");
    }

    public getGeneratedPackages(): Observable<any> {
        return super.get("tts/generatedpackages");
    }

    public deleteGeneratedPackage(key): Observable<any> {
        return super.delete("tts/generatedpackage/" + key);
    }
}
//end rest api handler