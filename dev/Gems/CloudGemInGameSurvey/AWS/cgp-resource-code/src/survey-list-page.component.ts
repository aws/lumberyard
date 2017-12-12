import { Component, Input, OnInit, ViewChild, ViewContainerRef } from '@angular/core'
import { Http } from '@angular/http';
import { AwsService } from "app/aws/aws.service";
import { ModalComponent } from 'app/shared/component/index';
import { SearchResult } from 'app/view/game/module/shared/component/search.component';
import { ActionItem } from "app/view/game/module/shared/class/index";
import { ToastsManager } from 'ng2-toastr';
import { LyMetricService } from 'app/shared/service/index';
import { FormBuilder, FormGroup, FormArray, Validators } from '@angular/forms';

import {
    InGameSurveyApi,    
    SurveyMetadata,
    Survey,
    SurveyStatus,
    TimeUtil,
    SortOrder,
    ValidationUtil
} from './index';

enum EditMode {
    None,
    CreateSurvey,
    CloneSurvey,
    DeleteSurvey,
    CloseSurvey
}

@Component({
    selector: 'survey-list-page',
    templateUrl: 'node_modules/@cloud-gems/cloudgemingamesurvey/survey-list-page.component.html',
    styleUrls: ['node_modules/@cloud-gems/cloudgemingamesurvey/survey-list-page.component.css']
})

export class InGameSurveySurveyListPageComponent {
    @Input() context: any;
    @Input() toSurveyDetailsPageCallback: Function;

    private EditMode = EditMode;
    private SurveyStatus = SurveyStatus;
    private epochToString = TimeUtil.epochToString;
    private isFormFieldRequiredEmpty = ValidationUtil.isFormFieldRequiredEmpty;
    private isFormFieldNotValid = ValidationUtil.isFormFieldNotValid;
    
    private cloneSurveyAction = new ActionItem("Clone", this.onShowCloneSurveyModal);
    private closeSurveyAction = new ActionItem("End", this.onShowCloseSurveyModal);
    private inactiveSurveyActions: ActionItem[] = [
        this.cloneSurveyAction];
    private activeSurveyActions: ActionItem[] = [
        this.cloneSurveyAction,
        this.closeSurveyAction];

    private curSurveyMetadata: SurveyMetadata;
    private apiHandler: InGameSurveyApi;    
    private editMode: EditMode;
    private isLoading: boolean;    
    private tabIndex: number;
    private surveyMetadataList: SurveyMetadata[];
    private filteredSurveyMetadataList: SurveyMetadata[];
    private closedSurveyMetadataList: SurveyMetadata[];
    private numSurveysPerPage = 10;
    private numFilteredSurveysPages;
    private curFilteredSurveysPageIndex = 0;
    private numClosedSurveysPages;
    private curClosedSurveysPageIndex = 0;
    private filteredSurveySortOrder = SortOrder.DESC;
    private closedSurveySortOrder = SortOrder.DESC;

    private surveyFilter = SurveyStatus.Active | SurveyStatus.Scheduled | SurveyStatus.Draft;
    private showSurveyFilter = false;
    private activeFilterChecked = true;
    private scheduledFilterChecked = true;
    private draftFilterChecked = true;

    private showEditSurveyOptions = false;

    private surveyNameForm: FormGroup;

    @ViewChild(ModalComponent) modalRef: ModalComponent;

    constructor(private fb: FormBuilder, private http: Http, private aws: AwsService, private toastr: ToastsManager, private vcr: ViewContainerRef, private metric: LyMetricService) {
    }

    ngOnInit() {
        this.apiHandler = new InGameSurveyApi(this.context.ServiceUrl, this.http, this.aws, this.metric, this.context.identifier);        
        this.isLoading = false;
        this.tabIndex = 0;

        this.bindMethods();

        this.surveyMetadataList = [];
        this.loadSurveyMetadataList(null, null);
    }    

