import { Type } from '@angular/core';
import { DashboardComponent } from '../../analytic/component/dashboard/dashboard.component'
import { HeatmapComponent } from '../../analytic/component/heatmap/heatmap.component'
import { AnalyticIndex } from '../../analytic/component/analytic.component';

export class Feature {
    //highest priority is 0, the higher the number the lower priority
    constructor(
        public parent: string,
        public name: string,
        public component: Type<any>,
        public order: number,
        public dependencies: Array<string>,
        public route_path: string,
        public description: string = "",
        public image: string = "",
        public data: any = {}) {
    }
}

export class FeatureDefinitions {
    private _defined = [
        new Feature(undefined, "Analytics", AnalyticIndex, 0, [], "analytics"),
        new Feature("Analytics", "Dashboard", DashboardComponent, 0, ["CloudGemMetric"], "analytics/dashboard", "View live game telemetry","CloudGemMetric_optimized._CB492341798_.png"),
        new Feature("Analytics", "Heatmap", HeatmapComponent, 1, ["CloudGemMetric"], "analytics/heatmap", "View heatmaps using your game data", "Heat_Map_Gem_Portal_Optinized._CB1517862571_.png")
    ]

    get defined() {
        return this._defined
    }
}
