import { ApiHandler } from 'app/shared/class/index';
import { Http } from '@angular/http';
import { AwsService } from "app/aws/aws.service";
import { Observable } from "rxjs/Observable";
import { BehaviorSubject } from 'rxjs/BehaviorSubject';
import 'rxjs/add/observable/merge'
import 'rxjs/add/operator/filter'
import 'rxjs/add/observable/throw'

/**
 * API Handler for the RESTful services defined in the cloud gem swagger.json
*/
export class CloudGemMetricApi extends ApiHandler {

    constructor(private serviceBaseURL: string, http: Http, aws: AwsService) {
        super(serviceBaseURL, http, aws);

    }

    getDashboard(facetid: string): Observable<any> {        
        return super.get('dashboard/' + facetid.toLocaleLowerCase().trim()).map(r => {
            let result = JSON.parse(JSON.parse(r.body.text()).result).Items;
            if (result.length > 0)
                return result[0].value
            else
                return result
        })
    }

    postDashboard(facetid: string, body: any): Observable<any> {
        return super.post('dashboard/' + facetid, body);
    }
    
    getCloudWatchMetrics(namespace: string, metric_name: string, dimension_name: string, dimension_value: string, aggregation_type: string, aggregation_period_in_seconds: number, period_of_time_in_hours: number): Observable<any> {
        return super.get(`graph/cloudwatch?namespace=${namespace}&metric_name=${metric_name}&dimension_name=${dimension_name}&dimension_value=${dimension_value}&aggregation_type=${aggregation_type}&period_in_seconds=${aggregation_period_in_seconds}&time_delta_hours=${period_of_time_in_hours}`);
    }

    getCurrentDeleteDuration(): Observable<any> {
        return super.get('graph/consumer/current/duration/delete');
    }

    getCurrentSaveDuration(): Observable<any> {
        return super.get('graph/consumer/current/duration/save');
    }

    getDeleteDuration(): Observable<any> {
        return super.get('graph/consumer/delete/duration');
    }
       
    getProcessedBytes(): Observable<any> {
        return super.get('graph/consumer/processed/bytes');
    }

    getProcessedMessages(): Observable<any> {
        return super.get('graph/consumer/processed/messages');
    }

    getRowsAdded(): Observable<any> {
        return super.get('graph/consumer/rows/added');
    }

    getSaveDuration(): Observable<any> {
        return super.get('graph/consumer/save/duration');
    }
  
    getPredefinedPartitions(): Observable<any> {
        return super.get('partition/predefined');
    }

    getCustomPartitions(): Observable<any> {
        return super.get('partition/custom');
    }

    getDuplicationRate(): Observable<any> {
        return super.get('athena/query/duplication/rate');
    }

    updatePartitions(body: any): Observable<any> {
        return super.post("partition", body);
    }

    getFilters(): Observable<any> {
        return super.get('filter');
    }

    updateFilters(body: any): Observable<any> {
        return super.post("filter", body);
    }

    getSettings(): Observable<any> {
        return super.get("setting")
    }

    updateSettings(body: any): Observable<any> {
        return super.post("setting", body);
    }

    getFilterEvents(): Observable<any> {
        return super.get('athena/query/events');
    }

    getFilterPlatforms(): Observable<any> {
        return super.get('athena/query/platforms');
    }

    postQuery(body: any): Observable<any> {

        let query_observable = new BehaviorSubject<any>(null);
        let obs = super.post('athena/query', { "sql": body }).map(r => {            
            let query_id = JSON.parse(r.body.text()).result
            this.getQueryStatus(query_id, query_observable);
        }, error => {
            query_observable.error(error)
            })
        return Observable.merge(query_observable, obs).filter(r => r != undefined)
    }

    getQueryStatus(id: string, observable: BehaviorSubject<any>) {
        super.get('athena/query/' + id).subscribe(r2 => {
            let data = JSON.parse(r2.body.text()).result;
            let status_id = data['Status']['State']
            if (status_id == 'SUCCEEDED' || status_id == 'FAILED' || status_id == 'CANCELLED') {
                observable.next(data)                  
            } else {
                setTimeout(this.getQueryStatus(id, observable), 750)
            }
        }, error => {
            observable.error(error)
        })
    }


    getCrawlerStatus(name:string): Observable<any> {
        return super.get('glue/crawler/crawl/status?name='+name)
    }

    getStatus(): Observable<any> {
        return super.get('service/status')
    }

    /**
     * Invokes the SQS FIFO consumer
     */
    invokeConsumer(): Observable<any> {
        return super.post("consumer/consume/message");
    }
    /**
     * Invoke the amoeba S3 file aggregator.  This will help optimize Athena queries
     */
    invokeAmoeba(): Observable<any> {
        return super.post("amoeba/consume/file");
    }
    /**
     * Invokes AWS GLUE lambda crawler
     */
    invokeCrawler(): Observable<any> {
        return super.post("glue/crawler/crawl");
    }

    /** Save the current heatmap */
    createHeatmap(heatmap): Observable<any> {
        return super.post("heatmaps", heatmap);
    }

    /** Get a specific heatmap */
    getHeatmap(heatmapName): Observable<any> {
        return super.get("heatmaps/" + heatmapName);
    }

    /** Delete a specific heatmap */
    deleteHeatmap(heatmapName): Observable<any> {
        return super.delete("heatmaps/" + heatmapName);
    }

    /** List all heatmaps */
    listHeatmaps(): Observable<any> {
        return super.get("heatmaps");
    }

    /** Update an existing heatmap */
    updateHeatmap(heatmapName, heatmap): Observable<any> {
        return super.put("heatmaps/" + heatmapName, heatmap);
    }


}