    private bindMethods(): void {
        this.onShowStatusFilter = this.onShowStatusFilter.bind(this);
        this.cloneSurveyAction.onClick = this.cloneSurveyAction.onClick.bind(this);
        this.closeSurveyAction.onClick = this.closeSurveyAction.onClick.bind(this);
        this.onShowCreateSurveyModal = this.onShowCreateSurveyModal.bind(this);
        this.onSurveyNameSearchUpdated = this.onSurveyNameSearchUpdated.bind(this);
        this.onReverseCurSurveyMetadataListSortOrder = this.onReverseCurSurveyMetadataListSortOrder.bind(this);
        this.onApplyFilterChange = this.onApplyFilterChange.bind(this);
        this.onCancelFilterChange = this.onCancelFilterChange.bind(this);
        this.onPageChanged = this.onPageChanged.bind(this);
        this.onShowSurveyDetailsPage = this.onShowSurveyDetailsPage.bind(this);
        this.onDismissModal = this.onDismissModal.bind(this);
        this.onCreateSurvey = this.onCreateSurvey.bind(this);
        this.onCloseSurvey = this.onCloseSurvey.bind(this);
        this.onCloneSurvey = this.onCloneSurvey.bind(this);
        this.onShowCloneSurveyModal = this.onShowCloneSurveyModal.bind(this);
        this.onShowCloseSurveyModal = this.onShowCloseSurveyModal.bind(this);
        this.onDeleteSurvey = this.onDeleteSurvey.bind(this);        
        this.onShowDeleteSurveyModal = this.onShowDeleteSurveyModal.bind(this);        
    }

    private createSurveyNameForm(): void {
        this.surveyNameForm = this.fb.group({
            'surveyName': [null, Validators.compose([Validators.required])]
        });
    }

    private validateSurveyNameForm(): boolean {
        this.surveyNameForm.controls["surveyName"].markAsTouched();
        return this.surveyNameForm.controls["surveyName"].valid;
    }

    private removeSurveyMetadata(surveyId: string): void {
        this.removeSurveyMetadataFromList(surveyId, this.surveyMetadataList);
        this.removeSurveyMetadataFromList(surveyId, this.filteredSurveyMetadataList);
        this.removeSurveyMetadataFromList(surveyId, this.closedSurveyMetadataList);
    }

    private removeSurveyMetadataFromList(surveyId: string, surveyMetadataList: SurveyMetadata[]): SurveyMetadata {
        let idx = surveyMetadataList.findIndex( (metadata) => {
            return metadata.surveyId == surveyId;
        })

        if (idx != -1) {
            return surveyMetadataList.splice(idx, 1)[0];
        } else {
            return null;
        }
    }

    private onDeleteSurvey(surveyMetadata: SurveyMetadata): void {
        this.apiHandler.deleteSurvey(surveyMetadata.surveyId).subscribe(response => {
            this.removeSurveyMetadata(surveyMetadata.surveyId);
            this.closeModal();
            this.editMode = EditMode.None;
        }, err => {
            this.toastr.error("Failed to delete survey. " + err);
        });
    }

    private onShowStatusFilter(): void {
        this.showSurveyFilter = !this.showSurveyFilter;
    }

    private closeModal(): void {
        this.modalRef.close();
    }

    private onCreateSurvey(): void {
        if (!this.validateSurveyNameForm()) {
            return;
        }

        let surveyName = this.surveyNameForm.value.surveyName;
        this.apiHandler.createSurvey(surveyName).subscribe(response => {
            let obj = JSON.parse(response.body.text());

            let surveyMetadata = new SurveyMetadata();
            surveyMetadata.surveyName = surveyName;
            surveyMetadata.surveyId = obj.result.survey_id;
            surveyMetadata.creationTime = obj.result.creation_time;

            this.closeModal();

            if (this.toSurveyDetailsPageCallback) {
                this.toSurveyDetailsPageCallback(surveyMetadata);
            }
        }, err => {
            this.toastr.error("Failed to create survey, please try again. " + err);
        });
    }

    private onDismissModal(): void {
        this.editMode = EditMode.None;
    }

    private onShowDeleteSurveyModal(surveyMetadata: SurveyMetadata): void {
        this.curSurveyMetadata = surveyMetadata;
        this.editMode = EditMode.DeleteSurvey;
    }

    private onPageChanged(pageIndex): void {
        if (this.tabIndex == 0) {
            this.curFilteredSurveysPageIndex = pageIndex - 1;
        } else if (this.tabIndex == 1) {
            this.curClosedSurveysPageIndex = pageIndex - 1;
        }
    }

    private onShowCloneSurveyModal(surveyMetadata: SurveyMetadata): void {
        this.curSurveyMetadata = surveyMetadata;
        this.editMode = EditMode.CloneSurvey;

        this.createSurveyNameForm();
    }

    private onShowCloseSurveyModal(surveyMetadata: SurveyMetadata): void {
        this.curSurveyMetadata = surveyMetadata;
        this.editMode = EditMode.CloseSurvey;
    }

