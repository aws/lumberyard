import { ApiHandler } from 'app/shared/class/index';
import { Http } from '@angular/http';
import { AwsService } from "app/aws/aws.service";
import { Measurable, TackableStatus } from 'app/view/game/module/cloudgems/class/index';
import { Observable } from 'rxjs/Observable';
import { LyMetricService } from 'app/shared/service/index';

export class InGameSurveyApi extends ApiHandler {
    constructor(serviceBaseURL: string, http: Http, aws: AwsService, metric: LyMetricService = null, metricIdentifier: string = "") {
        super(serviceBaseURL, http, aws, metric, metricIdentifier);
    }

    public createQuestion(surveyId: string, question: any): Observable<any> {
        return super.post("surveys/" + encodeURIComponent(surveyId) + "/questions", question);
    }

    public modifyQuestion(surveyId: string, questionId: string, question: any): Observable<any> {
        return super.put("surveys/" + encodeURIComponent(surveyId) + "/questions/" + encodeURIComponent(questionId), question);
    }

    public deleteQuestion(surveyId: string, questionId: string): Observable<any> {
        return super.delete("surveys/" + encodeURIComponent(surveyId) + "/questions/" + encodeURIComponent(questionId));
    }

    public enableQuestion(surveyId: string, questionId: string, enabled: boolean): Observable<any> {
        return super.put("surveys/" + encodeURIComponent(surveyId) + "/questions/" + encodeURIComponent(questionId) + "/status", {"enabled": enabled});
    }

    public createSurvey(surveyName: string): Observable<any> {
        return super.post("surveys", { "survey_name": surveyName });
    }

    public cloneSurvey(surveyName: string, surveyId: string): Observable<any> {
        return super.post("surveys", { "survey_name": surveyName, survey_id_to_clone: surveyId});
    }

    public getSurvey(surveyId: string): Observable<any> {
        return super.get("surveys/" + encodeURIComponent(surveyId));
    }

    public deleteSurvey(surveyId: string): Observable<any> {
        return super.delete("surveys/" + encodeURIComponent(surveyId));
    }

    public getSurveyMetadataList(surveyName: string, paginationToken: string): Observable<any> {
        let queryStr = "";

        if (paginationToken) {           
            queryStr += "pagination_token=" + encodeURIComponent(paginationToken);
        }
       
        if (surveyName) {
            if (queryStr) {
                queryStr += "&";
            }
            queryStr += "survey_name=" + encodeURIComponent(surveyName);
        }

        if (queryStr) {
            queryStr = "?" + queryStr;
        }

        return super.get("survey_metadata/" + queryStr);
    }

    public getSurveyAnswerAggregations(surveyId: string): Observable<any> {
        return super.get("surveys/" + encodeURIComponent(surveyId) + "/answer_aggregations");
    }

    public getSurveyAnswerSubmissions(surveyId: string, paginationToken: string, limit: number, sort: string): Observable<any> {
        let queryString = "";
        if (paginationToken) {
            queryString = "?limit=" + String(limit) + "&pagination_token=" + encodeURIComponent(paginationToken) + "&sort=" + sort
        } else {
            queryString = "?limit=" + String(limit) + "&sort=" + sort
        }

        return super.get("surveys/" + encodeURIComponent(surveyId) + "/answers" + queryString);
    }

    public deleteAnswerSubmission(surveyId: string, submissionId: string): Observable<any> {        
        return super.delete("surveys/" + encodeURIComponent(surveyId) + "/answers/" + encodeURIComponent(submissionId));
    }

    public setSurveyActivationPeriod(surveyId: string, activationStartTime: number, activationEndTime: number): Observable<any> {
        let body = {
            "activation_start_time": activationStartTime
        };

        if (activationEndTime) {
            body["activation_end_time"] = activationEndTime;
        }

        return super.put("surveys/" + encodeURIComponent(surveyId) + "/activation_period", body);
    }

    public publishSurvey(surveyId: string): Observable<any> {
        return super.put("surveys/" + encodeURIComponent(surveyId) + "/published", { "published": true });
    }

    public setSurveyQuestionOrder(surveyId: string, questionOrder: any): Observable<any> {
        return super.put("surveys/" + encodeURIComponent(surveyId) + "/question_order", questionOrder);
    }

    public exportSurveyAnswerSubmissionsToCSV(surveyId: string): Observable<any> {
        return super.post("surveys/" + encodeURIComponent(surveyId) + "/answers/export_csv", {});
    }

    public getExportSurveyAnswerSubmissionsToCSVStatus(surveyId: string, requestId: string): Observable<any> {
        return super.get("surveys/" + encodeURIComponent(surveyId) + "/answers/export_csv/" + encodeURIComponent(requestId));
    }
}