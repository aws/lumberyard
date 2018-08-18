import { Http } from '@angular/http';
import { ViewChild, ElementRef, Input, Component, ViewContainerRef } from '@angular/core';
import { ModalComponent } from 'app/shared/component/index';
import { AwsService } from "app/aws/aws.service";
import { SpeechEntry, CharacterEntry, GeneratedPackageEntry, TextToSpeechApi} from './index'
import { Subscription } from 'rxjs/Subscription';
import { AbstractCloudGemIndexComponent } from 'app/view/game/module/cloudgems/class/index';
import { ToastsManager } from 'ng2-toastr/ng2-toastr';
import { PaginationComponent } from 'app/view/game/module/shared/component/index';
import { DefinitionService, LyMetricService } from 'app/shared/service/index';

declare const System: any;
declare const moment: any;

export enum Mode {
    Delete,
    Import,
    List,
    Download,
    Edit
}

type DownloadJobStatus = "Success" | "Pending" | "Failure";
type EntryStatus = "Create" | "Update" | "Reset";

@Component({
    selector: 'text-to-speech-index',
    templateUrl: 'node_modules/@cloud-gems/cloudgemtexttospeech/index.component.html',
    styleUrls: ['node_modules/@cloud-gems/cloudgemtexttospeech/index.component.css']
})

export class TextToSpeechIndexComponent extends AbstractCloudGemIndexComponent{
    @Input() context: any;
    private subNavActiveIndex = 0;
    private _apiHandler: TextToSpeechApi;

    // define types
    private mode: Mode;
    private Modes: any;
    private speechLibrary: SpeechEntry[];
    private speechLibOnCurrentPage: SpeechEntry[];
    private importableSpeeches: SpeechEntry[];
    private characters: CharacterEntry[];
    private charactersOnCurrentPage: CharacterEntry[];
    private generatedPackages: GeneratedPackageEntry[];
    private generatedPackagesOnCurrentPage: GeneratedPackageEntry[];

    private isLoadingSpeechLibrary: boolean;
    private isLoadingCharacters: boolean;
    private isLoadingGeneratedPackages: boolean;
    private loadedSettings: string[];
    private isLoadingSettings: boolean;

    private selectedSpeechesNum: number;
    private selectedPackagesNum: number;
    private selectedSpeechIDs: string[];

    private currentSpeech: SpeechEntry;
    private speechBeforeChange: SpeechEntry;
    private currentCharacter: CharacterEntry;
    private characterBeforeChange: CharacterEntry;
    private currentGeneratedPackage: GeneratedPackageEntry;

    private tagFilterOpen: boolean;
    private tagFilterApplied: boolean;
    private tagFilterText: string;
    private filterTagList: Object[];
    private originalfilterTagList: Object[];
    private tagsFilterLogic: string;

    private characterFilterOpen: boolean;
    private characterFilterApplied: boolean;
    private characterFilterText: string;
    private filterCharacterList: Object[];
    private originalfilterCharacterList: Object[];

    private characterNames: string[];
    private languages: string[];
    private voices: Object;

    private speechMarksTip: string;
    private runtimeCapabilitiesTip: string;
    private downloadDescriptionText: string;

    private audioContext: AudioContext;

    private zipFileName: string;

    private runtimeCapabilitiesEnabled: boolean;
    private runtimeCacheEnabled: boolean;
    
    private importableSpeechesData: any;
    private currentCustomMappings: Object;
    private customFields: string[];
    private customMappingsLoaded: boolean;

    private pendingJobs: Object[];

    private sortDir: string;
    private highlightNewRow: boolean;

    private pageSize: number = 10;
    private Math: any;
    private speechLinePages: number;
    private currentSpeechLibIndex: number = 1;
    private currentCharactersIndex: number = 1;
    private currentGeneratedPackagesIndex: number = 1;
    private characterPages: number;
    private generatedPackagesPages: number;

    private autoSelectDisabled: boolean;
    private papa: any;

    private prosody: Object;
    private ssmlProsodyTagTypes: string[];
    private currentssmlProsodyTagMappings: Object;

    private deleteMultipleSpeeches: boolean;

    @ViewChild(ModalComponent) modalRef: ModalComponent;
    @ViewChild(PaginationComponent) paginationRef: PaginationComponent;

    @ViewChild('import') importButtonRef: ElementRef;
    @ViewChild('selectAllSpeeches') selectAllCheckBoxRef: ElementRef;
    @ViewChild('selectAllGeneratedPackages') selectAlleneratedPackagesRef: ElementRef;
    @ViewChild('speechLibTabArea') speechLibTabAreaRef: ElementRef;
    @ViewChild('charactersTabArea') charactersTabAreaRef: ElementRef;
    @ViewChild('play') playRef: ElementRef;

    constructor(private http: Http, private aws: AwsService, private toastr: ToastsManager, vcr: ViewContainerRef, private metric: LyMetricService) {
        super()
        this.toastr.setRootViewContainerRef(vcr);
    }

    // initialize values
    ngOnInit() {
        this._apiHandler = new TextToSpeechApi(this.context.ServiceUrl, this.http, this.aws, this.metric, this.context.identifier);

        System.import('https://m.media-amazon.com/images/G/01/cloudcanvas/papaparse/papaparse.min._V518065675_.js').then((lib) => {
            this.papa = lib;
        })

        // bind proper scopes for callbacks that have a this reference
        this.deleteModal = this.deleteModal.bind(this);
        this.deleteMultipleSpeechesModal = this.deleteMultipleSpeechesModal.bind(this);
        this.dismissModal = this.dismissModal.bind(this);
        this.downloadModal = this.downloadModal.bind(this);
        this.editModal = this.editModal.bind(this);
        // end of bind scopes

        this.characters = [];
        this.tagFilterText = "";
        this.tagsFilterLogic = "Or";
        this.characterFilterText = "";
        this.speechMarksTip = "Speech Marks are metadata files that describe synthesized speech which can be used for lip-syncing.";
        this.runtimeCapabilitiesTip = "If you disable the runtime capabilities, you will not be able to use a game client to generate speech files.";
        this.downloadDescriptionText = "After clicking the “Download” button, the .zip file may take a few minutes to generate on the server. The file will begin downloading automatically once it is ready.  View and re-download previously generated .zip files on the “Generated Packages” tab."
        this.audioContext = new AudioContext();
        this.pendingJobs = [];
        this.prosody = {
            volume: ["default", "silent", "x-soft", "soft", "medium", "loud", "x-loud"],
            rate: ["default", "x-slow", "slow", "medium", "fast", "x-fast"],
            pitch: ["default", "x-low", "low", "medium", "high", "x-high"]
        };
        this.ssmlProsodyTagTypes = ["volume", "rate", "pitch"];

        this.Modes = Mode;
        this.update();
    }