    private insertSurveyMetadataToList(surveyMetadata: SurveyMetadata, surveyMetadataList: SurveyMetadata[], sortOrder: SortOrder): void {
        if (sortOrder == SortOrder.DESC) {
            let i = 0;
            for (; i < surveyMetadataList.length; i++) {
                if (surveyMetadata.creationTime >= surveyMetadataList[i].creationTime) {
                    break;
                }
            }

            surveyMetadataList.splice(i, 0, surveyMetadata);
        } else {
            let i = surveyMetadataList.length;
            for (; i > 0; i--) {
                if (surveyMetadata.creationTime >= surveyMetadataList[i-1].creationTime) {
                    break;
                }
            }

            surveyMetadataList.splice(i, 0, surveyMetadata);
        }
    }

    private insertSurveyMetadata(surveyMetadata: SurveyMetadata): void {
        this.insertSurveyMetadataToList(surveyMetadata, this.surveyMetadataList, SortOrder.DESC);

        if (surveyMetadata.status == SurveyStatus.Closed) {
            this.insertSurveyMetadataToList(surveyMetadata, this.closedSurveyMetadataList, this.closedSurveySortOrder);
        } else if (surveyMetadata.status & this.surveyFilter) {
            this.insertSurveyMetadataToList(surveyMetadata, this.filteredSurveyMetadataList, this.filteredSurveySortOrder);
        }
    }

    private onCloneSurvey(): void {
        if (!this.validateSurveyNameForm()) {
            return;
        }

        let surveyName = this.surveyNameForm.value.surveyName;
        this.apiHandler.cloneSurvey(surveyName, this.curSurveyMetadata.surveyId).subscribe(response => {
            let obj = JSON.parse(response.body.text());

            let newSureyMetadata = new SurveyMetadata();
            newSureyMetadata.surveyName = surveyName;
            newSureyMetadata.surveyId = obj.result.survey_id;
            newSureyMetadata.creationTime = obj.result.creation_time;

            this.insertSurveyMetadata(newSureyMetadata);
            this.refreshPagination();

            this.closeModal();
            this.editMode = EditMode.None;
        }, err => {
            this.toastr.error("Failed to update activation period. " + err);
            this.closeModal();
            this.editMode = EditMode.None;
        });
    }

    private onCloseSurvey(surveyMetadata: SurveyMetadata): void {
        let surveyEndTime = Math.floor(Date.now() / 1000) - 1;

        this.apiHandler.setSurveyActivationPeriod(surveyMetadata.surveyId,
            surveyMetadata.activationStartTime, surveyEndTime).subscribe(response => {
            this.toastr.success("Survey has been closed");            

            let removedSurveyMetadata = this.removeSurveyMetadataFromList(surveyMetadata.surveyId, this.filteredSurveyMetadataList);
            if (removedSurveyMetadata) {                
                removedSurveyMetadata.activationEndTime = surveyEndTime;
                removedSurveyMetadata.status = SurveyStatus.Closed;

                this.insertSurveyMetadataToList(removedSurveyMetadata, this.closedSurveyMetadataList, this.closedSurveySortOrder);
            }

            this.closeModal();
            this.editMode = EditMode.None;
        }, err => {
            this.toastr.error("Failed to update activation period. " + err);
        });
    }    

    private onShowSurveyDetailsPage(surveyMetadata: SurveyMetadata): void {
        if (this.toSurveyDetailsPageCallback) {
            this.toSurveyDetailsPageCallback(surveyMetadata);
        }
    }

    private onCancelFilterChange(): void {
        this.activeFilterChecked = Boolean(this.surveyFilter & SurveyStatus.Active);
        this.scheduledFilterChecked = Boolean(this.surveyFilter & SurveyStatus.Scheduled);
        this.draftFilterChecked = Boolean(this.surveyFilter & SurveyStatus.Draft);
    }

    private onApplyFilterChange(): void {
        let newFilter = 0;

        if (this.activeFilterChecked) {
            newFilter |= SurveyStatus.Active;
        }

        if (this.scheduledFilterChecked) {
            newFilter |= SurveyStatus.Scheduled;
        }

        if (this.draftFilterChecked) {
            newFilter |= SurveyStatus.Draft;
        }

        if (this.surveyFilter != newFilter) {
            this.surveyFilter = newFilter
            this.filterSurveyMetadataList();
            this.refreshPagination();
        }
    }

