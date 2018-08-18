import { ViewChild, Input, Component, ViewContainerRef } from '@angular/core';
import { FormControl } from '@angular/forms';
import { DateTimeUtil } from 'app/shared/class/index';
import { ModalComponent } from 'app/shared/component/index';
import { DynamicContentApi} from './index'
import { Http } from '@angular/http';
import { AwsService } from "app/aws/aws.service";
import { DateTimeRangePickerComponent } from 'app/shared/component/datetime-range-picker.component';
import { AbstractCloudGemIndexComponent } from 'app/view/game/module/cloudgems/class/index';
import { ToastsManager } from 'ng2-toastr';
import { LyMetricService } from 'app/shared/service/index';

export enum DynamicContentMode {
    View = 0,
    Edit = 1,
    Delete = 2
}

export enum DynamicContentStage {
    Private = 0,
    Scheduled = 1,
    Public = 2
}

@Component({
    selector: 'dynamic-content-index',
    templateUrl: "node_modules/@cloud-gems/cloudgemdynamiccontent/index.component.html",
    styleUrls: ["node_modules/@cloud-gems/cloudgemdynamiccontent/index.component.css"]
})
export class DynamicContentIndexComponent extends AbstractCloudGemIndexComponent {
    @Input() context: any;
    private _apiHandler: DynamicContentApi;
    private stages: { displayName: string, state: string, value: number, tree: Array<any> }[];
    private bucket: string;
    private bucketHeight: string;
    private mode: DynamicContentMode;
    private currentDynamicContent: any;
    private transitionStages: { name: string, val: number }[];
    private isLoading: boolean;
    private hasNoContent: boolean;
    private publicStage: { displayName: string, state: string, value: number, tree: Array<any> };
    private bShowDragTutorial: boolean;
    private currentStageDisplayName: string;

    @ViewChild(ModalComponent) modalRef: ModalComponent;

    constructor(private http: Http, private aws: AwsService, private toastr: ToastsManager, vcr: ViewContainerRef, private metric: LyMetricService) {
        super()
        this.toastr.setRootViewContainerRef(vcr);
    }

    ngOnInit() {
        this._apiHandler = new DynamicContentApi(this.context.ServiceUrl, this.http, this.aws, this.metric, this.context.identifier);
        this.bucket = "swimlane";
        this.bucketHeight = "700"
        this.mode = DynamicContentMode.View;
        this.stages = [{ displayName: "Private", state: "PRIVATE", value: DynamicContentStage.Private, tree: [] },
        { displayName: "Scheduled", state: "WINDOW", value: DynamicContentStage.Scheduled, tree: [] },
        { displayName: "Public", state: "PUBLIC", value: DynamicContentStage.Public, tree: [] }];
        this.publicStage = this.stages[2];

        this.transitionStages = [{ name: this.stages[DynamicContentStage.Private].displayName, val: this.stages[DynamicContentStage.Private].value },
        { name: this.stages[DynamicContentStage.Scheduled].displayName, val: this.stages[DynamicContentStage.Scheduled].value },
        { name: this.stages[DynamicContentStage.Public].displayName, val: this.stages[DynamicContentStage.Public].value }];
        this.currentDynamicContent = {};
        this.bShowDragTutorial = true;

        //bind the proper scopes
        this.onClick = this.onClick.bind(this);
        this.onDrop = this.onDrop.bind(this);
        this.onEdit = this.onEdit.bind(this);
        this.onSubmit = this.onSubmit.bind(this);
        this.onDelete = this.onDelete.bind(this);
        this.onDeleteSubmit = this.onDeleteSubmit.bind(this);
        this.validate = this.validate.bind(this);
        this.deleteContent = this.deleteContent.bind(this);
        this.setViewMode = this.setViewMode.bind(this);
        this.updateView = this.updateView.bind(this);
        this.postContent = this.postContent.bind(this);

        this.getContent();
    }

    protected onEdit(model: any): void {
        let currentStage = this.findStage(model);
        let targetStage: number = model.target !== undefined && model.target >= 0 ? model.target : currentStage.value.toString();
        this.currentDynamicContent = model;
        this.currentDynamicContent.stage = targetStage.toString();

        let startDate = model.StagingStart ? new Date(model.StagingStart) : null;
        let endDate = model.StagingEnd ? new Date(model.StagingEnd) : null;

        
        this.currentDynamicContent.start = DateTimeRangePickerComponent.default(startDate);
        let datetime = DateTimeRangePickerComponent.default(endDate)
        if (!model.StagingEnd)
            datetime.time.minute = 1;
        this.currentDynamicContent.end = datetime;

        this.currentDynamicContent.start.isScheduled = DateTimeUtil.isValid(startDate);
        this.currentDynamicContent.end.isScheduled = DateTimeUtil.isValid(endDate);

        this.currentDynamicContent.validation = {
            date: {
                isValid: true,
                message: ""
            }
        }

        for (var i = 0; i < this.stages.length; i++) {
            let stage = this.stages[i];
            if (Number(this.currentDynamicContent.stage) === stage.value) {
                this.currentDynamicContent.stage = stage.displayName;
                break;
            }
        }

        this.currentStageDisplayName = currentStage.displayName;
        this.mode = DynamicContentMode.Edit;
    }

