import { Component, ViewChild, SecurityContext } from '@angular/core';
import { Observable } from 'rxjs/rx';
import { Http } from '@angular/http';
import { FormControl } from '@angular/forms';
import { Tackable, Measurable, Stateable, StyleType, Gemifiable, TackableStatus, TackableMeasure } from 'app/view/game/module/cloudgems/class/index';
import { DynamicGem } from 'app/view/game/module/cloudgems/class/index';
import { DateTimeUtil, ApiHandler } from 'app/shared/class/index';
import { DateTimeRangePickerComponent, ModalComponent } from 'app/shared/component/index';
import { AwsService } from "app/aws/aws.service";
import { Subscription } from 'rxjs/rx';
import { SearchResult } from 'app/view/game/module/shared/component/search.component';

declare const toastr: any;

const cacheTimeLimitInSeconds = 300; // 5 minute

export enum LeaderboardMode {
    Add,
    List,
    Show,
    Edit,
    Delete,
    Unban,
    ProcessQueue
}

export enum PlayerMode {
    List,
    BanUser,
    DeleteScore
}

export class Leaderboard extends DynamicGem {
    public identifier: "leaderboard";
    public apiHandler: LeaderboardAPI;

    public classType(): any {
        const clone = Leaderboard;
        return clone;
    }

    public tackable = <Tackable>{
        displayName: "Leaderboard",
        iconSrc: "https://m.media-amazon.com/images/G/01/cloudcanvas/images/Leadboard_icon._V536715120_.png",
        cost: "High",
        state: new TackableStatus(),
        metric: new TackableMeasure()
    };

    public get htmlTemplateUrl(): string {
        return this.context.provider.definition.isProd ? this.getSignedUrl(this.context.meta.key + "/cloudgemleaderboard.html") : "external/cloudgemleaderboard/cloudgemleaderboard.html";
    }
    public get styleTemplateUrl(): string {
        return this.context.provider.definition.isProd ? this.getSignedUrl(this.context.meta.key + "/cloudgemleaderboard.css") : "external/cloudgemleaderboard/cloudgemleaderboard.css";
    }

    private mode: LeaderboardMode;
    private aggregateModes: any;
    private leaderboardModes: any;
    private currentLeaderboard: LeaderboardForm;
    private leaderboards: LeaderboardForm[];

    private playerMode: PlayerMode;
    private playerModes: any;
    private currentScore: any;
    private currentPlayer: string;
    private currentPlayerSearch: string;
    private currentScores: PlayerMeasure[];
    private bannedPlayers: String[];

    private isLoadingLeaderboards: boolean;
    private isLoadingScores: boolean;
    private isLoadingBannedPlayers: boolean;
    private isLoadingProcessedQueue: boolean;
    private pageSize: number = 25;
    private subNavActiveIndex: number = 0;

    private getScoresSub: Subscription;

    // Form Controls
    searchScoresControl = new FormControl();

    @ViewChild(ModalComponent) modalRef: ModalComponent;


    //Need an empty constructor or Firefox will complain.
    constructor() {
        super();
    }

    public initialize(context: any) {
        super.initialize(context);
        this.apiHandler = new LeaderboardAPI(this.context.meta.cloudFormationOutputMap["ServiceUrl"].outputValue, this.context.provider.http, this.context.provider.aws);
        this.apiHandler.report(this.tackable.metric);
        this.apiHandler.assign(this.tackable.state);
    }

