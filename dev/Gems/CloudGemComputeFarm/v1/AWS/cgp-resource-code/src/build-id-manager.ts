import { CloudGemComputeFarmIndexComponent } from "./index.component"
import { CloudGemComputeFarmApi } from "./index"
import { Subject } from "rxjs/Rx";
import { ToastsManager } from 'ng2-toastr';

export class BuildIdManager {
    buildIds: Array<any> = [];
    currentBuildId = new Subject<string>();
    private _loading = false;

    constructor(
        private _indexComponent: CloudGemComputeFarmIndexComponent, 
        private _apiHandler: CloudGemComputeFarmApi,
        private _toastr: ToastsManager
    ) {
    }

    refresh(): void {
        if (this._loading) {
            return;
        }

        this._loading = true;
        this._apiHandler.getBuildIds().subscribe(response => {
            let obj = JSON.parse(response.body.text());
            let items = obj['result'];
            let result = [];

            for (var idx = 0; idx < items.length; idx++) {
                let item = items[idx];

                if (item['id'] == "-") {
                    continue;
                }

                let data = JSON.parse(item['id']);
                result.push(
                    {
                        'name': data['workflowId'],
                        'id': item['id'],
                        'start': this.convertDate(item['start'])
                    }
                );

            }

            this.buildIds = result;
            this._loading = false;
        }, err => {
            this._toastr.error("Failed to load existing build IDs. Retrying in 5 seconds. " + err.message);
            this._loading = false;

            let self = this;
            setTimeout(function() { self.refresh(); }, 5000);
        });
    }

    clearAndReferesh(): void {
        if (this._loading) {
            return;
        }

        this._loading = true;
        this._apiHandler.clear().subscribe(response => {
            this._loading = false;
            this.refresh();
        }, err => {
            this._toastr.error("Failed to clear existing build logs. Retrying in 5 seconds. " + err.message);
            this._loading = false;

            let self = this;
            setTimeout(function() { self.clearAndReferesh(); }, 5000);
        });
    }

    isLoading(): boolean {
        return this._loading;
    }

    private convertDate(s: string) {
        let result = new Date(s + "Z");
        return result.toString();
    }
}