    protected updateView(): void {
        this.setViewMode();
        this.getContent();
    }

    protected dprModelUpdated(model): void {        
        this.currentDynamicContent.start = model.start;
        this.currentDynamicContent.end = model.end;
        this.currentDynamicContent.start.valid = model.start.valid;
        this.currentDynamicContent.end.valid = model.end.valid;        
        this.currentDynamicContent.start.isScheduled = model.hasStart;
        this.currentDynamicContent.end.isScheduled = model.hasEnd;
        this.currentDynamicContent.hasRequiredStartDateTime = (this.currentDynamicContent.stage === 'Scheduled' && !model.hasEnd) ? true : false;
        this.currentDynamicContent.hasRequiredEndDateTime = (this.currentDynamicContent.stage === 'Scheduled' && !model.hasStart) ? true : false;
        this.currentDynamicContent.datetime = {
            valid: model.valid,
            message: model.date.message
        }
    }

    protected validate(model): boolean {
        let isValid: boolean = true;

        let stageIdx: keyof typeof DynamicContentStage = this.currentDynamicContent.stage;        
        if (DynamicContentStage[DynamicContentStage.Public] == this.currentDynamicContent.stage) {
            this.currentDynamicContent.end.isScheduled = false;
            this.currentDynamicContent.start.isScheduled = false;
        } else {
            isValid = isValid && model.datetime.valid;
        }

        return isValid;
    }

    protected onSubmit(model): void {
        if (!this.validate(model))
            return

        var submitStage = "PRIVATE";
        for (let stage of this.stages) {
            if (stage.displayName === this.currentDynamicContent.stage) {
                submitStage = stage.state;
            }
        }

        status = submitStage;

        if (!status) {
            this.toastr.error("An error was encountered with pak: " + JSON.stringify(this.currentDynamicContent), "Error generating 'pak' hierarchy")
            return;
        }

        let obj = <any>{
            FileName: model.FileName,
            StagingStatus: status
        }

        let start = null, end = null;
        if (status !== this.publicStage.state) {
            if (this.currentDynamicContent.start.isScheduled) {
                start = DateTimeUtil.toDate(this.currentDynamicContent.start);
            }
            if (this.currentDynamicContent.end.isScheduled) {
                end = DateTimeUtil.toDate(this.currentDynamicContent.end);
            }
        }

        //if (start !== null)
        obj['StagingStart'] = start;

        //if (end !== null)
        obj['StagingEnd'] = end;

        let paks = [];
        if (model.isMissing === undefined)
            paks.push(obj);
        this.getChildrenToMigrate(paks, model.children, start, end, status);

        let payload = {
            FileList: paks
        }

        this.postContent(payload);
        this.setViewMode();
    }

    private getChildrenToMigrate(paks: any[], branch: any[], start: any, end: any, status: string): void {
        for (var i = 0; i < branch.length; i++) {
            var decendant = branch[i];
            if (decendant.checked && decendant.isMissing === undefined) {
                let obj = {
                    FileName: decendant.FileName,
                    StagingStatus: status
                }
                //if (start !== null)
                obj['StagingStart'] = start;

                //if (end !== null)
                obj['StagingEnd'] = end;

                paks.push(obj)
            }
            this.getChildrenToMigrate(paks, decendant.children, start, end, status);
        }
    }

    private clear() {
        for (var i = 0; i < this.stages.length; i++) {
            let stage = this.stages[i];
            stage.tree = [];
        }
    }
    private onDelete(model): void {
        this.currentDynamicContent = model;
        this.mode = DynamicContentMode.Delete;
    }

    private onDeleteSubmit(model): void {
        let obj = {
            FileName: model.FileName,
        }

        let paks = [];
        paks.push(obj);
        this.getChildrenToDelete(paks, model.children);

        // Issue a DELETE request for each file to be deleted in the hierarchy

        let deletionTracker = <{ count: number, length: number }>{
            count: 0,
            length: paks.length
        }

        for (let pak of paks) {
            this.deleteContent(pak.FileName, deletionTracker);
        }
        this.setViewMode();
    }