    ngOnInit() {
        this.aggregateModes = [
            {
                display: "Overwrite",
                server: "OVERWRITE"
            },
            {
                display: "Increment",
                server: "INCREMENT"
            }
        ];

        //bind proper scopes
        this.addModal = this.addModal.bind(this);
        this.editModal = this.editModal.bind(this);
        this.deleteModal = this.deleteModal.bind(this);
        this.dismissModal = this.dismissModal.bind(this);

        this.list = this.list.bind(this);
        this.addLeaderboard = this.addLeaderboard.bind(this);
        this.editLeaderboard = this.editLeaderboard.bind(this);
        this.show = this.show.bind(this);
        this.deleteLeaderboard = this.deleteLeaderboard.bind(this);

        this.deleteScoreModal = this.deleteScoreModal.bind(this);
        this.deleteScore = this.deleteScore.bind(this);
        this.getBannedPlayers = this.getBannedPlayers.bind(this);
        this.banPlayerModal = this.banPlayerModal.bind(this);
        this.banPlayer = this.banPlayer.bind(this);
        this.dismissPlayerModal = this.dismissPlayerModal.bind(this);
        this.unbanPlayerModal = this.unbanPlayerModal.bind(this);
        this.unbanPlayer = this.unbanPlayer.bind(this);
        this.processLeaderboardQueue = this.processLeaderboardQueue.bind(this);

        this.checkLocalScoreCacheForChanges = this.checkLocalScoreCacheForChanges.bind(this);
        // end of scope binding

        // Pass modes to view and set default mode
        this.leaderboardModes = LeaderboardMode;
        this.leaderboards = [];
        this.list();

        this.playerModes = PlayerMode;
        this.playerMode = PlayerMode.List;
        this.currentPlayerSearch = "";

        this.currentLeaderboard = new LeaderboardForm("", null, null, null);
    }

    playerSearchUpdated(searchResult: SearchResult) {
        if (searchResult.value || searchResult.value.length > 0) {
            this.currentLeaderboard.numberOfPages = 0;
            this.apiHandler.searchLeaderboard(this.currentLeaderboard.name, searchResult.value)
                .subscribe(response => {
                    let obj = JSON.parse(response.body.text());
                    let searchResults = obj.result.scores;
                    this.currentScores = searchResults;
                    this.currentScores = this.checkLocalScoreCacheForChanges();
                    this.isLoadingScores = false;
                }, err => {
                    toastr.error(err);
                    this.isLoadingScores = false;
                })
        } else {
            // If there is no value just get the default list of scores for the leaderboard
            this.show(this.currentLeaderboard);
        }
    }
    protected validate(model): boolean {
        let isValid: boolean = true;
        let split = model.name.split(" ");

        model.validation = {
            id: {},
            min: {},
            max: {}
        }
        if (split.length > 1) {
            isValid = false;
            model.validation.id = {
                valid: false,
                message: "The leaderboard identifier can not have any spaces in it."
            }
        } else if (split.length == 0 || split[0].length == 0) {
            isValid = false;
            model.validation.id = {
                valid: false,
                message: "The leaderboard identifier can not be empty."
            }
        } else if(!split[0].toString().match(/^[0-9a-zA-Z]+$/)) {
            isValid = false;
            model.validation.id = {
                valid: false,
                message: "The leaderboard identifier can not contains non-alphanumeric characters"
            }
        }

        else {
            split[0] = split[0].trim();
        }

        model.validation.max = {
            valid: true
        }
        model.validation.min = {
            valid: true
        }
        if (model.max != null && Number(model.max) === NaN) {
            model.validation.max = false;
        } else if (model.max != null) {
            model.validation.max = { valid: Number(model.max) >= 0 };
        }
        if (model.min != null && Number(model.min) === NaN) {
            model.validation.min = false;
        } else if (model.min != null) {
            model.validation.min = { valid: Number(model.min) >= 0 };
        }

        if (!model.validation.max.valid) {
            model.validation.max.message = "This value must be numeric and greater than zero."
            isValid = false;
        }

        if (!model.validation.min.valid) {
            model.validation.min.message = "This value must be numeric and greater than zero."
            isValid = false;
        }

        if (model.validation.max.valid && model.validation.min.valid) {
            let max = Number(model.max);
            let min = Number(model.min);
            if (min > max) {
                model.validation.max.valid = false;
                model.validation.min.valid = false;
                model.validation.min.message = "The minimum reportable value must be a greater than the maximum reportable value."
                isValid = false;
            }
        }
        let addMode: number = LeaderboardMode.Add;
        if (Number(model.state) == addMode) {
            for (var i = 0; i < model.leaderboards.length; i++) {
                let id = model.leaderboards[i].name
                if (id == split[0]) {
                    isValid = false;
                    model.validation.id = {
                        valid: false,
                        message: "That leaderboard identifier is already in use in your project.  Please choose another name."
                    }
                    break;
                }
            }
        }

        if (isValid)
            model.validation = null;

        return isValid;
    }

