import { Component, Input, OnInit, ViewEncapsulation, EventEmitter } from '@angular/core';
import { DragulaService } from 'ng2-dragula/ng2-dragula';

@Component({
    selector: 'dragable',
    template: `
        <div [id]="id" class="" [style.width.%]="widthPc" [style.height.px]="heightPx" style="overflow: auto; cursor: move;" [dragula]="bagId">
            <ng-content></ng-content>
        </div>
    `,
    styleUrls: ['node_modules/dragula/dist/dragula.css', './app/view/game/module/shared/component/dragula.css'],
    encapsulation: ViewEncapsulation.None
})
export class DragableComponent {
    @Input() bagId: string;
    @Input() widthPc: string = "100";
    @Input() heightPx: string;
    @Input() drop: Function;
    @Input() over: Function;
    @Input() drag: Function;
    @Input() out: Function;
    @Input() id: string;

    private onDragSubscription: EventEmitter<any>;
    private onDropSubscription: EventEmitter<any>;
    private onOverSubscription: EventEmitter<any>;
    private onOutSubscription: EventEmitter<any>;

    constructor(private dragulaService: DragulaService) {     
    }

    public ngOnInit() {        
        this.onDragSubscription = this.dragulaService.drag.subscribe((value) => {
            this.onDrag(value.slice(1));
        });
        this.onDropSubscription = this.dragulaService.drop.subscribe((value) => {
            this.onDrop(value.slice(1));
        });
        this.onOverSubscription = this.dragulaService.over.subscribe((value) => {
            this.onOver(value.slice(1));
        });
        this.onOutSubscription = this.dragulaService.out.subscribe((value) => {
            this.onOut(value.slice(1));
        });
    }

    ngOnDestroy() {        
        this.onDragSubscription.unsubscribe();
        this.onDropSubscription.unsubscribe();
        this.onOverSubscription.unsubscribe();
        this.onOutSubscription.unsubscribe();
    }

    private hasClass(el: any, name: string) {
        return new RegExp('(?:^|\\s+)' + name + '(?:\\s+|$)').test(el.className);
    }

    private addClass(el: any, name: string) {
        if (!this.hasClass(el, name)) {
            el.className = el.className ? [el.className, name].join(' ') : name;
        }
    }

    private removeClass(el: any, name: string) {
        if (this.hasClass(el, name)) {
            el.className = el.className.replace(new RegExp('(?:^|\\s+)' + name + '(?:\\s+|$)', 'g'), '');
        }
    }

    private onDrag(args) {
        let element = args[0];
        let targetContainer = args[1];
        let sourceContainer = args[2];
        this.removeClass((element && element.children.length) > 0 ? element.children[0] : element, 'ex-moved');
        if (this.drag)
            this.drag(element, targetContainer, sourceContainer)
    }

    private onDrop(args) {
        let element = args[0];
        let targetContainer = args[1];
        let sourceContainer = args[2];
        this.addClass((element && element.children.length) > 0 ? element.children[0] : element, 'ex-moved');
        if (this.drop)
            this.drop(element, targetContainer, sourceContainer)
    }

    private onOver(args) {
        let element = args[0];
        let targetContainer = args[1];        
        let sourceContainer = args[2];        
        this.addClass((element && element.children.length) > 0 ? element.children[0] : element, 'ex-over');
        if(this.over)
            this.over(element, targetContainer, sourceContainer)
    }

    private onOut(args) {
        let element = args[0];
        let targetContainer = args[1];
        let sourceContainer = args[2];
        this.removeClass(targetContainer, 'ex-over');
        if (this.out)
            this.out(element, targetContainer, sourceContainer)
    }
}