    public clickTabArea($event): void {
        let targetElement = $event.target;
        var tabArea;
        if (this.subNavActiveIndex == 0) {
            tabArea = this.speechLibTabAreaRef.nativeElement;
        }
        if (this.subNavActiveIndex == 1) {
            tabArea = this.charactersTabAreaRef.nativeElement;
        }

        if (targetElement == tabArea) {
            this.saveCurrentEditing();
        }
    }

    public switchRuntimeCapabilitiesStatus(): void {
        let body = {
            enabled: this.runtimeCapabilitiesEnabled
        }
        this._apiHandler.enableRuntimeCapabilities(body).subscribe(response => {
        }, err => {
            this.toastr.error("Runtime capabilities status was not switched correctly. " + err.message);
        });
    }

    public switchRuntimeCacheStatus(): void {
        let body = {
            enabled: this.runtimeCacheEnabled
        }
        this._apiHandler.enableRuntimeCache(body).subscribe(response => {
        }, err => {
            this.toastr.error("Runtime cache status was not switched correctly. " + err.message);
        });
    }

    public openTagFilter(): void {
        this.originalfilterTagList = JSON.parse(JSON.stringify(this.filterTagList));
        this.tagFilterOpen = !this.tagFilterOpen;
    }

    public openCharacterFilter(): void {
        this.originalfilterCharacterList = JSON.parse(JSON.stringify(this.filterCharacterList));
        this.characterFilterOpen = !this.characterFilterOpen;
    }

    public clearPendingJobsList(): void {
        this.pendingJobs = [];
    }

    public applyFilter(): void {
        let selectedTags = [];
        this.tagFilterText = "";
        for (let tag of this.filterTagList) {
            if (tag["isSelected"]) {
                selectedTags.push(tag["name"]);
            }
        }
        this.tagFilterApplied = selectedTags.length > 0;

        let selectedCharacters = [];
        for (let character of this.filterCharacterList) {
            if (character["isSelected"]) {
                selectedCharacters.push(character["name"]);
            }
        }
        this.characterFilterApplied = selectedCharacters.length > 0;

        let body = {
            tags: selectedTags,
            characters: selectedCharacters,
            logic: this.tagsFilterLogic
        }
        this.isLoadingSpeechLibrary = true;
        this.sortDir = "asc";
        this.originalfilterTagList = JSON.parse(JSON.stringify(this.filterTagList));
        this._apiHandler.filter(body).subscribe(response => {
            let obj = JSON.parse(response.body.text());
            this.speechLibrary = obj.result.entries;
            this.sort(0, this.speechLibrary.length - 1, this.speechLibrary, "character", "asc");
            this.filterTagList = [];
            this.updatePaginationInfo();
            this.updatePageContent(1);
            this.isLoadingSpeechLibrary = false;
            this.initializeSpeeches();
        }, err => {
            this.toastr.error("The filter did not work properly. " + err.message);
            this.isLoadingCharacters = false;
        });

        this.tagFilterOpen = false;
        this.characterFilterOpen = false;
    }

    public cancelTagFilter(): void {
        this.filterTagList = JSON.parse(JSON.stringify(this.originalfilterTagList));
        this.tagFilterOpen = !this.tagFilterOpen;
    }

    public cancelCharacterFilter(): void {
        this.filterCharacterList = JSON.parse(JSON.stringify(this.originalfilterCharacterList));
        this.characterFilterOpen = !this.characterFilterOpen;
    }

    public update(): void {
        if (this.subNavActiveIndex == 0) {
            this.updateSpeechLibraryTab();
        }
        if (this.subNavActiveIndex == 1) {
            this.updateCharacterTab();
        }
        if (this.subNavActiveIndex == 2) {
            this.updateGeneratedpackagesTab();
        }
        
        if (this.subNavActiveIndex == 3) {
            this.updateSettingsTab();
        }
    }

    //Create a new speech or character entry when the "New" button is clicked
    public addNewRow(): void {
        if (this.subNavActiveIndex == 0) {
            this.sortDir = "";
            this.createNewSpeech();
        }
        else if (this.subNavActiveIndex == 1) {
            this.sortDir = "";
            this.createNewCharacter();
        }
    }

    /* Modal functions for TextToSpeech*/
    public deleteModal(model): void {
        this.deleteMultipleSpeeches = false;
        this.mode = Mode.Delete;
        if (this.subNavActiveIndex == 0) {
            this.currentSpeech = model;
        }
        else if (this.subNavActiveIndex == 1) {
            this.currentCharacter = model;
        }
        else if (this.subNavActiveIndex == 2) {
            this.currentGeneratedPackage= model;
        }
    }

    public deleteMultipleSpeechesModal(): void {
        this.deleteMultipleSpeeches = true;
        this.mode = Mode.Delete;
    }

    public dismissModal(): void {
        this.mode = Mode.List;
    }

    public downloadModal(): void {
        if (this.subNavActiveIndex == 0) {
            this.mode = Mode.Download;
        }
        else if (this.subNavActiveIndex == 2) {
            this.downloadGeneratedPackages();
        }
    }

    public editModal(model): void {
        this.mode = Mode.Edit;
        this.currentCharacter = JSON.parse(JSON.stringify(model));
        this.characterBeforeChange = this.currentCharacter;
    }

    public getSubNavItem(subNavItemIndex: number): void {
        this.subNavActiveIndex = subNavItemIndex;
        this.update();
    }

    /* form function */
    public delete(): void {
        this.modalRef.close();
        this.mode = Mode.List;
        if (this.subNavActiveIndex == 0) {
            this.deleteSpeechLine(this.currentSpeech);
        }
        else if (this.subNavActiveIndex == 1) {
            this.deleteCurrentCharacter();
        }
        else if (this.subNavActiveIndex == 2) {
            this.deleteGeneratedPackage(this.currentGeneratedPackage);
        }
    }

    public deleteSelectedSpeeches(): void {
        this.modalRef.close();
        this.mode = Mode.List;
        for (let speech of this.speechLibrary) {
            if (speech.isSelected["list"]) {
                this.deleteSpeechLine(speech);
            }
        }
    }

    public downloadSpeeches(): void {
        this.modalRef.close();
        this.mode = Mode.List;
        let speechlib = []
        for (let speech of this.speechLibrary) {
            if (speech.isSelected["list"]) {
                speechlib.push(
                    {
                        character: speech.character,
                        line: speech.line,
                        tags: speech.tags
                    }
                )
            }
        }

        let body = {
            entries: speechlib,
            name: this.zipFileName ? this.zipFileName : "export"
        }

        for (let pendingJob of this.pendingJobs) {
            if (pendingJob["name"] == body.name && pendingJob["status"] == "Pending") {
                this.toastr.error("The zip file was not downloaded correctly. Another file with the same name '" + body.name + "' is being generated at the moment.");
                this.pendingJobs.push({ name: body.name, key:"", status: "Failure" });
                return;
            }
        }

        this._apiHandler.generateZip(body).subscribe(response => {
            let obj = JSON.parse(response.body.text());
            if (obj.result.key === "") {
                this.toastr.error("The zip file was not downloaded correctly. Cannot generate the zip file.");
            }
            else {
                this.pendingJobs.push({ name: body.name, key: obj.result.key, status: "Pending" });
                this.unselectSpeechLines();
                this.getPackageUrl(obj.result.key, false);
            }
        }, (err) => {
            this.toastr.error("The zip file was not downloaded correctly." + err.message);
            this.setPendingJobStatus(name, "Failure");
        });

        this.zipFileName = "";
    }

