import { ApiHandler } from 'app/shared/class/index';
import { Http } from '@angular/http';
import { AwsService } from "app/aws/aws.service";
import { Measurable, TackableStatus } from 'app/view/game/module/cloudgems/class/index';
import { Observable } from 'rxjs/Observable'
import { LyMetricService } from 'app/shared/service/index';

/**
 * API Handler for the Message of the Day RESTful services defined in the cloud gem swagger.json
*/
export class LeaderboardApi extends ApiHandler {
    constructor(serviceBaseURL: string, http: Http, aws: AwsService, metric: LyMetricService = null, metricIdentifier: string = undefined) {
        super(serviceBaseURL, http, aws, metric, metricIdentifier);
    }

    public getEntries(leaderboard: string, body: any): Observable<any> {
        return super.post("leaderboard/" + leaderboard, body);
    }

    public deleteLeaderboard(id: string): Observable<any> {
        return super.delete("stats/" + id);
    }

    public addLeaderboard(body: any): Observable<any> {
        return super.post("stats", body);
    }

    public editLeaderboard(body: any): Observable<any> {
        return super.post("stats", body);
    }

    public searchLeaderboard(leaderboardid: string, id: string): Observable<any> {
        return super.get("score/" + leaderboardid + "/" + id);
    }

    public deleteScore(leaderboardId: string, currentScore: any): Observable<any> {
        return super.delete("score/" + leaderboardId + "/" + currentScore);
    }

    public banPlayer(user: string): Observable<any> {
        return super.post("player/ban/" + user);
    }

    public unbanPlayer(user: string): Observable<any> {
        return super.delete("player/ban/" + encodeURIComponent(user));
    }

    public processRecords(): Observable<any> {
        return super.get("process_records");
    }

}
//end rest api handler