    // Leaderboard action stub functions
    public addModal(leaderboard): void {
        this.mode = LeaderboardMode.Add;
        this.currentLeaderboard = new LeaderboardForm("", this.aggregateModes[0].display);
        this.currentLeaderboard.leaderboards = this.leaderboards;
        this.currentLeaderboard.state = this.mode.toString();
    }

    public editModal(leaderboard): void {
        this.mode = LeaderboardMode.Edit;
        this.currentLeaderboard = new LeaderboardForm(leaderboard.name, leaderboard.mode, leaderboard.min, leaderboard.max);
        this.currentLeaderboard.leaderboards = this.leaderboards;
        this.currentLeaderboard.state = this.mode.toString();
    }

    public deleteModal(leaderboard): void {
        this.mode = LeaderboardMode.Delete;
        this.currentLeaderboard = leaderboard;
    }

    public dismissModal(): void {
        this.mode = LeaderboardMode.List;
    }

    public show(leaderboard: any, currentPageIndex: number = 1): void {
        this.currentLeaderboard = leaderboard;
        this.mode = LeaderboardMode.Show;
        this.isLoadingScores = true;

        if (leaderboard.additional_data === undefined) {
            leaderboard.additional_data = <PageModel>{
                users: [],
                page: currentPageIndex,
                page_size: this.pageSize
            }
            leaderboard.numberOfPages = 0;
        }

        leaderboard.additional_data.page = currentPageIndex - 1;

        if (this.getScoresSub) {
            this.getScoresSub.unsubscribe();
        }
        this.getScoresSub = this.apiHandler.getEntries(leaderboard.name, leaderboard.additional_data).subscribe(response => {
            let json = JSON.parse(response.body.text());
            this.currentScores = json.result.scores;
            leaderboard.numberOfPages = json.result.total_pages;
            this.currentScores = this.checkLocalScoreCacheForChanges();
            this.isLoadingScores = false;
        }, err => {
            toastr.error("The leaderboard with id '" + leaderboard.name + "' failed to retrieve its data: " + err);
            this.isLoadingScores = false;
        });
    }

    public list(): void {
        this.isLoadingLeaderboards = true;
        this.mode = LeaderboardMode.List;
        this.currentScores = null;
        this.apiHandler.get("stats").subscribe(response => {
            let obj = JSON.parse(response.body.text());
            this.leaderboards = obj.result.stats;
            this.isLoadingLeaderboards = false;
        }, err => {
            toastr.error("The leaderboards did not refresh properly: " + err);
            this.isLoadingLeaderboards = false;
        });
    }

    // Functions inside modals
    public addLeaderboard(model): void {
        if (this.validate(model)) {
            var body = this.getPostObject();
            this.modalRef.close();
            this.apiHandler.addLeaderboard(body).subscribe(response => {
                toastr.success("The leaderboard with id '" +  this.currentLeaderboard.name + "' was created.");
                this.currentLeaderboard = new LeaderboardForm("");
                this.list();
                this.mode = LeaderboardMode.List;
            },err => {
                toastr.error("The leaderboard could not be created: " + err);
                this.list();
            });
        }
    }

    public editLeaderboard(model): void {
        if (this.validate(model)) {
            var body = this.getPostObject();
            this.modalRef.close();
            this.apiHandler.editLeaderboard(body).subscribe(response => {
                toastr.success("The leaderboard with id '" +  this.currentLeaderboard.name + "' was updated.");
                this.list();
                this.mode = LeaderboardMode.List;
            }, err => {
                toastr.error("The leaderboard could not be updated: " + err);
                this.list();
            });
        }
    }

    public deleteLeaderboard(): void {
        this.modalRef.close();
        this.apiHandler.deleteLeaderboard(this.currentLeaderboard.name).subscribe(response => {
            toastr.success("The leaderboard with id '" +  this.currentLeaderboard.name + "' has been deleted.");
            this.list();
            this.mode = LeaderboardMode.List;
        }, err => {
            toastr.error("The leaderboard could not be deleted: "  + err);
        });
    }