    public getImportableSpeeches($event): void {
        let inputValue = $event.target;
        let file = inputValue.files[0];
        inputValue.value = "";

        this.papa.parse(file, {
            preview: 1,
            complete: function (results) {
                let header = results.data[0];
                this.customFields = [];
                let MD5ColumnExists = 0;
                for (let field of header) {
                    if (field != 'MD5') {
                        if (this.customFields.indexOf(field, 0) == -1) {
                            this.customFields.push(field);
                        }
                        else {
                            this.toastr.error("Duplicate field names are not allowed in the header row of the .csv file.");
                            break;
                        }
                    }
                    else {
                        MD5ColumnExists = 1;
                    }
                }
                if (this.customFields.length == header.length - MD5ColumnExists) {
                    this.parseCsvFile(file);
                }
            }.bind(this)
        })
    }

    public listImportableSpeeches() {
        let body = { "mappings": this.currentCustomMappings };
        this._apiHandler.saveCustomMappings(body).subscribe(response => {
            this.toastr.success("The custom mappings were saved");
        },(err) => {
            this.toastr.error("The custom mappings were not saved correctly." + err.message);
        });

        this.customMappingsLoaded = true;
        if (!this.ImportableSpeechesExist(this.importableSpeechesData, this.customFields)) {
            this.toastr.success("No new speeches were imported.");
        }
    }

    public preview(model, modelType): void {
        let body = {};
        if (modelType == "speech") {
            body = {
                character: model.character,
                line: model.line
            }
        }
        else if (modelType == "character") {
            body = {
                character: model.name,
                line: "Hi! My name is " + model.name + "."
            }
        }
        model.previewLabelText = "Waiting...";
        this._apiHandler.preview(body).subscribe(response => {
            let obj = JSON.parse(response.body.text());
            if (obj.result.audioUrl != "" && obj.result.speechMarksUrl != "" && modelType == "speech") {
                let audioUrl = obj.result.audioUrl;
                let speechMarksUrl =obj.result.speechMarksUrl;
                model.previewLabelText = "Preview";
                this.playRef.nativeElement.href = "https://m.media-amazon.com/images/I/91XDP5dWVvL.html?&audiourl=" + encodeURIComponent(audioUrl) + "&visemesurl=" + encodeURIComponent(speechMarksUrl);
                this.playRef.nativeElement.click();
            }
            else if ((obj.result.audioUrl != "" && obj.result.speechMarksUrl == "") || modelType == "character") {
                var request = new XMLHttpRequest();
                request.open('GET', obj.result.audioUrl, true);
                request.responseType = 'arraybuffer';
                request.onload = function () {
                    model.previewLabelText = "Preview";
                    this.playAudio(request.response);
                }.bind(this);
                request.send();
            }
        }, (err) => {
            this.toastr.error("The spoken line was not generated correctly." + err.message);
            model.previewLabelText = "Preview";
        });
    }

    //Interation with the HTML elements
    public clickImportButton(): void {
        this.importButtonRef.nativeElement.click();
    }

    public selectAll($event, entries, mode?): void {
        let selectAll = $event.target.checked;
        if (this.subNavActiveIndex == 0) {
            for (let speech of entries) {
                if (speech.isSelected[mode] != selectAll) {
                    if (mode == "list") {
                        this.updateSelectedSpeechesNum(speech);
                    }
                    speech.isSelected[mode] = selectAll;
                }
            }
            if (this.selectedSpeechesNum != entries.length) {
                this.selectAllCheckBoxRef.nativeElement.checked = false;
            }
        }
        else if (this.subNavActiveIndex == 2) {
            for (let generatedPackage of entries) {
                if (generatedPackage.isSelected != selectAll) {
                    this.updateSelectedPackgesNum(generatedPackage);
                    generatedPackage.isSelected = selectAll;
                }
            }
            if (this.selectedPackagesNum != entries.length) {
                this.selectAlleneratedPackagesRef.nativeElement.checked = false;
            }
        }
    }

    public onEditableFieldsClicked(model): void {
        if (!model.isEditing) {
            this.saveCurrentEditing();
            model.isEditing = true;

            if (this.subNavActiveIndex == 1) {
                this.characterBeforeChange = JSON.parse(JSON.stringify(model));
                this.currentCharacter = model;
            }
            else {
                this.speechBeforeChange = JSON.parse(JSON.stringify(model));
                this.currentSpeech = model;
            }
        }
    }

    public onLanguageChanged(character): void {
        this.onEditableFieldsClicked(character);
        character.voiceList = this.voices[character.language];
        character.voice = character.voiceList[0].voiceId;
    }

    public onSpeechMarksChanged(character, speechMarkName): void {
        if (!character.isEditing) {
            this.saveCurrentEditing();
            character.isEditing = true;
            this.characterBeforeChange = JSON.parse(JSON.stringify(character));
            this.currentCharacter = character;
            for (let speechMark of this.characterBeforeChange.speechMarksStatus) {
                if (speechMark["name"] == speechMarkName) {
                    speechMark["isChecked"] = !speechMark["isChecked"];
                    break;
                }
            }
        }
    }

    public save(model): void {
        model.isEditing = false;
        this.autoSelectDisabled = true;
        if (this.subNavActiveIndex == 0) {
            this.updateSpeechLine(model);
        }
        else {
            this.updateCharacter(model);
        }
    }

    public cancel(model): void {
        if (!model.isUploaded) {
            if (this.subNavActiveIndex == 0) {
                this.removeFromArray(this.speechLibrary, model);
            }
            else if (this.subNavActiveIndex == 1) {
                this.removeFromArray(this.characters, model);
                this.removeFromArray(this.characterNames, model.name);
            }
            this.updatePaginationInfo();
            this.paginationRef.reset();
            return;
        }

        let originalModel = this.subNavActiveIndex == 0 ?  this.speechBeforeChange : this.characterBeforeChange;
        for (let key of Object.keys(originalModel)) {
            model[key] = originalModel[key];
        }
        model.isEditing = false;
        this.autoSelectDisabled = true;
    }

    public saveCurrentEditing(): void {
        let currentModel = this.subNavActiveIndex == 0 ? this.currentSpeech : this.currentCharacter;
        if (currentModel.isUploaded) {
            let originalModel = this.subNavActiveIndex == 0 ? this.speechBeforeChange : this.characterBeforeChange;
            if (originalModel && currentModel.isEditing && JSON.stringify(originalModel) != JSON.stringify(currentModel)) {
                this.save(currentModel);
            }
            else {
                currentModel.isEditing = false;
            }
        }
        else {
            if (this.subNavActiveIndex == 0 && this.speechLibrary.indexOf(this.currentSpeech, 0) > -1) {
                this.save(currentModel);
            }
            else if (this.subNavActiveIndex == 1 && this.characters.indexOf(this.currentCharacter, 0) > -1) {
                this.save(currentModel);
            }
            else {
                currentModel.isEditing = false;
            }
        }
    }

