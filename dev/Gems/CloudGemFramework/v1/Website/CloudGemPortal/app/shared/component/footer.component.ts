import { Component, OnInit, Input} from '@angular/core';
import { trigger, state, style, animate, transition } from '@angular/animations';

@Component({
    selector: 'cgp-footer',
    templateUrl: 'app/shared/component/footer.component.html',
    styles: [`
        .footer-container {
            height: 35px;	
            width: 100%;
            text-align: center;
        }
    `
    ],
    animations: [
        trigger('footerMargin', [
            state('Collapsed', style ({
                'margin-left': '20px'
            })),
            state('Expanded', style ({
                'margin-left': '75px'
            })),
            transition('Collapsed => Expanded', animate('300ms ease-in')),
            transition('Expanded => Collapsed', animate('300ms ease-out'))
        ])
    ]
})
export class FooterComponent { 
    @Input() sidebarState: string;
}