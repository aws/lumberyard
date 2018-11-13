import { Type } from '@angular/core';
import { DashboardComponent } from '../../analytic/component/dashboard/dashboard.component'
import { HeatmapComponent } from '../../analytic/component/heatmap/heatmap.component'
import { JiraCredentialsComponent } from '../../admin/component/jira/jira-credentials.component'
import { AnalyticIndex } from '../../analytic/component/analytic.component'
import { AdminComponent } from '../../admin/component/admin.component'
import { UserAdminComponent } from '../../admin/component/user-admin/user-admin.component'
import { ProjectLogComponent } from '../../admin/component/log.component';

export class Feature {
    //highest priority is 0, the higher the number the lower priority
    constructor(
        public parent: string,
        public name: string,
        public component: Type<any>,
        public order: number,
        public dependencies: Array<string>,
        public route_path: string,
        public location: Location,
        public description: string = "",
        public image: string = "",
        public data: any = {}) {
    }
}

export enum Location{
    Top,
    Bottom
}


export class FeatureDefinitions {
    private _defined = [
        new Feature(undefined, "Analytics", AnalyticIndex, 0, [], "analytics", Location.Top),
        new Feature("Analytics", "Dashboard", DashboardComponent, 0, ["CloudGemMetric"], "analytics/dashboard", Location.Top, "View live game telemetry","CloudGemMetric_optimized._CB492341798_.png"),
        new Feature("Analytics", "Heatmap", HeatmapComponent, 1, ["CloudGemMetric"], "analytics/heatmap", Location.Top, "View heatmaps using your game data", "Heat_Map_Gem_Portal_Optinized._CB1517862571_.png"),

        new Feature(undefined, "Administration", AdminComponent, 0, [], "administration", Location.Bottom),
        new Feature("Administration", "User Administration", UserAdminComponent, 0, [], "admin/users", Location.Bottom, "Manage who has access to the Cloud Gem Portal", "user._CB536715123_.png"),
        new Feature("Administration", "Project Logs", ProjectLogComponent, 1, [], "admin/logs", Location.Bottom, "Project Stack Logs", "setting._CB536715120_.png"),
        new Feature("Administration", "Jira Credentials", JiraCredentialsComponent, 2, ["CloudGemDefectReporter"], "admin/jira", Location.Bottom, "View heatmaps using your game data", "Heat_Map_Gem_Portal_Optinized._CB1517862571_.png")
    ]

    get defined() {
        return this._defined;
    }
}