    public selectCurrentRow(speech): void {
        if (this.autoSelectDisabled) {
            this.autoSelectDisabled = false;
        }
        else if (!speech.isEditing && this.mode != Mode.Delete && speech.previewLabelText != "Waiting...") {
            if (!speech.isSelected.list) {
                this.updateSelectedSpeechesNum(speech);
                speech.isSelected.list = true;
            }
            this.saveCurrentEditing();
        }
    }

    public updatePageContent(pageIndex): void {
        let startIndex = (pageIndex - 1) * this.pageSize;
        let endIndex = pageIndex * this.pageSize;
        if (this.subNavActiveIndex == 0) {
            this.speechLibOnCurrentPage = this.speechLibrary.slice(startIndex, endIndex);
            this.currentSpeechLibIndex = pageIndex;
        }
        else if (this.subNavActiveIndex == 1) {
            this.charactersOnCurrentPage = this.characters.slice(startIndex, endIndex);
            this.currentCharactersIndex = pageIndex;
        }
        else if (this.subNavActiveIndex == 2) {
            this.generatedPackagesOnCurrentPage = this.generatedPackages.slice(startIndex, endIndex);
            this.currentGeneratedPackagesIndex = pageIndex;
        }
    }

    public sortTable(tableName): void {
        var table, key;
        if (this.subNavActiveIndex == 0) {
            table = this.speechLibrary;
            key = "character";
        }
        else if (this.subNavActiveIndex == 1) {
            table = this.characters;
            key = "name";
        }
        else if (this.subNavActiveIndex == 2) {
            table = this.generatedPackages;
            key = "name";
        }

        this.sort(0, table.length - 1, table, key, this.getSortOrder(key));
        this.paginationRef.reset();
    }

    public savessmlTag(): void {
        this.modalRef.close();
        this.mode = Mode.List;

        for (let character of this.charactersOnCurrentPage) {
            if (character.name == this.currentCharacter.name) {
                let ssmlProsodyTags = [];
                for (let ssmlType of Object.keys(character.ssmlProsodyTagMappings)) {
                    if (this.currentCharacter.ssmlProsodyTagMappings[ssmlType] != "default") {
                        ssmlProsodyTags.push(ssmlType + "='" + this.currentCharacter.ssmlProsodyTagMappings[ssmlType] + "'");
                    }   
                }
                if (ssmlProsodyTags.length > 0) {
                    character.ssmlProsodyTags = ssmlProsodyTags;
                }
                else if (character.ssmlProsodyTags) {
                    delete character['ssmlProsodyTags'];
                }
                character.ssmlProsodyTagMappings = this.currentCharacter.ssmlProsodyTagMappings;

                character.ssmlLanguage = this.currentCharacter.ssmlLanguage;
                if (character.ssmlLanguage == this.voices[character.language][0]['languageCode']) {
                    delete character['ssmlLanguage'];
                }

                character.timbre = this.currentCharacter.timbre;
                if (character.timbre == 100){
                    delete character['timbre'];
                }

                this.updateCharacter(character);
                break;
            }
        }
    }

    public resetTimbre() {
        this.currentCharacter.timbre = 100;
    }

    public getDeleteModalTitle(): string {
        if (this.subNavActiveIndex == 0) {
            return "Delete Speech";
        }
        else if (this.subNavActiveIndex == 1) {
            return "Delete Chatacter";
        }
        else {
            return "Delete Generated Package";
        }
    }

    private getSortOrder(key): string {
        if (this.sortDir == "") {
            this.sortDir = "asc";
        }
        else {
            this.sortDir = this.sortDir == "asc" ? "desc" : "asc";
        }
        return this.sortDir;
    }

    private addExistingTagToList(tagName): void {
        for (let tag of this.filterTagList) {
            if (tag["name"] == tagName) {
                tag["count"]++;
                return;
            }
        }

        let tagIsSelected = false;
        for (let tag of this.originalfilterTagList) {
            if (tag["name"] == tagName) {
                tagIsSelected = tag["isSelected"];
                break;
            }
        }

        let tag = { name: tagName, isSelected: tagIsSelected , count: 1};
        this.filterTagList.push(tag);
    }

    private addCharacterInUseToList(characterName): void {
        for (let character of this.filterCharacterList) {
            if (character["name"] == characterName) {
                character["count"]++;
                return;
            }
        }
        let character = { name: characterName, isSelected: false, count: 1 };
        this.filterCharacterList.push(character);
    }

    private deleteExistingTagFromList(tagName): void {
        this.filterTagList = this.filterTagList.filter(function (tag) {
            let keepCurrentTag = tag["name"] != tagName;
            // Do not remove the tag from the filter list if some other speech entries contain it.
            if (tag["name"] == tagName && tag["count"] > 1) {
                tag["count"]--;
                keepCurrentTag = true;
            }
            return keepCurrentTag;
        });
    }

    private deleteCharacterInUseFromList(characterName): void {
        this.filterCharacterList = this.filterCharacterList.filter(function (character) {
            let keepCurrentCharacter = character["name"] != characterName;
            // Do not remove the character from the filter list if some other speech entries contain it.
            if (character["name"] == characterName && character["count"] > 1) {
                character["count"]--;
                keepCurrentCharacter = true;
            }
            return keepCurrentCharacter;
        });
    }

    private updateCharacterTab(): void {
        this.currentCharacter = this.defaultCharacter();
        this.characterBeforeChange = this.defaultCharacter();
        this.sortDir = "asc";
        this.isLoadingCharacters = true;

        this._apiHandler.getVoices().subscribe(response => {
            let obj = JSON.parse(response.body.text());
            this.voices = obj.result.voices;
            this.languages = Object.keys(this.voices);
            this.updateCharacters();
        }, err => {
            this.toastr.error("The language list did not refresh properly. " + err.message);
            this.isLoadingCharacters = false;
        });
    }

    private updateCharacters(): void {
        this._apiHandler.getCharacters().subscribe(response => {
            let obj = JSON.parse(response.body.text());
            this.characters = obj.result.characters;
            this.sort(0, this.characters.length - 1, this.characters, "name", this.sortDir);
            this.updatePaginationInfo();
            this.updatePageContent(1);

            if (this.characters.length == 0) {
                this.isLoadingCharacters = false;
            }

            this.initializeCharacters();
        }, err => {
            this.toastr.error("The characters did not refresh properly. " + err.message);
            this.isLoadingCharacters = false;
        });
    }

    private initializeCharacters(): void {
        for (let character of this.characters) {
            this.initializeCurrentCharacter(character);
        }
    }