    public subNavUpdated(activeIndex: number): void {
        this.subNavActiveIndex = activeIndex;
        if (this.subNavActiveIndex == 0) {
            this.list();
        } else if (this.subNavActiveIndex == 1) {
            this.getBannedPlayers();
        }
    }

    public deleteScoreModal(model): void {
        this.playerMode = PlayerMode.DeleteScore;
        this.currentScore = model;
    }

    public deleteScore(): void {
        this.isLoadingScores = true;
        this.modalRef.close();
        this.apiHandler.deleteScore(this.currentLeaderboard.name, this.currentScore.user).subscribe(response => {
           toastr.success("The score for player '" +  this.currentScore.user + "' was deleted successfully.");
           this.context.provider.cache.setObject(this.currentScore, this.currentScore);
           this.currentScores = this.checkLocalScoreCacheForChanges();
           this.isLoadingScores = false;
            this.playerMode = PlayerMode.List;
        }, err => {
           this.playerMode = PlayerMode.List;
           toastr.error("The score for could not be deleted: " + err);
        });
    }

    public getBannedPlayers(): void {
        this.isLoadingBannedPlayers = true;
         this.apiHandler.get("player/ban_list").subscribe(response => {
            this.isLoadingBannedPlayers = false;
            let obj = JSON.parse(response.body.text());
            this.bannedPlayers = obj.result.players;
         }, err => {
             this.isLoadingBannedPlayers = false;
             toastr.error("Unable to get the list of banned players: " + err);
        });
    }

    public banPlayerModal(score): void {
        this.playerMode = PlayerMode.BanUser;
        this.currentScore = score;
    }

    public banPlayer(): void {
        this.isLoadingScores = true;
        this.modalRef.close();
        this.apiHandler.banPlayer(this.currentScore.user).subscribe(response => {
            let obj = JSON.parse(response.body.text());
            toastr.success("The player '" +  this.currentScore.user + "' has been banned.");
            this.context.provider.cache.set(this.currentScore.user, this.currentScore.user);
            this.currentScores = this.checkLocalScoreCacheForChanges();
            this.isLoadingScores = false;
            this.playerMode = PlayerMode.List;
        }, err => {
            this.playerMode = PlayerMode.List;
            toastr.error("The player could not be banned: " + err);
        });
    }

    public unbanPlayerModal(model): void {
        this.currentPlayer = model;
        this.mode = LeaderboardMode.Unban;
    }

    public unbanPlayer(): void {
        this.modalRef.close();
        this.apiHandler.unbanPlayer(this.currentPlayer).subscribe(response => {
            let obj = JSON.parse(response.body.text());
            toastr.success("The player '" + this.currentPlayer + "' has had their ban lifted");
            toastr.success("The player '" +  this.currentPlayer + "' has had their ban lifted.");
            this.context.provider.cache.remove(this.currentPlayer);
            this.getBannedPlayers();
            this.mode = LeaderboardMode.List;
        }, err => {
            this.mode = LeaderboardMode.List;
            toastr.error("The ban could not be removed: " + err);
        });
    }

    public dismissPlayerModal(): void {
        this.playerMode = PlayerMode.List;
    }

    public promptProcessLeaderboard(): void {
        this.mode = LeaderboardMode.ProcessQueue
    }

    public processLeaderboardQueue(): void {
        this.modalRef.close();
        this.mode = LeaderboardMode.List
        this.apiHandler.processRecords().subscribe(response => {
            toastr.success("The leaderboard processor has been started. Processing should only take a few moments.");
        }, err => {
            toastr.error("The leaderboard process failed to start" + err);
        });
    }

    private checkLocalScoreCacheForChanges(): PlayerMeasure[] {
        var currentCachedScores: PlayerMeasure[] = this.currentScores;
        var indexBlacklist = Array<number>();

        for (var i = 0; i < this.currentScores.length; i++) {
            let isScoreDeleted = this.context.provider.cache.objectExists(this.currentScores[i]);
            let isPlayerBanned = this.context.provider.cache.exists(this.currentScores[i].user);
            if (isScoreDeleted || isPlayerBanned) {
                indexBlacklist.push(i);
            }
        }

        var itemsRemoved = 0;
        indexBlacklist.forEach((blackListedIndex) => {
            currentCachedScores.splice(blackListedIndex - itemsRemoved, 1);
            itemsRemoved++;
        });

        return currentCachedScores;
    }

