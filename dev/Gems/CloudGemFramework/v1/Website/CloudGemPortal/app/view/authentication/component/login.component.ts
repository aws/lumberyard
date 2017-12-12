import { Component, OnInit, Input, Output, EventEmitter } from '@angular/core';

@Component({
    selector: 'login-container',
    template: `
        <div (keydown.enter)="submit()" class="login-container">
            <h2> {{ heading }} </h2>
            <ng-content></ng-content>
            <div class="login-submit-container">
                <button type="submit" (click)="submit()" class="btn l-primary" [disabled]="isButtonDisabled">
                    {{ submitButtonText }}
                </button>
            </div>
            <div class="row justify-content-center text-center align-items-center">
                    <div [ngClass]="tertiaryLinkText ? 'col-5' : 'col-12' ">
                        <p><a (click)="secondaryLink()"> {{ secondaryLinkText }} </a></p>
                    </div>
                    <div *ngIf="tertiaryLinkText" class="col-5">
                        <p><a (click)="tertiaryLink()"> {{ tertiaryLinkText }} </a></p>
                    </div>
            </div>
        </div>
    `,
    styleUrls: ['app/view/authentication/component/login.component.css']
})
export class LoginContainerComponent implements OnInit {
    constructor() { }

    @Input() heading: string = "";
    @Input() submitButtonText: string = "Save";
    @Input() secondaryLinkText: string = "";
    @Input() tertiaryLinkText: string = "";
    @Input() isButtonDisabled: boolean = false;
    @Output() formSubmitted = new EventEmitter();
    @Output() secondaryLinkClick = new EventEmitter();
    @Output() tertiaryLinkClick = new EventEmitter();

    ngOnInit() { }

    submit() {
        this.formSubmitted.emit();
    }

    secondaryLink() {
        this.secondaryLinkClick.emit();
    }

    tertiaryLink() {
        this.tertiaryLinkClick.emit();
    }
}