    private initializeCurrentCharacter(character): void {
        character.isEditing = false;
        character.isUploaded = true;
        character.previewLabelText = "Preview";
        character.speechMarksStatus = [
            { name: "SSML", isChecked: character.speechMarks && character.speechMarks.includes("T") },
            { name: "Viseme", isChecked: character.speechMarks && character.speechMarks.includes("V") },
            { name: "Sentence/Word", isChecked: character.speechMarks && character.speechMarks.includes("S") }
        ];

        if (this.voices) {
            if (!character.voiceList) {
                this.getCharacterVoiceList(character);
            }
            character.ssmlProsodyTagMappings = { volume: "default", rate: "default", pitch: "default" };
            if (character.ssmlProsodyTags) {
                for (let ssmlProsodyTag of character.ssmlProsodyTags) {
                    let tagType = ssmlProsodyTag.split("=")[0];
                    let tmpStr = ssmlProsodyTag.split("=")[1];
                    let ssmlProsodyTagValue = tmpStr.substring(1, tmpStr.length - 1);
                    character.ssmlProsodyTagMappings[tagType] = ssmlProsodyTagValue;
                }
            }
            if (!character.ssmlLanguage) {
                character.ssmlLanguage = this.voices[character.language][0]['languageCode'];
            }
            if (!character.timbre) {
                character.timbre = 100;
            }
        }
    }

    private getCharacterVoiceList(character): void {
        for (let language in this.voices) {
            let voicelist = this.voices[language];
            for (let voice of voicelist) {
                if (voice["voiceId"] == character.voice) {
                    character.language = language;
                    character.voiceList = voicelist;
                    break;
                }
            }
        }
        this.isLoadingCharacters = false;
    }

    private updateCharacter(character): void {
        character.speechMarks = "";
        for (let speechMark of character.speechMarksStatus) {
            if (speechMark.isChecked) {
                character.speechMarks += speechMark.name == "SSML" ? "T" : speechMark.name.charAt(0);
            }
        }
        if (!character.isUploaded) {
            this.sortDir = "";
            this.addCharacterEntry(character, "Create");
        }
        else {
            if (character.name != this.characterBeforeChange.name) {
                this.sortDir = "";
            }
            let originalCharacterName = this.characterBeforeChange.name;

            // Delete the existing character first
            this._apiHandler.deleteCharacter(this.characterBeforeChange.name).subscribe(response => {
                // Create a new character using the updated info
                this.addCharacterEntry(character, "Update", originalCharacterName);
            }, err => {
                this.toastr.error("The character '" + originalCharacterName + "' was not updated. " + err.Message);
                this.cancel(character);
            });
        }
    }

    private createNewCharacter(characterName?): void {
        let newCharacter = this.defaultCharacter();
        if (this.characters.length > 0) {
            newCharacter = JSON.parse(JSON.stringify(this.characters[0]));
        }
        newCharacter.name = characterName ? characterName : "NewCharacter_" + this.characters.length;
        if (!newCharacter.name.match(/^[0-9a-zA-Z_]+$/)) {
            this.toastr.error("The character name can only contain non-alphanumeric characters and \"_\".");
            return;
        }
        newCharacter.ssmlProsodyTags = [];
        this.initializeCurrentCharacter(newCharacter);
        newCharacter.isUploaded = false;
        newCharacter.isEditing = true;

        this.characters.unshift(newCharacter);
        this.characterNames.unshift(newCharacter.name);
        this.updatePaginationInfo();
        this.paginationRef.reset();
        this.highlightNewRow = true;
        setTimeout(() => { this.highlightNewRow = false }, 1000);
        this.currentCharacter = newCharacter;

        if (characterName) {
            this.addCharacterEntry(newCharacter, "Create");
        }
    }

    private addCharacterEntry(model, operation: EntryStatus, originalCharacterName?): void {
        let body = {
            name: model.name,
            voice: model.voice,
            speechMarks: model.speechMarks
        }
        if (model.ssmlProsodyTags) {
            body["ssmlProsodyTags"] = model.ssmlProsodyTags;
        }
        if (model.ssmlLanguage && model.ssmlLanguage != this.voices[model.language][0]['languageCode']) {
            body["ssmlLanguage"] = model.ssmlLanguage;
        }
        if (model.timbre && model.timbre != 100) {
            body["timbre"] = model.timbre;
        }

        let name = originalCharacterName ? originalCharacterName : model.name;
        if (!body.name.match(/^[0-9a-zA-Z_]+$/)) {
            this.toastr.error("The character name can only contain non-alphanumeric characters and \"_\".");
            if (operation == "Create") {
                model.isEditing = true;
            }
            else if (operation == "Update") {
                this.cancel(model);
                this.addCharacterEntry(model, "Reset");
            }
            return;
        }

        this._apiHandler.createCharacter(body).subscribe(() => {
            if (operation == "Update") {
                this.toastr.success("The character '" + name + "' has been updated.");
                this.initializeCurrentCharacter(model);
            }
            else if (operation == "Create") {
                this.toastr.success("The character '" + name + "' was created.");
                this.initializeCurrentCharacter(model);
            }
        }, (err) => {
            if (operation == "Create") {
                this.toastr.error("The character '" + name + "' was not created correctly. " + err.message);
                model.isEditing = true;
            }
            else if (operation == "Update") {
                this.toastr.error("The character '" + name + "' was not updated. " + err.message);
                this.cancel(model);
                this.addCharacterEntry(model, "Reset");
            }
            else if (operation == "Reset") {
                this.toastr.error("The character '" + name + "' was not reset correctly. " + err.message);
            }
        });
    }

    private deleteCurrentCharacter(): void {
        this._apiHandler.deleteCharacter(this.currentCharacter.name).subscribe(response => {
            this.toastr.success("The character '" + this.currentCharacter.name + "' was deleted.");
            this.removeFromArray(this.characters, this.currentCharacter);
            this.removeFromArray(this.characterNames, this.currentCharacter.name);
            this.updatePaginationInfo();
            this.currentCharactersIndex = this.characterPages < this.currentCharactersIndex ? this.characterPages : this.currentCharactersIndex;
            this.updatePageContent(this.currentCharactersIndex);
        }, err => {
            this.toastr.error("The character '" + this.currentCharacter.name + "' could not be deleted. " + err.message);
        });
    }

    private updateGeneratedpackagesTab(): void {
        this.sortDir = "asc";
        this.isLoadingGeneratedPackages = true;
        this._apiHandler.getGeneratedPackages().subscribe(response => {
            let obj = JSON.parse(response.body.text());
            this.generatedPackages = obj.result.generatedPackages;
            this.sort(0, this.generatedPackages.length - 1, this.generatedPackages, "name", this.sortDir);
            this.updatePaginationInfo();
            this.updatePageContent(1);

            this.updateGeneratedPackagesInfo();
            this.selectedPackagesNum = 0;
            this.isLoadingGeneratedPackages = false;
        }, err => {
            this.toastr.error("The generated packages did not refresh properly. " + err.message);
            this.isLoadingGeneratedPackages = false;
        });
    }

