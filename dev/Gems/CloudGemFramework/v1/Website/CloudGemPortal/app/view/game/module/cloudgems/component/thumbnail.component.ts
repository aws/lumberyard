import { Input, Component, trigger, state, style, transition, animate } from '@angular/core';
import { Measurable, Stateable, Cost } from "app/view/game/module/cloudgems/class/index";


/**
  * MediumTackable component for gems
  * Contains overview of gem and defines required information for custom gems
**/
@Component({
    selector: 'thumbnail-gem',
    templateUrl: 'app/view/game/module/cloudgems/component/thumbnail.component.html',
    styleUrls: ['app/view/game/module/cloudgems/component/thumbnail.component.css']
})

export class MediumTackableGem {
    @Input() title: string = "Unknown";
    @Input() cost: Cost = "Low";
    @Input() srcIcon: string = "None";
    @Input() metric: Measurable = <Measurable>{
        name: "Unknown",
        value: "Loading..."
    };
    @Input() state: Stateable = <Stateable>{
        label: "Loading",
        styleType: "Loading"
    };
}
