import { Component, Input, Output, ViewEncapsulation, EventEmitter } from '@angular/core';

@Component({
    selector: 'switch-button',
    template: `
        <label class="switch">
            <input type="checkbox" [(ngModel)]="enabled" (ngModelChange)="switchStatusChange()">
            <span class="slider round"></span>
        </label>
    `,
    styleUrls: ['./app/shared/component/switch-button.component.css']
})
export class SwitchButtonComponent {
    @Input() enabled: boolean;
    @Output() enabledChange = new EventEmitter<any>();
    @Output() switch = new EventEmitter<any>();

    public switchStatusChange(): void {
        this.enabledChange.emit(this.enabled);
        this.switch.emit();
    }
}