    private getPostObject(): LeaderboardForm {
        if (this.currentLeaderboard.min == null && this.currentLeaderboard.max == null) {
            var body = new LeaderboardForm(this.currentLeaderboard.name, this.currentLeaderboard.mode.toUpperCase());
        } else if (this.currentLeaderboard.min != null && this.currentLeaderboard.max != null) {
            var body = new LeaderboardForm(this.currentLeaderboard.name, this.currentLeaderboard.mode.toUpperCase(), this.currentLeaderboard.min, this.currentLeaderboard.max);
        } else if (this.currentLeaderboard.min != null) {
            var body = new LeaderboardForm(this.currentLeaderboard.name, this.currentLeaderboard.mode.toUpperCase(), this.currentLeaderboard.min);
        } else if (this.currentLeaderboard.max != null) {
            var body = new LeaderboardForm(this.currentLeaderboard.name, this.currentLeaderboard.mode.toUpperCase(), undefined, this.currentLeaderboard.max);
        }
        return body;
    }
}

export function getInstance(context: any): Gemifiable {
    let gem = new Leaderboard();
    gem.initialize(context);
    return gem;
}

export class LeaderboardAPI extends ApiHandler {
    constructor(serviceBaseURL: string, http: Http, aws: AwsService) {
        super(serviceBaseURL, http, aws);
    }

    public report(metric: Measurable) {
        metric.name = "Total Leaderboards";
        metric.value = "Loading...";
        super.get("stats").subscribe(response => {
            let obj = JSON.parse(response.body.text());
            metric.value = obj.result.stats.length;
        }, err => {
            metric.value = "Offline";
        });
    }

    public assign(tackableStatus: TackableStatus) {
        tackableStatus.label = "Loading";
        tackableStatus.styleType = "Loading";
        super.get("service/status").subscribe(response => {
            let obj = JSON.parse(response.body.text());
            tackableStatus.label = obj.result.status == "online" ? "Online" : "Offline";
            tackableStatus.styleType = obj.result.status == "online" ? "Enabled" : "Offline";
        }, err => {
            tackableStatus.label = "Offline";
            tackableStatus.styleType = "Offline";
        })
    }

    public getEntries(leaderboard: string, body: any): Observable<any> {
        return super.post("leaderboard/" + leaderboard, body);
    }

    public deleteLeaderboard(id: string): Observable<any> {
        return super.delete("stats/" + id);
    }

    public addLeaderboard(body: any): Observable<any> {
        return super.post("stats", body);
    }

    public editLeaderboard(body: any): Observable<any> {
        return super.post("stats", body);
    }

    public searchLeaderboard(leaderboardid:string, id:string): Observable<any> {
        return super.get("score/" + leaderboardid + "/" + id);
    }

    public deleteScore(leaderboardId: string, currentScore: any): Observable<any> {
        return super.delete("score/" + leaderboardId + "/" + currentScore);
    }

    public banPlayer(user: string): Observable<any> {
        return super.post("player/ban/" + user);
    }

    public unbanPlayer(user: string): Observable<any> {
        return super.delete("player/ban/" + encodeURIComponent(user));
    }

    public processRecords(): Observable<any> {
        return super.get("process_records");
    }

}

///Models
class PageModel {
    public users: string[]
    public page: number
    public page_size: number
}

class LeaderboardForm {
    name?: string;
    mode?: string;
    min?: string;
    max?: string;
    leaderboards?: LeaderboardForm[]
    state?: string;
    additional_data?: PageModel;
    numberOfPages?: number;

    constructor(name?: string, mode?: string, min?: string, max?: string, leaderboards?: LeaderboardForm[], state?: string) {
        this.name = name;
        this.mode = mode;
        this.min = min == "" ? null : min;
        this.max = max == "" ? null : max;
        this.leaderboards = leaderboards;
        this.state = state;
    }
}

class PlayerMeasure {
    stat: string;
    user: string;
    value: number;
}
