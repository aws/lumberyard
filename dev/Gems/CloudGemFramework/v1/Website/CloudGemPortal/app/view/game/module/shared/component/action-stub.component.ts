import {Component, Input, ViewEncapsulation, OnInit, trigger} from '@angular/core';
import { ActionItem } from "../class/index";

@Component({
    selector: 'action-stub',
    templateUrl: 'app/view/game/module/shared/component/action-stub.component.html',
    styleUrls: ['app/view/game/module/shared/component/action-stub.component.css'],
    encapsulation: ViewEncapsulation.None   
})
export class ActionStubComponent {
    @Input() text: string;
    @Input() subtext: string;
    @Input() secondaryText: string;
    @Input() textTouch: Function;
    @Input() img: string;
    @Input() imgTouch: Function;
    @Input() delete: Function;    
    @Input() edit: Function;
    @Input() add: Function;
    @Input() custom: ActionItem[];
    @Input() click: Function;
    @Input() id: string;
    @Input() model: any;  
    @Input() iconClassNames: string;  
    @Input() statusClassNames: string;  

    private isHovering: boolean;
    private isExpanded: boolean;    

    constructor() {        
        this.isExpanded = false;
    }

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

    protected onTextTouch(model: any) {
        if (this.textTouch)
            this.textTouch(model);
    }

    protected onImgTouch(model: any) {
        if (this.imgTouch)
            this.imgTouch(model);
    }

    protected onHover(model: any) {
        this.isHovering = true;
    }

    protected onLeave(model: any) {
        this.isHovering = false;
    }

    protected onExpand(model: any) {     
        this.isExpanded = !this.isExpanded;
    }
}
