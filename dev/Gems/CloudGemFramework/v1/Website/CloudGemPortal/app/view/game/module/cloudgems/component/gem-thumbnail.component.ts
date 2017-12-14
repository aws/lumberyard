import { Input, Component } from '@angular/core';
import { Measurable, Stateable, Cost } from "app/view/game/module/cloudgems/class/index";
import { ThumbnailComponent } from 'app/view/game/module/shared/component/thumbnail.component';

/**
  * Gem Thumbnail Component
  * Contains overview of gem and defines required information for custom gems
**/
@Component({
    selector: 'thumbnail-gem',
    templateUrl: 'app/view/game/module/cloudgems/component/gem-thumbnail.component.html',
    styleUrls: ['app/view/game/module/cloudgems/component/gem-thumbnail.component.css']
})

export class GemThumbnailComponent extends ThumbnailComponent {
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

    ngOnInit() {
    }
}
