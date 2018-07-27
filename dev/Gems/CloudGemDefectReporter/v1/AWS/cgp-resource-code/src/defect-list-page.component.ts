import { Input, Output, EventEmitter, Component, ViewChild } from '@angular/core';
import { AbstractCloudGemIndexComponent } from 'app/view/game/module/cloudgems/class/index';
import { MetricQuery } from 'app/view/game/module/shared/class/index';
import { Http } from '@angular/http';
import { AwsService } from "app/aws/aws.service";
import { MetricGraph, MetricBarData } from 'app/view/game/module/shared/class/metric-graph.class';
import { Queries } from './index' ;
import { ToastsManager } from 'ng2-toastr/ng2-toastr';
import { Observable } from 'rxjs/Observable';
import { UrlService, LyMetricService } from 'app/shared/service/index';
import { CloudGemDefectReporterApi } from './index';
import { Clipboard } from 'ts-clipboard';
import { ActivatedRoute } from '@angular/router';

 
@Component({
    selector: 'defect-list-page',
    templateUrl: 'node_modules/@cloud-gems/cloudgemdefectreporter/defect-list-page.component.html',
    styleUrls: ['node_modules/@cloud-gems/cloudgemdefectreporter/defect-list-page.component.scss']
})

export class CloudGemDefectReporterDefectListPageComponent {    
    @Input() context: any;
    @Input() metricApiHandler: any;
    @Input() toDefectDetailsPageCallback: Function;

    @ViewChild('datatable') datatable;
    @ViewChild('bookmarktable') bookmarktable;
    @ViewChild('facetgenerator') facetgenerator;

    public limit:string = '50';
    public queryGraph: MetricGraph;
    public configurationMappings: Object[];
    private metricQueryBuilder: MetricQuery;
    private _apiHandler: CloudGemDefectReporterApi;

    private columns = [];
    private allColumns = [];
    private tabIndex = 0;
    private partialInputQuery: string = "";

    private recentSearches = [];
    private isLoadingRecentSearches: boolean;

    constructor(private http: Http, private aws: AwsService, private urlService: UrlService, private metric: LyMetricService, private toastr: ToastsManager, private route: ActivatedRoute) {
    }
  

    ngOnInit() {
        this.metricQueryBuilder = new MetricQuery(this.aws, "defect");
        this.queryGraph = this.queryLastXHours(24);
        this._apiHandler = new CloudGemDefectReporterApi(this.context.ServiceUrl, this.http, this.aws, this.metric, this.context.identifier);

        this.route.queryParams.subscribe(params => {
            this.partialInputQuery = params['params'] ? params['params'] : this.partialInputQuery;
        })
        this.initializeConfigurationMappings();
    }

    /**
    * Switch to a new tab
    * @param subNavItemIndex Index of the new tab
    **/
    public getSubNavItem(subNavItemIndex: number): void {
        this.tabIndex = subNavItemIndex;

        this.update();
    }
    
    /**
    * Setter for limit to be used in query.
    * @param limit  String limit to be sent along with ANSI SQL query.
    **/
    public setLimit(limit:string): void {
        this.limit = limit;
    }

    
    /**
    * Return metric api handler to be passed into other components
    **/
    public getMetricApiHandler(): void {
        return this.metricApiHandler;
    }

    /**
    * Return defect reporter api handler to be passed into other components
    **/
    public getDefectReporterApiHandler(): CloudGemDefectReporterApi {
        return this._apiHandler;
    }

    /**
    * Updates the visible graph.
    * @param graph MetricGraph object
    **/
    public updateGraph(graph: MetricGraph): void {
        this.queryGraph = graph;
    }

    /**
    * Switch to the overview tab and run a recent search when the user clicks on an entry in the recent searches table
    * @param query_params The query parameters for the recent search
    **/
    public runRecentSearch(query_params: string): void {
        this.facetgenerator.emitActiveTab(0);
        this.partialInputQuery = query_params == "*" ? "" : query_params;
    }

