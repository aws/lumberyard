import { Component } from '@angular/core';
import { SidebarState } from 'app/view/game/component/sidebar.component';
import { trigger, state, style, animate, transition } from '@angular/animations';

@Component({
    selector: 'cc-game',
    templateUrl: 'app/view/game/component/game.component.html',
    styles: [
        `
        .cgp-content-container {
            padding-top: 100px;
            margin-right: 35px;
        }
        .cgp-content-container > h1 {
            margin: 25px 0 15px 0;
        }

        .cgp-content-container.expanded {
            margin-left: 300px;
        }
        .cgp-content-container.collapsed {
            margin-left: 115px;
        }
        `
    ],
     animations: [
        trigger('containerMargin', [
            state('Collapsed', style ({
                'margin-left': '115px'
            })),
            state('Expanded', style ({
                'margin-left': '300px'
            })),
            transition('Collapsed => Expanded', animate('300ms ease-in')),
            transition('Expanded => Collapsed', animate('300ms ease-out'))
        ])
    ]
})
export class GameComponent {
    private sidebarState = "Expanded";

    updateSidebarState(sidebarState: SidebarState) {
        this.sidebarState = sidebarState;
    }
}

