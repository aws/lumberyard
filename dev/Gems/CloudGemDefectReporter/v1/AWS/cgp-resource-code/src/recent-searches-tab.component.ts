import { Input, Component, Output, EventEmitter } from '@angular/core';
import { UrlService, LyMetricService } from 'app/shared/service/index';
import { AwsService } from "app/aws/aws.service";
import { ToastsManager } from 'ng2-toastr/ng2-toastr';
import { Clipboard } from 'ts-clipboard';

@Component({
    selector: 'recent-searches-tab',
    templateUrl: 'node_modules/@cloud-gems/cloudgemdefectreporter/recent-searches-tab.component.html',
    styleUrls: ['node_modules/@cloud-gems/cloudgemdefectreporter/defect-list-page.component.css']
})


export class CloudGemDefectReporterRecentSearchesTabComponent {
    @Input() defectReporterApiHandler: any;
    @Output() runRecentSearch = new EventEmitter<any>();

    private isLoading: boolean;
    private recentSearches = [];

    constructor(private aws: AwsService, private urlService: UrlService, private toastr: ToastsManager, private metric: LyMetricService) {
    }

    ngOnInit() {
        this.isLoading = true;
        this.defectReporterApiHandler.getRecentSearches(this.aws.context.authentication.user.username).subscribe(
            response => {
                let obj = JSON.parse(response.body.text());
                this.recentSearches = obj.result;
                this.isLoading = false;
            },
            err => {
                this.toastr.error("Failed to load the recent searches. ", err);
                this.isLoading = false;
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