import { Component, OnInit, Input, Output, EventEmitter } from '@angular/core';

@Component({
    selector: 'sub-nav',
    template: `
    <div class="row subNav">
        <div class="tab"  [class.tab-active]="activeTab == i"  *ngFor="let tab of tabs; let i = index">
            <a (click)="emitActiveTab(i)"> {{tabs[i]}} </a>
        </div>
    </div>
    `,
    styleUrls: ['app/view/game/module/shared/component/subnav.component.css']
})
export class SubNavComponent implements OnInit {

    @Input() tabs?: String[];
    @Output() tabClicked = new EventEmitter<number>();

    private activeTab: number = 0;

    constructor() { }

    ngOnInit() { 
        // Default setting for subnav component
        if (!this.tabs)
            this.tabs = ["Overview"] 
    }

    emitActiveTab(activeTabIndex: number) {
        this.activeTab = activeTabIndex;
        this.tabClicked.emit(this.activeTab);
    }

    
}