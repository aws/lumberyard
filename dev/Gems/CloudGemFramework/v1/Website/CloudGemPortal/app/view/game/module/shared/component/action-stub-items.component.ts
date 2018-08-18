import { Component, Input, ViewEncapsulation, OnInit, trigger, ViewChild } from '@angular/core';
import { ActionItem } from '../class/index';

@Component({
    selector: 'action-stub-items',
    template: `<div class="actions">
        <div class="action-stub-btn">
            <i *ngIf="delete && !hideDelete" class="fa fa-trash-o" (click)="delete(model)" data-toggle="tooltip" data-placement="top" title="Delete"> </i>
            <i *ngIf="edit && !hideEdit" class="fa fa-cog" (click)="edit(model)" data-toggle="tooltip" data-placement="top" title="Edit"> </i>
            <i *ngIf="add && !hideAdd" class="fa fa-plus" (click)="add(model)" data-toggle="tooltip" data-placement="top" title="Add"> </i>
        </div>
        <div *ngIf="custom.length > 0" ngbDropdown>
            <i class="action-stub-btn" id="action-dropdown" ngbDropdownToggle></i>
            <div class="dropdown-menu dropdown-menu-right" aria-labelledby="action-dropdown">
                <button *ngFor="let action of custom; let i = index" (click)="handle(action, model)" type="button" class="dropdown-item action-stub-btn"> {{action.name}} </button>
            </div>
        </div>
    </div>`,
    styleUrls: ['app/view/game/module/shared/component/action-stub-basic.css'],
    encapsulation: ViewEncapsulation.None
})
export class ActionStubItemsComponent {
    @Input() delete: Function;
    @Input() hideDelete?: boolean = false;
    @Input() edit: Function;
    @Input() hideEdit?: boolean = false;
    @Input() add: Function;
    @Input() hideAdd?: boolean = false;
    @Input() custom: ActionItem[];
    @Input() model: any;

    ngOnInit() {
        if (!this.custom)
            this.custom = []
    }

    public handle(action: ActionItem, model: any) {
        action.onClick(model);
    }
}
