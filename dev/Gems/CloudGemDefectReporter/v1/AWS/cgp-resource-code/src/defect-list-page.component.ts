import { Input, Output, EventEmitter, Component, ViewChild } from '@angular/core';
import { AbstractCloudGemIndexComponent } from 'app/view/game/module/cloudgems/class/index';
import { Http } from '@angular/http';
import { AwsService } from "app/aws/aws.service";
import { ToastsManager } from 'ng2-toastr/ng2-toastr';
import { LyMetricService } from 'app/shared/service/index';
import { CloudGemDefectReporterApi } from './index';
import { ActivatedRoute } from '@angular/router';

 
@Component({
    selector: 'defect-list-page',
    templateUrl: 'node_modules/@cloud-gems/cloudgemdefectreporter/defect-list-page.component.html',
    styleUrls: ['node_modules/@cloud-gems/cloudgemdefectreporter/defect-list-page.component.scss']
})

export class CloudGemDefectReporterDefectListPageComponent {    
    @Input() context: any;
    @Input() metricApiHandler: any;
    @Input() isJiraIntegrationEnabled: any;
    @Input() toDefectDetailsPageCallback: Function;

    @ViewChild('defectListOverviewTab') defectListOverviewTab;
    @ViewChild('bookmarktable') bookmarktable;
    @ViewChild('facetgenerator') facetgenerator;

    public configurationMappings: Object[];
    private _apiHandler: CloudGemDefectReporterApi;

    private columns = [];
    private allColumns = [];
    private tabIndex = 0;
    private partialInputQuery: string = "";
    public tableHeaders = [];

    private isLoadingRecentSearches: boolean;

    constructor(private http: Http, private aws: AwsService, private metric: LyMetricService, private toastr: ToastsManager, private route: ActivatedRoute) {
    }
  

    ngOnInit() {        
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
    * Switch to the overview tab and run a recent search when the user clicks on an entry in the recent searches table
    * @param query_params The query parameters for the recent search
    **/
    public runRecentSearch(query_params: string): void {
        this.partialInputQuery = query_params == "*" ? "" : query_params;
        this.defectListOverviewTab.partialInputQuery = this.partialInputQuery;

        this.facetgenerator.emitActiveTab(0);
        this.defectListOverviewTab.searchButton.nativeElement.click();
    }


    /**
    * Switch to the Jira Integration tab
    **/
    public updateJiraMappings(): void {
        this.facetgenerator.emitActiveTab(6);
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
}