    /**
    * Initialize the report configuration mappings
    **/
    private initializeConfigurationMappings(): void {
        let jsonString = localStorage.getItem("configurationMappings");
        if (jsonString) {
            this.configurationMappings = JSON.parse(jsonString);
        }
        else {
            this.configurationMappings = [
                { 'key': 'report_status', 'displayName': 'Status', 'category': 'Report Information' },
                { 'key': 'client_timestamp', 'displayName': 'Time', 'category': 'Report Information' },
                { 'key': 'p_event_source', 'displayName': 'Source', 'category': 'Report Information' },
                { 'key': 'p_event_name', 'displayName': 'Category', 'category': 'Report Information' },
                { 'key': 'universal_unique_identifier', 'displayName': 'Report ID', 'category': 'Report Information' },
                { 'key': 'annotation', 'displayName': 'Player text', 'category': 'Report Information' },
                { 'key': 'unique_user_identifier', 'displayName': 'Player ID', 'category': 'Player Information' },
                { 'key': 'locale', 'displayName': 'Locale', 'category': 'Player Information' },
                { 'key': 'client_ip_address', 'displayName': 'IP address', 'category': 'Player Information' },
                { 'key': 'p_client_build_identifier', 'displayName': 'Build Version', 'category': 'System Information' },
                { 'key': 'platform_identifier', 'displayName': 'Platform', 'category': 'System Information' }
            ]
        }
    }

    /**
    * Creates a graph with data from the last x hours.
    * @param hour Number representing the number of hours to count back from.
    * @returns MetricGraph object with updated labels and query.
    **/
    private queryLastXHours(hour:number): MetricGraph {
        let title = "Number of Defects per Hour";
        let xAxis = "Hours";
        let yAxis = "Number of Defects";

        return new MetricGraph(
            title, 
            xAxis, 
            yAxis, 
            [], 
            [this.postQuery(Queries.lastXHours(hour))], 
            [MetricBarData], 
            "ngx-charts-bar-vertical", 
            [], 
            []
        );
    }


    /**
    * Creates a graph with data from the last x days.
    * @param day Number that represents the number of days to count back from.
    * @returns MetricGraph object with updated labels and query.
    **/
    private queryLastXDays(day:number): MetricGraph {
        let title = "Number of Defects per Day";
        let xAxis = "Days";
        let yAxis = "Number of Defects";

        return new MetricGraph(
            title, 
            xAxis, 
            yAxis, 
            [], 
            [this.postQuery(Queries.lastXDays(day))], 
            [MetricBarData], 
            "ngx-charts-bar-vertical", 
            [], 
            []
        );
    }


    /**
    * Creates graph with data from the last x months.
    * @param month Number that represents the number of months to count back from.
    * @returns MetricGraph object with updated labels and query.
    **/
    private queryLastXMonths(month:number): MetricGraph {
        let title = "Number of Defects per Month";
        let xAxis = "Months";
        let yAxis = "Number of Defects";

        return new MetricGraph(
            title, 
            xAxis, 
            yAxis, 
            [], 
            [this.postQuery(Queries.lastXMonths(month))], 
            [MetricBarData], 
            "ngx-charts-bar-vertical", 
            [], 
            []
        );
    }


    /**
    * Observable wrapper for metric api handler call to post query.
    * @param query ANSI SQL string without table programmatically populated.
    **/
    private postQuery(query:string): Observable<any> {
        return this.metricApiHandler.postQuery(this.metricQueryBuilder.toString(query));
    }

    /**
    * Update the content on display when switching to a new tab
    **/
    private update(): void {
        if (this.tabIndex !== 3) {
            return
        }

        this.isLoadingRecentSearches = true;
        this._apiHandler.getRecentSearches(this.aws.context.authentication.user.username).subscribe(
            response => {
                let obj = JSON.parse(response.body.text());
                this.recentSearches = obj.result;
                this.isLoadingRecentSearches = false;
            },
            err => {
                this.toastr.error("Failed to load the recent searches. ", err);
                this.isLoadingRecentSearches = false;
            }
        );
    }

    /**
    * Share a recent search record
    * @param recentSearch The recent search to share
    **/
    public shareRecentSearch(recentSearch: Object): void {
        let url = this.urlService.baseUrl;
        url = url.indexOf("deployment=") === -1 ? url : url.slice(0, url.indexOf("deployment=") - 1);
        url += url.indexOf("?") == -1 ? "?deployment=" : "&deployment="
        url += this.aws.context.project.activeDeployment.settings.name + "&target=CloudGemDefectReporter&params=" + encodeURIComponent(recentSearch['query_params']);


        Clipboard.copy(url);
        this.toastr.success("The shareable search URL has been copied to your clipboard.", "Copy To Clipboard");
    }
}