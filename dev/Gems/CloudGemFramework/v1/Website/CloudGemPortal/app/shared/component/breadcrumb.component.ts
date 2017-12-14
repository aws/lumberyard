import { Component, OnInit } from '@angular/core';
import { BreadcrumbService } from '../service/breadcrumb.service';

@Component({
	selector: 'breadcrumbs',
	template: `
	<span *ngFor="let breadcrumb of breadcrumbService?.breadcrumbs; let i = index" [attr.data-index]="i">
		<ng-container [ngSwitch]="i + 1 === breadcrumbService?.breadcrumbs.length">
			<!-- If this is the last breadcrumb, make it a header and not a usable link -->
			<div *ngSwitchCase="true">
				<h1> {{breadcrumb.label}} </h1>
			</div>
			<span *ngSwitchCase="false" class="breadcrumb-link">
				<a (click)="breadcrumb.functionCb()"> {{breadcrumb.label}} </a>
				<i class="fa fa-angle-right" aria-hidden="true"></i>
			</span>
		</ng-container>
	</span>
	`,
    styleUrls: ['./app/shared/component/breadcrumb.component.css']
		
})
export class BreadcrumbComponent implements OnInit {
	constructor(private breadcrumbService: BreadcrumbService) {}

	ngOnInit() {}
}