    private updateGeneratedPackagesInfo(): void {
        var timedifference = new Date().getTimezoneOffset();
        for (let generatedPackage of this.generatedPackages) {
            generatedPackage.isSelected = false;
            let utcTime = generatedPackage.lastModified + ' UTC';
            generatedPackage.lastModified = moment.utc(utcTime, 'YYYY-MM-DD HH:mm:ss [UTC]').local();
        }
    }

    private deleteGeneratedPackage(generatedPackage): void {
        this._apiHandler.deleteGeneratedPackage(generatedPackage.name).subscribe(response => {
            let obj = JSON.parse(response.body.text());
            if (obj.result.status == "success") {
                this.toastr.success("The generated package '" + generatedPackage.name + "' was deleted.");
                if (generatedPackage.isSelected) {
                    this.selectedPackagesNum--;
                }
                this.removeFromArray(this.generatedPackages, generatedPackage);
                this.updatePaginationInfo();
                this.updatePageContent(this.currentGeneratedPackagesIndex);
            }
            else {
                this.toastr.error("The generated package '" + generatedPackage.name + "' was not deleted properly. " + obj.result.status);
            }
        }, err => {
            this.toastr.error("The generated package '" + generatedPackage.name + "' was not deleted properly. " + err.message);
        });
    }

    private markSettingAsLoaded(setting: string): void {
        this.loadedSettings.push(setting);
        if (this.loadedSettings.length == 2) {
            this.isLoadingSettings = false;
        }
    }

    private updateSettingsTab(): void {
        this.loadedSettings = [];
        this.isLoadingSettings = true;
        this._apiHandler.getRuntimeCapabilitiesStatus().subscribe(response => {
            let obj = JSON.parse(response.body.text());
            this.runtimeCapabilitiesEnabled = obj.result.status == "enabled" ? true : false;
            this.markSettingAsLoaded("runtimeCapabilities");
        }, err => {
            this.toastr.error("The settings did not refresh properly. " + err.message);
            this.markSettingAsLoaded("runtimeCapabilitiesERROR");
        });

        this._apiHandler.getCacheRuntimeGeneratedFiles().subscribe(response => {
            let obj = JSON.parse(response.body.text());
            this.runtimeCacheEnabled = obj.result.status == "enabled" ? true : false;
            this.markSettingAsLoaded("runtimeCache");
        }, err => {
            this.toastr.error("The settings did not refresh properly. " + err.message);
            this.markSettingAsLoaded("runtimeCacheERROR");
        });
    }

    private updateSpeechLibraryTab(): void {
        this.currentSpeech = this.defaultSpeech();
        this.speechBeforeChange = this.defaultSpeech();
        this.sortDir = "asc";
        this.originalfilterTagList = [];
        this.isLoadingSpeechLibrary = true;
        // Get the existing character names first to generate the dropdown box in the character column
        this._apiHandler.getCharacterNames().subscribe(response => {
            let obj = JSON.parse(response.body.text());
            this.characterNames = obj.result.characters;
            // Get the speech library
            this.updateSpeechesInfo();
        }, err => {
            this.toastr.error("The speech library did not refresh properly. " + err.message);
            this.isLoadingSpeechLibrary = false;
            });
    }

    private updateSpeechesInfo(): void {
        this._apiHandler.getSpeechLibrary().subscribe(response => {
            let obj = JSON.parse(response.body.text());
            this.speechLibrary = obj.result.entries;
            this.sort(0, this.speechLibrary.length - 1, this.speechLibrary, "character", this.sortDir);
            this.updatePaginationInfo();
            this.updatePageContent(1);
            this.isLoadingSpeechLibrary = false;
            this.selectedSpeechesNum = 0;
            this.selectedSpeechIDs = [];
            this.filterTagList = [];
            this.filterCharacterList = [];
            this.initializeSpeeches();

        }, err => {
            this.toastr.error("The speech library did not refresh properly. " + err.message);
            this.isLoadingSpeechLibrary = false;
        });
    }

    private updatePaginationInfo(): void {
        if (this.subNavActiveIndex == 0) {
            let numberEntries: number = this.speechLibrary.length;
            this.speechLinePages = Math.ceil(numberEntries / this.pageSize);
        }
        else if (this.subNavActiveIndex == 1) {
            let numberEntries: number = this.characters.length;
            this.characterPages = Math.ceil(numberEntries / this.pageSize);
        }
        else if (this.subNavActiveIndex == 2) {
            let numberEntries: number = this.generatedPackages.length;
            this.generatedPackagesPages = Math.ceil(numberEntries / this.pageSize);
        }
    }

    private initializeSpeeches(): void {
        for (let speech of this.speechLibrary) {
            this.initializeCurrentSpeech(speech);
        }
    }

    private initializeCurrentSpeech(speech): void {
        speech.isEditing = false;
        speech.isUploaded = true;
        speech.previewLabelText = "Preview";
        let index = this.selectedSpeechIDs.indexOf(speech.character + speech.line, 0);
        speech.isSelected = { list: index > -1, import: false };
        this.addCharacterInUseToList(speech.character);
        for (let tag of speech.tags) {
            this.addExistingTagToList(tag);
        }
    }

    private updateSpeechLine(speech): void {
        if (!speech.isUploaded) {
            this.sortDir = "";
            this.addSpeechEntry(speech, "Create");
        }
        else {
            if (speech.character != this.speechBeforeChange.character) {
                this.sortDir = "";
            }
            let body = {
                character: this.speechBeforeChange.character,
                line: this.speechBeforeChange.line
            }
            // Delete the existing speech first
            this._apiHandler.deleteSpeech(body).subscribe(response => {
                this.deleteCharacterInUseFromList(this.speechBeforeChange.character);
                this.originalfilterTagList = JSON.parse(JSON.stringify(this.filterTagList));
                for (let tagName of this.speechBeforeChange.tags) {
                    this.deleteExistingTagFromList(tagName);
                }
                // Create a new speech using the updated info
                this.addSpeechEntry(speech, "Update", body.line);
            }, err => {
                this.toastr.error("The speech '" + body.line + "' was not updated. " + err.Message);
                this.cancel(speech);
            });
        }
    }

    private createNewSpeech(): void {
        if (this.characterNames.length == 0) {
            this.createNewCharacter("NewCharacter_0");
            this.toastr.success("A new character was created. Switch to the Characters tab to edit it.");
        }

        let newSpeech = this.defaultSpeech();      
        if (this.speechLibrary.length > 0) {
            newSpeech = JSON.parse(JSON.stringify(this.speechLibrary[0]));
        }
        else {
            newSpeech.character = this.characterNames[0];
        }
        newSpeech.line = "NewSpeechLine_" + this.speechLibrary.length;
        this.initializeCurrentSpeech(newSpeech);
        newSpeech.isUploaded = false;
        newSpeech.isEditing = true;

        this.speechLibrary.unshift(newSpeech);
        this.updatePaginationInfo();
        this.paginationRef.reset();
        this.highlightNewRow = true;
        setTimeout(() => { this.highlightNewRow = false }, 1000);
        this.currentSpeech = newSpeech;
    }