    private getChildrenToDelete(paks, branch) {
        for (var i = 0; i < branch.length; i++) {
            var decendant = branch[i];
            let obj = {
                FileName: decendant.FileName,
            }
            paks.push(obj)
            this.getChildrenToDelete(paks, decendant.children);
        }
    }

    private onDrop(element: any, target: any, source: any): void {
        //find the element that we need to trigger the edit action on.
        let modelId = element.firstChild.id;
        let targetContainerId = target.id;
        let sourceContainerId = source.id;
        let targetContainer, sourceContainer;

        //locate the source/target containers
        for (var i = 0; i < this.stages.length; i++) {
            let container = this.stages[i];
            if (targetContainerId == container.state) {
                targetContainer = container;
            }
            if (sourceContainerId == container.state) {
                sourceContainer = container;
            }
        }

        let model = undefined;
        //find the model in the source container
        for (var i = 0; i < sourceContainer.tree.length; i++) {
            if (modelId == sourceContainer.tree[i].FileName) {
                model = sourceContainer.tree[i];
                break;
            }
        }

        model.target = targetContainer.value

        this.bShowDragTutorial = false;
        this.onEdit(model);
    }

    private onClick(model: any): void {
        this.currentDynamicContent = model;
    }

    private remove(arr, value): void {
        var index = arr.indexOf(value);
        if (index !== -1) {
            arr.splice(index, 1);
        }
    }

    private setViewMode() {
        this.mode = DynamicContentMode.View;
    }

    private clearScheduledDates() {
        this.currentDynamicContent.start = {};
        this.currentDynamicContent.end = {}
    }

    private hierarchize(tree): void {
        //ground the roots and identify the decendants
        let decendants = [];
        for (var i = 0; i < tree.length; i++) {
            let decendant = tree[i];
            decendant.children = [];

            if (decendant.StagingStatus != this.publicStage.state) {
                if (Date.parse(decendant.StagingStart) > 0 && Date.parse(decendant.StagingEnd) > 0) {
                    decendant.scheduledDateTime = decendant.StagingStart + " to " + decendant.StagingEnd;
                } else if (Date.parse(decendant.StagingStart) > 0) {
                    decendant.scheduledDateTime = decendant.StagingStart + " - No End"
                } else if (Date.parse(decendant.StagingEnd) > 0) {
                    decendant.scheduledDateTime = "No Start - " + decendant.StagingEnd
                }
            }

            if (!decendant.Parent) {
                this.ground(decendant);
            } else {
                decendants.push(decendant);
            }
        }

        //branch the decendants, must be a two step as some roots may not be grounded before you start branching
        //iterate the branches until all have found parents or X iterations
        let maxiterations = (tree.length * tree.length) + 1;
        let iteration = 1;
        while (decendants.length > 0 && iteration < maxiterations) {
            let decendant = decendants.splice(0, 1)[0];
            decendant.checked = true;

            //assuming the decendant and parent have the same StagingStatus
            let branch = this.findTree(decendant);

            let parent = this.find(branch, decendant.Parent);
            if (parent) {
                //parent found.
                parent.push(decendant);
            } else {
                //no parents found.  Push him to the back of the array to check later.
                decendants.push(decendant);
            }
            iteration++;
        }

        //zombies can occur when you don't move a full hierarchy to another state.
        //place all zombie decendants at the root of their tree
        for (var i = 0; i < decendants.length; i++) {
            let decendant = decendants[i];
            let branch = this.findTree(decendant);
            let parent = this.findChildById(decendant.Parent);
            let parentInBranch = this.findChildObjectById(branch, decendant.Parent);

            if (parentInBranch) {
                parent.statusClassNames = null;
                parent = parentInBranch
            } else if (parent) {
                parent.isMissingParent = undefined
                parent.subText = "This is the parent package for '" + decendant.FileName + "'."
                parent.statusClassNames = "fa fa-warning warning-overlay-color"
            }

            if (parent !== null && parent !== undefined) {
                if (parent.StagingStatus != decendant.StagingStatus && parent.isMissingParent === undefined) {
                    var duplicatedMissingParent = this.findChildObjectById(this.findTree(decendant), parent.FileName);
                    var missingParent = Object.assign({}, duplicatedMissingParent !== null ? duplicatedMissingParent : parent)
                    parent.className = "located-parent"
                    missingParent.children = []
                    missingParent.isMissing = true
                    missingParent.isMissingParent = true
                    missingParent.secondaryText = "(Missing)"
                    missingParent.className = "missing"
                    missingParent.subText = "Fix: Drag '" + parent.FileName + "' to this pak or this pak to the '" + parent.FileName + "'."
                    missingParent.children.push(decendant);
                    branch.push(missingParent);
                } else {
                    parent.children.push(decendant)
                }
                decendant.secondaryText = "(Hidden)"
                decendant.subText = "Hidden due to missing '" + parent.FileName + "' in this swimlane."
                decendant.className = "missing-decendant"
                parent.isMissingParent = true
            } else {
                branch.push(decendant);
            }
        }
    }

