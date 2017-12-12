import { Component, Input, ViewEncapsulation, OnInit, trigger, ViewChild } from '@angular/core';
import { ActionItem } from "../class/index";

@Component({
    selector: 'action-stub-basic',
    templateUrl: 'app/view/game/module/shared/component/action-stub-basic.html',
    styleUrls: ['app/view/game/module/shared/component/action-stub-basic.css'],
    encapsulation: ViewEncapsulation.None
})
export class ActionStubBasicComponent {
    @Input() delete: Function;
    @Input() hideDelete?: boolean = false;
    @Input() edit: Function;
    @Input() hideEdit?: boolean = false;
    @Input() add: Function;
    @Input() hideAdd?: boolean = false;
    @Input() custom: ActionItem[];
    @Input() click: Function;
    @Input() id: string;
    @Input() model: any;

    ngOnInit() {
    }

    public onChange(index: string, model:any) {
        var num = Number(index)
        if (num >= 0)
            this.custom[num].onClick(model);
    }

    public onClick(model: any) {
        if (this.click)
            this.click(model);
    }
}