    private addSpeechEntry(model, operation: EntryStatus, originalLine?): void {
        let body = {
            character: model.character,
            line: model.line,
            tags: model.tags
        }
        let line = originalLine ? originalLine : model.line;

        this._apiHandler.createSpeech(body).subscribe(response => {
            let obj = JSON.parse(response.body.text());
            status = obj.result.status;

            if (status == "duplicate") {
                this.toastr.error("Speech line '" + body.line + "' already exists!");
                if (operation == "Create") {
                    model.isEditing = true;
                }
                if (operation == "Update") {
                    this.cancel(model);
                    this.addSpeechEntry(model, "Reset");
                }
            }
            else {
                if (operation == "Update") {
                    this.toastr.success("The speech '" + line + "' has been updated.");
                    this.initializeCurrentSpeech(model);
                }               
                else if (operation == "Create") {
                    this.toastr.success("The speech '" + line + "' was created.");
                    this.initializeCurrentSpeech(model);
                }
            }
        }, (err) => {
            if (operation == "Create") {
                this.toastr.error("The speech '" + line + "'was not created correctly. " + err.message);
                model.isEditing = true;
            }
            else if (operation == "Update") {
                this.toastr.error("The speech '" + line + "' was not updated. " + err.message);
                this.cancel(model);
                this.addSpeechEntry(model, "Reset");
            }
            else if (operation == "Reset") {
                this.toastr.error("The speech '" + line + "' was not reset correctly. " + err.message);
            }
        });
    }

    private deleteSpeechLine(speech): void {
        for (let tagName of speech.tags) {
            this.deleteExistingTagFromList(tagName);
        }

        let body = {
            character: speech.character,
            line: speech.line
        }
        this._apiHandler.deleteSpeech(body).subscribe(response => {
            this.toastr.success("The speech '" + body.line + "' was deleted.");
            this.removeFromArray(this.speechLibrary, speech);
            if (speech.isSelected["list"]) {
                this.updateSelectedSpeechesNum(speech);
            }
            this.updatePaginationInfo();
            this.currentSpeechLibIndex = this.speechLinePages < this.currentSpeechLibIndex ? this.speechLinePages : this.currentSpeechLibIndex;
            this.updatePageContent(this.currentSpeechLibIndex);
        }, err => {
            this.toastr.error("The speech '" + body.line + "' could not be deleted. " + err.message);
            this.selectedSpeechesNum--;
        });
    }

    private removeFromArray(array, key): void {
        let index = array.indexOf(key, 0);
        if (index > -1) {
            array.splice(index, 1);
        }
    }

    private ImportableSpeechesExist(speechesData, fields): boolean {
        this.importableSpeeches = [];
        for (let speechData of speechesData) {
            let importableSpeech = this.defaultSpeech();
            importableSpeech.tags = [];
            for (let field of this.customFields) {
                if (field != 'MD5' && this.currentCustomMappings[field] != "tag" && this.currentCustomMappings[field] != "") {
                    importableSpeech[this.currentCustomMappings[field]] = speechData[field];
                }
                else if (this.currentCustomMappings[field] != "" && speechData[field]){
                    let tags = String(speechData[field]).split(",");
                    for (let tag of tags) {
                        if (tag != "") {
                            let value = field == "Tags" ? tag : field + ":" + tag;
                            importableSpeech.tags.push(value);
                        }
                    }
                }
            }
            if (importableSpeech["character"] && importableSpeech["character"] != "") {
                this.addImportableSpeech(importableSpeech);
            }
        }

        return this.importableSpeeches.length > 0 ? true : false;
    }

    private addImportableSpeech(speech): void {
        for (let existingSpeech of this.speechLibrary) {
            if (existingSpeech.character == speech.character && existingSpeech.line == speech.line) {
                return;
            }
        }

        this.importableSpeeches.push(speech);
    }

    private importSelectedSpeeches(): void {
        this.modalRef.close();

        let speechesToBeImported = [];
        for (let speech of this.importableSpeeches) {
            if (speech.isSelected["import"]) {
                let newSpeech = new SpeechEntry(speech);
                let index = this.characterNames.indexOf(speech.character, 0);
                if (index == -1) {
                    this.createNewCharacter(speech.character);
                }
                speechesToBeImported.push(newSpeech);
            }
        }
        if (speechesToBeImported.length == 0) {
            return;
        }
        let body = {
            "entries": speechesToBeImported
        }
        this._apiHandler.importSpeeches(body).subscribe(() => {
            this.toastr.success("The speeches were imported.");
            for (let speech of speechesToBeImported) {
                this.initializeCurrentSpeech(speech);
                this.speechLibrary.unshift(speech);
                this.updatePaginationInfo();
                this.paginationRef.reset();
            }
            this.updatePaginationInfo();
            this.paginationRef.reset();
        }, (err) => {
            this.toastr.error("The speeches were not imported correctly.");
        });

        this.mode = Mode.List;
    }

    private characterNameExists(characterName): boolean {
        for (let character of this.characters) {
            if (character.name == characterName) {
                return true;
            }
        }
        return false;
    }

    private getPackageUrl(name, inLib = true): void {
        this._apiHandler.getPackageUrl(name, inLib).subscribe(response => {
            let obj = JSON.parse(response.body.text());
            let url = obj.result.url;
            if (url != "pending") {
                window.open(obj.result.url);
                this.setPendingJobStatus(name, "Success", !inLib);
            }
            else{
                setTimeout(() => { this.getPackageUrl(name, inLib); }, 1000);
            }
        }, (err) => {
            this.toastr.error("The zip file was not downloaded correctly." + err.message);
            // if we are looking up by key instead of name then it was not a download from the existing library
            this.setPendingJobStatus(name, "Failure", !inLib);
        });
    }

    private setPendingJobStatus(name, status: DownloadJobStatus, lookupByKey = false): void {
        let index = 0;
        for (index = 0; index < this.pendingJobs.length; index++) {
            if (!lookupByKey){
                if (this.pendingJobs[index]["name"] == name && this.pendingJobs[index]["status"] == "Pending") {
                    this.pendingJobs[index]["status"] = status;
                    break;
                }
            }
            else
            {
                if (this.pendingJobs[index]["key"] == name && this.pendingJobs[index]["status"] == "Pending") {
                    this.pendingJobs[index]["status"] = status;
                    break;
                }
            }
        }
        if (status == "Success") {
            this.pendingJobs.splice(index, 1);
        }
    }

    private unselectSpeechLines(): void {
        for (let speech of this.speechLibrary) {
            speech.isSelected["list"] = false;
        }
        this.selectAllCheckBoxRef.nativeElement.checked = false;
        this.selectedSpeechesNum = 0;
        this.selectedSpeechIDs = [];
    }

