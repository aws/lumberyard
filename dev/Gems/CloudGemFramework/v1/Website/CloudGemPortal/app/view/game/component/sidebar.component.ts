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
    cloudGems: Observable<Array<Gemifiable>>;
    isViewingGem: boolean;

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

    ngOnInit() { }

    loadGemIndex(gem: Gemifiable) {
        if(gem) {
            this.gemService.currentGem = null;
        }
    }
}

export type SidebarState = "Expanded" | "Collapsed";
