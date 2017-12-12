import { Component, Input, Output, ViewChild, ViewEncapsulation, EventEmitter, ElementRef } from '@angular/core';
import { NgbModal, ModalDismissReasons, NgbModalRef } from '@ng-bootstrap/ng-bootstrap';
import { LyMetricService } from 'app/shared/service/index'

@Component({
    selector: 'modal',
    template: `
    <ng-template #content let-c="close" let-d="dismiss">
    <div class="modal-header">
        <h2>{{title}}</h2>
        <button type="button" class="close" aria-label="Close" (click)="d('Cross click')">
        <span aria-hidden="true">&times;</span>
        </button>
    </div>
    <ng-content select=".modal-body"></ng-content>
    <div class="modal-footer">
        <!-- inline 5px margin here stops button from moving a bit on modal popup -->
        <button id="modal-dismiss" type="button" class="btn btn-outline-primary" style="margin-right: 5px" (click)="c('Cancel')">{{closeButtonText}}</button>
        <button *ngIf="hasSubmit" id="modal-submit" type="submit" [disabled]="!canSubmit" (click)="onSubmit()" class="btn l-primary">{{submitButtonText}}</button>
    </div>
    </ng-template>
    <span class="modal-trigger-span" (click)="open(content)"><ng-content select=".modal-trigger"></ng-content></span>
    `
})
export class ModalComponent {
    @Input() size: 'sm' | 'lg' = 'lg';
    @Input() title: string;
    @Input() submitButtonText: string = "Save";
    @Input() closeButtonText: string = "Close";
    // Boolean for whether or not the modal should have a submit button
    @Input() hasSubmit: boolean = false
    // Boolean to make the modal open on an *ngIf condition that is attached to the modal selector
    @Input() autoOpen: boolean;
    @Input() width: number = 500;
    // Boolean toggle for the submit button
    @Input() canSubmit: boolean = true;
    // Optional function that is executed when the close button is clicked.
    @Input() onClose: Function;
    // Optional function that is executed when the modal is dismissed
    @Input() onDismiss: Function;
    // Optional string that will be used as the registered cloud gem id for metrics
    @Input() metricIdentifier: string;

    @Output() modalSubmitted = new EventEmitter();


    @ViewChild("content") modalTemplate: any;
    private _modalRef : NgbModalRef;
    private _modalOpen: boolean = false;

    private _formDisabled: boolean = false;

    constructor(private _modalService: NgbModal, private lymetrics: LyMetricService) { }

    ngOnInit() { 
        this.close = this.close.bind(this);
        this.dismiss = this.dismiss.bind(this);
        this.ngAfterViewChecked = this.ngAfterViewChecked.bind(this);
        this.onSubmit = this.onSubmit.bind(this);
        this.open = this.open.bind(this);
        this.lymetrics.recordEvent('ModalOpened', {
            "Title": this.title,
            "Identifier": this.metricIdentifier            
        })
    }

    close(preventFunction?: boolean) {
        this._modalRef.close();
        this._modalOpen = false;
        if (!preventFunction) preventFunction = false;
        if (this.onClose && !preventFunction) this.onClose();
    }
    dismiss(preventFunction?: boolean) {
        this._modalRef.dismiss();
        this._modalOpen = false;
        if (!preventFunction) preventFunction = false;
        if (this.onDismiss && !preventFunction) this.onDismiss();
    }

    ngAfterViewChecked() {
        if (this.autoOpen && !this._modalOpen) {
            this._modalOpen = true;
            window.setTimeout(() => {
                this.open(this.modalTemplate);
            });
        }
    }

    onSubmit() {
        this.modalSubmitted.emit();
    }

    open(content) {
        this._modalRef = this._modalService.open(content, { size: this.size });
        this._modalRef.result.then((reason) => {
            if (reason === "Cancel")
                this.close();
        }, (reason) => {
            this.dismiss();
        })
    }
}