    private parseCsvFile(file): void {
        this.papa.parse(file, {
            header: true,
            dynamicTyping: true,
            complete: function (results) {
                this.importableSpeechesData = results.data;
                this.currentCustomMappings = {};
                let recentCustomMapping = {};
                this.customMappingsLoaded = false;
                this._apiHandler.getCustomMappings().subscribe(response => {
                    let obj = JSON.parse(response.body.text());
                    let recentCustomMappings = obj.result.mappings;
                    for (let field of this.customFields) {
                        this.currentCustomMappings[field] = field in recentCustomMappings ? recentCustomMappings[field] : "";
                    }
                    this.mode = Mode.Import;
                }, (err) => {
                    this.mode = Mode.Import;
                    this.toastr.error("The most recent custom mappings were not loaded correctly." + err.message);

                });
            }.bind(this)
        })
    }

    private playAudio(buffer): void {
        let wavBuffer = this.generateWavBuffer(buffer);
        this.audioContext.decodeAudioData(wavBuffer, function (buffer) {
            var source = this.audioContext.createBufferSource(); // creates a sound source
            source.buffer = buffer;                    // tell the source which sound to play
            source.connect(this.audioContext.destination);       // connect the source to the context's destination (the speakers)
            source.start(0);
        }.bind(this), function (e) {
            this.toastr.error("Error with decoding audio data.");
            }.bind(this));
    }

    private generateWavBuffer(buffer): ArrayBuffer {
        let header = this.generateWavHeader(buffer.byteLength);
        let wav = new Uint8Array(header.byteLength + buffer.byteLength);
        wav.set(new Uint8Array(header), 0);
        wav.set(new Uint8Array(buffer), header.byteLength);
        return wav.buffer;
    }

    private generateWavHeader(length): ArrayBuffer {
        let numChannels = 1;
        let sampleRate = 16000;
        let bytesPerSample = 2;
        let blockAlign = numChannels * bytesPerSample;
        let byteRate = sampleRate * blockAlign;
        let dataSize = length;

        let buffer = new ArrayBuffer(44);
        let dataView = new DataView(buffer);

        let headerInfo = [
            { data: "RIFF", type: "string", startPosition: 0 },             // ChunkID
            { data: dataSize + 36, type: "uint32", startPosition: 4 },      // ChunkSize
            { data: "WAVE", type: "string", startPosition: 8 },             // Format
            { data: "fmt ", type: "string", startPosition: 12 },            // Subchunk1ID
            { data: 16, type: "uint32", startPosition: 16 },                // Subchunk1Size
            { data: 1, type: "uint16", startPosition: 20 },                 // AudioFormat
            { data: numChannels, type: "uint16", startPosition: 22 },       // NumChannels
            { data: sampleRate, type: "uint32", startPosition: 24 },        // SampleRate
            { data: byteRate, type: "uint32", startPosition: 28 },          // ByteRate
            { data: blockAlign, type: "uint16", startPosition: 32 },        // BlockAlign
            { data: bytesPerSample * 8, type: "uint16", startPosition: 34 },// BitsPerSample
            { data: "data", type: "string", startPosition: 36 },            // Subchunk2ID
            { data: dataSize, type: "uint32", startPosition: 40 }           // Subchunk2Size
        ];

        for (let headerItem of headerInfo) {
            this.writeBuffer(headerItem.data, headerItem.type, headerItem.startPosition, dataView);
        }

        return buffer;
    };

    private writeBuffer(data, type, startPosition, dataView): void {
        if (type == "string") {
            for (var i = 0; i < data.length; i++) {
                dataView.setUint8(startPosition + i, data.charCodeAt(i));
            }
        }
        else if (type == "uint32") {
            dataView.setUint32(startPosition, data, true);
        }
        else if (type == "uint16") {
            dataView.setUint16(startPosition, data, true);
        }
    }

    private downloadGeneratedPackages(): void {
        for (let generatedPackage of this.generatedPackages) {
            if (generatedPackage.isSelected) {
                let packageName = generatedPackage.name.substring(0, generatedPackage.name.length - 4)
                this.getPackageUrl(packageName);
                generatedPackage.isSelected = false;
            }
        }
        this.selectAlleneratedPackagesRef.nativeElement.checked = false;
        this.selectedPackagesNum = 0;
    }

    private updateSelectedSpeechesNum(speech): void {
        if (speech.isSelected.list) {
            this.selectedSpeechesNum--;

            let index = this.selectedSpeechIDs.indexOf(speech.character + speech.line, 0);
            if (index > -1) {
                this.selectedSpeechIDs.splice(index, 1);
            }   
        }
        else {
            this.selectedSpeechesNum++;
            this.selectedSpeechIDs.push(speech.character + speech.line);
        }
    }

    private updateSelectedPackgesNum(generated_package): void {
        if (generated_package.isSelected) {
            this.selectedPackagesNum--;
        }
        else {
            this.selectedPackagesNum++;
        }
    }

    private defaultSpeech(): SpeechEntry {
        return new SpeechEntry({
            character: "",
            line: "NewSpeechLine_0",
            tags: [],
            isSelected: { import: false, list: false },
            previewLabelText: "Preview",
            isEditing: false,
            uploaded: false
        });
    }

    private defaultCharacter(): CharacterEntry {
        return new CharacterEntry({
            name: "NewCharacter_0",
            voice: "Joanna",
            speechMarksStatus: [{ name: "SSML", isChecked: true },
                { name: "Viseme", isChecked: true },
                { name: "Sentence/Word", isChecked: true }],
            previewLabelText: "Preview",
            speechMarks: "SVT",
            isEditing: false,
            uploaded: false
        });
    }

    private sort(left, right, table, key, dir): void {
        // Use quick sort algorithm to sort the table by a given column
        let i = left, j = right;
        let pivot = table[Math.floor((left + right) / 2)];

        /* partition */
        while (i <= j) {
            if (dir == "asc") {
                while (table[i][key].toLowerCase() < pivot[key].toLowerCase()) {
                    i++;
                }
                while (table[j][key].toLowerCase() > pivot[key].toLowerCase()) {
                    j--;
                }
            }
            else if (dir == "desc") {
                while (table[i][key].toLowerCase() > pivot[key].toLowerCase())
                    i++;
                while (table[j][key].toLowerCase() < pivot[key].toLowerCase())
                    j--;
            }

            if (i <= j) {
                let tmp = JSON.parse(JSON.stringify(table[i]));
                table[i] = JSON.parse(JSON.stringify(table[j]));
                table[j] = JSON.parse(JSON.stringify(tmp));

                i++;
                j--;
            }
        };

        /* recursion */
        if (left < j)
            this.sort(left, j, table, key, dir);
        if (i < right)
            this.sort(i, right, table, key, dir);
    }

    private verifyMappings(customField): boolean {
        for (let field of this.customFields) {
            if (this.currentCustomMappings[customField] != "" && this.currentCustomMappings[customField] != "tag") {
                if (field != customField && this.currentCustomMappings[field] == this.currentCustomMappings[customField]) {
                    return false;
                }
            }
        }
        return true;
    }
}