    private ground(root: any): void {
        if (!root.StagingStatus) {
            this.toastr.error("We did not find a 'StagingStatus' attribute on pak '" + root.FileName + "'", "Error generating 'pak' hierarchy")
            return;
        }

        let status = root.StagingStatus.toUpperCase();
        let bFound = false;
        for (var i = 0; i < this.stages.length; i++) {
            let stage = this.stages[i];
            if (status == stage.state) {
                stage.tree.push(root);
                bFound = true;
                break;
            }
        }
        if (!bFound)
            this.toastr.error("We did not find a known 'StagingStatus' attribute on pak '" + root.FileName + "'", "Error generating 'pak' hierarchy")

    }


    private find(branch, id): any[] {
        let result = null;
        for (var i = 0; i < branch.length; i++) {
            let decendant = branch[i];

            if (decendant.FileName == id) {
                if (!decendant.children)
                    decendant.children = []
                result = decendant.children;
                break;
            }

            result = this.find(decendant.children, id);
            if (result)
                break;
        }
        return result;
    }

    private findChildById(id): any {
        for (var i = 0; i < this.stages.length; i++) {
            let branch = this.stages[i];
            for (var x = 0; x < branch.tree.length; x++) {
                let decendant = branch.tree[x];
                if (decendant.FileName == id) {
                    return decendant;
                }

                var obj = this.findChildObjectById(decendant.children, id)
                if (obj !== null)
                    return obj;
            }
        }
        return null;
    }

    private findChildObjectById(branch, id): any {
        let result = null;
        for (var i = 0; i < branch.length; i++) {
            let decendant = branch[i];

            if (decendant.FileName == id) {
                result = decendant;
                break;
            }

            result = this.findChildObjectById(decendant.children, id);
            if (result)
                break;
        }
        return result;
    }

    private findTree(entry): any[] {
        let branch = [];
        let status = entry.StagingStatus.toUpperCase();
        for (var i = 0; i < this.stages.length; i++) {
            let stage = this.stages[i];
            if (status == stage.state) {
                branch = stage.tree
                break;
            }
        }
        return branch;
    }

    private findStage(entry): { displayName: string, state: string, value: number, tree: any[] } {
        let branch = null;
        let status = entry.StagingStatus.toUpperCase();
        for (var i = 0; i < this.stages.length; i++) {
            let stage = this.stages[i];
            if (status == stage.state) {
                return stage;
            }
        }
        return null;
    }

    //REST commands
    private postContent(model: any) {
        this.modalRef.close(true);
        this._apiHandler.post(model).subscribe(response => {
            this.toastr.success("The package " + (model.FileList.length > 0 ? "'" + model.FileList[0].FileName + "'" : "") + " was successfully updated.")
            this.updateView();
        }, (err) => {
            this.toastr.error("The update of the dynamic content package " + (model.FileList.length > 0 ? "'" + model.FileList[0].FileName + "'" : "") + " failed: " + err.message);
        });
    }


    private getContent() {
        this.clear();
        this.isLoading = true;
        this.hasNoContent = false;
        this._apiHandler.getContent().subscribe(response => {
            let obj = JSON.parse(response.body.text());
            let content: any = obj.result.PortalFileInfoList;

            if (content.length == 0) {
                this.hasNoContent = true;
            }
            //create the hierarchy
            this.hierarchize(content);
            this.isLoading = false;
        }, (err) => {
            this.toastr.error("The listing of dynamic content packages failed. " + err.message);
        });
    }

    private _deletionTracker: {
        length: number,
        count: number
    }

    private isDeletionComplete(deletionTracker?: {
        length: number,
        count: number
    }): boolean {
        if (deletionTracker === null) {
            return true;
        } else {
            deletionTracker.count++;
            if (deletionTracker.count == (deletionTracker.length)) {
                return true;
            }
        }
        return false;
    }

    private deleteContent(model: any, deletionTracker: {
        length: number,
        count: number
    } = null) {
        this.isLoading = true;
        this.modalRef.close(true);
        this._apiHandler.delete(model).subscribe(response => {
            this.toastr.success("The package '" + model + "' was successfully deleted.")
            this.getContent();
        }, (err) => {
            this.toastr.error("The deletion of package '" + model + "' failed. " + err.message);
        })
    }
}
