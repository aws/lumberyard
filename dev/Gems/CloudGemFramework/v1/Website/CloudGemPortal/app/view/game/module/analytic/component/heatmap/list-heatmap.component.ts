import { Component, OnInit, ViewChild, ElementRef, Input, EventEmitter, Output, ChangeDetectorRef } from '@angular/core';
import { Http } from '@angular/http';
import { ToastsManager } from 'ng2-toastr/ng2-toastr';
import { DefinitionService, BreadcrumbService } from 'app/shared/service/index';
import { AwsService } from 'app/aws/aws.service';
import { ModalComponent } from 'app/shared/component/index';
import { Heatmap } from 'app/view/game/module/analytic/component/heatmap/heatmap';

@Component({
    selector: 'list-heatmap',
    templateUrl: 'app/view/game/module/analytic/component/heatmap/list-heatmap.component.html',
    styles: [`
    `]
})

export class ListHeatmapComponent implements OnInit {
    @Output() onHeatmapClick: any = new EventEmitter();

    @ViewChild(ModalComponent) modalRef: ModalComponent;

    metricsApiHandler: any;
    heatmaps: Array<Heatmap>;
    currentHeatmap: Heatmap;
    isLoadingHeatmaps: boolean = true;
    mode: string = "List";

    constructor(
        private breadcrumbs: BreadcrumbService,
        private definition: DefinitionService,
        private aws: AwsService,
        private http: Http,
        private toastr: ToastsManager) { }

    ngOnInit() {
        // Get Metrics Data
        let metricServiceUrl = this.definition.getService("CloudGemMetric")
        this.metricsApiHandler = new metricServiceUrl.constructor(metricServiceUrl.serviceUrl, this.http, this.aws)
        this.heatmaps = new Array<Heatmap>();
        this.refreshList()
    }

    refreshList = () => {
        // Get existing heatmaps
        this.isLoadingHeatmaps = true;
        this.heatmaps = [];
        this.metricsApiHandler.listHeatmaps().subscribe(response => {
            let heatmapObjs = JSON.parse(response.body.text()).result[0].value;
            // Get rid of extra array for easier use
            heatmapObjs.map(heatmap => {
                this.heatmaps.push(heatmap.heatmap);
            })
            this.isLoadingHeatmaps = false;
        }, err => {
            this.toastr.error("Unable to retrieve existing heatmap data", err);
        })
    }

    editHeatmap = (heatmap) => {
        this.onHeatmapClick.emit(heatmap);
        this.breadcrumbs.addBreadcrumb("Edit Heatmap", null);
    }

    dismissModal = () => {
        this.mode = "List";
    }

    deleteHeatmapModal = (heatmap) => {
        this.mode = "Delete";
        this.currentHeatmap = heatmap;
    }

    deleteHeatmap = (heatmap) => {
        this.isLoadingHeatmaps = false;
        this.metricsApiHandler.deleteHeatmap(heatmap.id).subscribe(response => {
            this.toastr.success(`The heatmap '${heatmap.id}' has been deleted.`);
            this.modalRef.close();
            this.refreshList()
        }, err => {
            this.toastr.error("Unable to delete heatmap", err);
            this.modalRef.close();
        })
    }
}


