import { Component, OnInit, Input, Output, EventEmitter } from '@angular/core';
import { Router } from '@angular/router';
import { trigger, state, style, animate, transition } from '@angular/animations';
import { AwsService } from 'app/aws/aws.service';
import { DefinitionService, LyMetricService, GemService } from 'app/shared/service/index';
import { Observable } from 'rxjs';
import { Gemifiable } from 'app/view/game/module/cloudgems/class/gem-interfaces';

@Component({
    selector: 'cgp-sidebar',
    templateUrl: "app/view/game/component/sidebar.component.html",
    styleUrls: [ 'app/view/game/component/sidebar.component.css' ],
    animations: [
        trigger('sidebarState', [
            state('Collapsed', style ({
                'width': '75px'
            })),
            state('Expanded', style ({
                'width': '265px'
            })),
            transition('Collapsed => Expanded', animate('300ms ease-in')),
            transition('Expanded => Collapsed', animate('300ms ease-out'))
        ])
    ]
})

export class SidebarComponent implements OnInit {
    sidebarStateVal = "Expanded";
    isViewingGem: boolean;
    isLoadingGems: boolean;

    @Input()
    get sidebarState() {
        return this.sidebarStateVal;
    }

    @Output() sidebarStateChange = new EventEmitter();

    set sidebarState(val) {
        this.sidebarStateVal = val;
        this.sidebarStateChange.emit(this.sidebarStateVal);
    }

    constructor(
        private aws: AwsService,
        private lymetrics: LyMetricService,
        protected gemService: GemService,
        private router: Router
    ) { }

    ngOnInit( ) { }

    // TODO: Implement a routing solution for metrics that covers the whole website.
    raiseRouteEvent(route: string): void {
        this.lymetrics.recordEvent('FeatureOpened', {
            "Name": route
        })
        this.router.navigate([route]);
    }
}

export type SidebarState = "Expanded" | "Collapsed";
