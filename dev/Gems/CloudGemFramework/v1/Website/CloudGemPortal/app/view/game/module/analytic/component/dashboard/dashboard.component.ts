import { Component, OnInit, Input, Output, AfterViewInit } from '@angular/core';
import { FormBuilder, FormGroup, Validators } from '@angular/forms';
import { Router } from '@angular/router';
import { ToastsManager } from 'ng2-toastr/ng2-toastr';
import { MetricGraph, MetricQuery } from 'app/view/game/module/shared/class/index';
import { Http } from '@angular/http';
import { AwsService } from "app/aws/aws.service";
import { DefinitionService, BreadcrumbService } from 'app/shared/service/index';
import { ProjectApi } from 'app/shared/class/index';


@Component({
    selector: 'analytic-dashboard',
    template: `
        <breadcrumbs></breadcrumbs>
        <facet-generator [context]="context"
        [tabs]="tabs"
        (tabClicked)="idx=$event"
        ></facet-generator>
        <div *ngIf="idx === 0">            
            <dashboard-graph [facetid]="tabs[idx]"></dashboard-graph>
        </div>      
        <div *ngIf="idx === 1">
            <dashboard-graph [facetid]="tabs[idx]"></dashboard-graph>
        </div>      
    `,
    styles: [`
        .dashboard-heading {
            font-size: 24px;
            padding-left: 0;
        }
    `]
})
export class DashboardComponent implements OnInit {    
    private context: any
    private tabs = ['Telemetry', 'Engagement']

    constructor(private fb: FormBuilder,
        private http: Http,
        private aws: AwsService,
        private definition: DefinitionService,
        private breadcrumbs: BreadcrumbService,
        private router: Router) {
      
     
    }

    ngOnInit() {
        this.initializeBreadcrumbs();
    }

    initializeBreadcrumbs = () => {
        this.breadcrumbs.removeAllBreadcrumbs();
        this.breadcrumbs.addBreadcrumb("Analytics", () => {
            this.router.navigateByUrl('/game/analytics');
        });
        this.breadcrumbs.addBreadcrumb("Dashboard", () => {            
            this.initializeBreadcrumbs();
        });
    }
    
}