    private onReverseCurSurveyMetadataListSortOrder(): void {        
        if (this.tabIndex == 0) {
            this.filteredSurveySortOrder ^= 1;
            this.filteredSurveyMetadataList.reverse();
        } else if (this.tabIndex == 1) {
            this.closedSurveySortOrder ^= 1;
            this.closedSurveyMetadataList.reverse();
        }
    }

    private getCurSurveyMetadataList(): SurveyMetadata[] {
        let curPageIndex = this.tabIndex == 0 ? this.curFilteredSurveysPageIndex : this.curClosedSurveysPageIndex;
        let start = this.numSurveysPerPage * curPageIndex;
        let end = start + this.numSurveysPerPage;

        if (this.tabIndex == 0) {
            return this.filteredSurveyMetadataList.slice(start, end);
        } else if (this.tabIndex == 1) {
            return this.closedSurveyMetadataList.slice(start, end);
        } else {
            return null;
        }
    }

    private refreshPagination(): void {
        let activeSurveyMetadataList = this.tabIndex == 0 ? this.filteredSurveyMetadataList : this.closedSurveyMetadataList;

        let numPages;
        if (!activeSurveyMetadataList || activeSurveyMetadataList.length == 0) {
            numPages = 1;
        } else {
            numPages = Math.floor(activeSurveyMetadataList.length / this.numSurveysPerPage);
            numPages += activeSurveyMetadataList.length % this.numSurveysPerPage > 0 ? 1 : 0;
        }

        if (this.tabIndex == 0) {
            this.numFilteredSurveysPages = numPages;
        } else if (this.tabIndex == 1) {
            this.numClosedSurveysPages = numPages;
        }
    }

    private clearFilteredSurveyMetadata(): void {
        this.filteredSurveyMetadataList = [];
        this.closedSurveyMetadataList = []
    }

    private populateSuveyStatuses(surveyMetadataList: SurveyMetadata[]) {
        let curentEpochTime = Math.floor(Date.now() / 1000);

        for (let surveyMetadata of surveyMetadataList) {
            surveyMetadata.determineSurveyStatus(curentEpochTime);
        }
    }

    private filterSurveyMetadataList(): void {
        this.clearFilteredSurveyMetadata();        

        for (let surveyMetadata of this.surveyMetadataList) {
            this.filterSurveyMetadata(surveyMetadata);
        }
    }

    private filterSurveyMetadata(surveyMetadata: SurveyMetadata): void {
        if (surveyMetadata.status == SurveyStatus.Closed) {
            this.closedSurveyMetadataList.push(surveyMetadata);
            return;
        }

        if (surveyMetadata.status & this.surveyFilter) {
            this.filteredSurveyMetadataList.push(surveyMetadata);
        }
    }

    private loadSurveyMetadataList(surveyName: string, paginationToken: string): void {
        this.isLoading = true;

        this.apiHandler.getSurveyMetadataList(surveyName, paginationToken).subscribe(response => {
            let obj = JSON.parse(response.body.text());

            this.surveyMetadataList = this.surveyMetadataList.concat(
                obj.result.metadata_list.map((surveyMetadata) => {
                    let out = new SurveyMetadata();
                    out.fromBackend(surveyMetadata);
                    return out;
                })
            );

            if (obj.result.pagination_token) {
                this.loadSurveyMetadataList(surveyName, obj.result.pagination_token);
            } else {
                this.populateSuveyStatuses(this.surveyMetadataList);
                this.filterSurveyMetadataList();
                this.refreshPagination();

                this.isLoading = false;
            }            
        }, err => {
            this.toastr.error("The survey list did not refresh properly. " + err);
            this.clearFilteredSurveyMetadata();
            this.refreshPagination();
            this.isLoading = false;
        });
    }

    private onSurveyNameSearchUpdated(searchResult: SearchResult) {
        this.surveyMetadataList = [];
        let surveyName = searchResult.value;        
        this.loadSurveyMetadataList(surveyName, null);
    }

    private onShowCreateSurveyModal(): void {
        this.editMode = EditMode.CreateSurvey;

        this.createSurveyNameForm();
    }

    private onTabIndexUpdated(activeIndex: number): void {
        this.tabIndex = activeIndex;

        if (this.tabIndex == 0) {
            this.curFilteredSurveysPageIndex = 0;
        } else if (this.tabIndex == 1) {
            this.curClosedSurveysPageIndex = 0;
        }

        this.refreshPagination();
    }
}