import { Component, OnInit, AfterViewInit, ViewChild} from '@angular/core';
import { FormBuilder, FormGroup, Validators } from '@angular/forms';
import { Http } from '@angular/http';
import { ToastsManager } from 'ng2-toastr/ng2-toastr';
import { DefinitionService, BreadcrumbService } from 'app/shared/service/index';
import { MetricGraph, MetricQuery } from 'app/view/game/module/shared/class/index';
import { AwsService } from 'app/aws/aws.service';
import { Router } from '@angular/router';

import { Heatmap } from 'app/view/game/module/analytic/component/heatmap/heatmap';

@Component({
    selector: 'heatmap',
    templateUrl: 'app/view/game/module/analytic/component/heatmap/heatmap.component.html',
    styles: [`
        .existing-heatmaps-container {
            margin-top: 25px;
        }
    `]
})

export class HeatmapComponent implements OnInit {
    mode: HeatmapMode = "List";
    metricsApiHandler: any;
    metricQuery: MetricQuery;

    heatmaps: Array<Heatmap>;
    selectedHeatmap: Heatmap;
    isLoadingHeatmaps: boolean = true;

    constructor(private fb: FormBuilder, private breadcrumbs: BreadcrumbService,
        private definition: DefinitionService, private aws: AwsService,
        private http: Http,
        private toastr: ToastsManager,
        private router: Router) { }

    ngOnInit() {
        this.initializeBreadcrumbs();
    }

    initializeBreadcrumbs = () => {
        this.breadcrumbs.removeAllBreadcrumbs();
        this.breadcrumbs.addBreadcrumb("Analytics", () => {
            this.router.navigateByUrl('/game/analytics');
        });
        this.breadcrumbs.addBreadcrumb("Heatmaps", () => {
            this.mode = "List";
            this.initializeBreadcrumbs();
        });
    }

    onHeatmapClick = (heatmap) => {
        this.selectedHeatmap = heatmap
        this.mode = "Create";
    }


    createHeatmap = () => {
        this.breadcrumbs.addBreadcrumb("Create Heatmap", null);
        this.selectedHeatmap = undefined
        this.mode = "Create";        
    }
}
export type HeatmapMode = "Create" | "Edit" | "Delete" | "List" | "Save" | "Update";