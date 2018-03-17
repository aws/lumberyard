import { ViewChild, Input, Component, ViewContainerRef } from '@angular/core';
import { FormControl } from '@angular/forms';
import { DateTimeUtil, StringUtil } from 'app/shared/class/index';
import { ModalComponent } from 'app/shared/component/index';
import { ActionItem } from "app/view/game/module/shared/class/index";
import { Subscription } from 'rxjs/Subscription';
import { SearchResult } from 'app/view/game/module/shared/component/index';
import { PageModel, LeaderboardForm, PlayerMeasure, LeaderboardApi} from './index'
import { Http } from '@angular/http';
import { AwsService } from "app/aws/aws.service";
import { PaginationComponent } from 'app/view/game/module/shared/component/index';
import { CacheHandlerService } from 'app/view/game/module/cloudgems/service/cachehandler.service';
import { AbstractCloudGemIndexComponent } from 'app/view/game/module/cloudgems/class/index';
import { ToastsManager } from 'ng2-toastr';
import { LyMetricService } from 'app/shared/service/index';

const CACHE_TIME_LIMIT_IN_SECONDS = 300; // 5 minute

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

@Component({
    selector: 'leaderboard-index',
    templateUrl: "node_modules/@cloud-gems/cloudgemleaderboard/index.component.html",
    styleUrls: ["node_modules/@cloud-gems/cloudgemleaderboard/index.component.css"]
})
export class LeaderboardIndexComponent extends AbstractCloudGemIndexComponent {
    @Input() context: any;    
    private _apiHandler: LeaderboardApi;
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

    constructor(private http: Http, private aws: AwsService, private cache: CacheHandlerService, private toastr: ToastsManager, vcr: ViewContainerRef, private metric: LyMetricService) {
        super()
        this.toastr.setRootViewContainerRef(vcr);
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
        this._apiHandler = new LeaderboardApi(this.context.ServiceUrl, this.http, this.aws, this.metric, this.context.identifier);

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
            this._apiHandler.searchLeaderboard(this.currentLeaderboard.name, searchResult.value)
                .subscribe(response => {
                    let obj = JSON.parse(response.body.text());
                    let searchResults = obj.result.scores;
                    this.currentScores = searchResults;
                    this.currentScores = this.checkLocalScoreCacheForChanges();
                    this.isLoadingScores = false;
                }, err => {
                    this.toastr.error(err);
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
            id: {
                valid: true
            },
            min: {
                valid: true
            },
            max: {
                valid: true
            }, 
            sample_size: {
                valid: true
            }
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
        } else if (!split[0].toString().match(/^[0-9a-zA-Z]+$/)) {
            isValid = false;
            model.validation.id = {
                valid: false,
                message: "The leaderboard identifier can not contains non-alphanumeric characters"
            }
        }
        else {
            split[0] = split[0].trim();
        }

        this.setNumberValidationObject(model, "max");
        this.setNumberValidationObject(model, "min");
        this.setNumberValidationObject(model, "sample_size");      

        if (model.validation.max.valid && model.validation.min.valid) {
            let max = Number(model.max);
            let min = Number(model.min);
            if (min > max) {
                model.validation.max.valid = false;
                model.validation.min.valid = false;
                model.validation.min.message = "The minimum reportable value must be a greater than the maximum reportable value."                
            }
        }
        isValid = model.validation.max.valid && model.validation.min.valid && model.validation.sample_size.valid && model.validation.id.valid;       
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
        this.currentLeaderboard = new LeaderboardForm(leaderboard.name,
            leaderboard.mode,
            leaderboard.min,
            leaderboard.max,
            this.leaderboards,
            this.mode.toString(),
            leaderboard.sample_size);        
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
        this.getScoresSub = this._apiHandler.getEntries(leaderboard.name, leaderboard.additional_data).subscribe(response => {
            let json = JSON.parse(response.body.text());
            this.currentScores = json.result.scores;
            leaderboard.numberOfPages = json.result.total_pages;
            this.currentScores = this.checkLocalScoreCacheForChanges();
            this.isLoadingScores = false;
        }, err => {
            this.toastr.error("The leaderboard with id '" + leaderboard.name + "' failed to retrieve its data: " + err);
            this.isLoadingScores = false;
        });
    }

    public list(): void {
        this.isLoadingLeaderboards = true;
        this.mode = LeaderboardMode.List;
        this.currentScores = null;
        this._apiHandler.get("stats").subscribe(response => {
            let obj = JSON.parse(response.body.text());
            this.leaderboards = obj.result.stats;
            this.isLoadingLeaderboards = false;
        }, err => {
            this.toastr.error("The leaderboards did not refresh properly: " + err);
            this.isLoadingLeaderboards = false;
        });
    }

    // Functions inside modals
    public addLeaderboard(model): void {
        if (this.validate(model)) {
            var body = this.getPostObject();
            this.modalRef.close();
            this._apiHandler.addLeaderboard(body).subscribe(response => {
                this.toastr.success("The leaderboard with id '" + this.currentLeaderboard.name + "' was created.");
                this.currentLeaderboard = new LeaderboardForm("");
                this.list();
                this.mode = LeaderboardMode.List;
            }, err => {
                this.toastr.error("The leaderboard could not be created: " + err);
                this.list();
            });
        }
    }

    public editLeaderboard(model): void {
        if (this.validate(model)) {
            var body = this.getPostObject();
            this.modalRef.close();
            this._apiHandler.editLeaderboard(body).subscribe(response => {
                this.toastr.success("The leaderboard with id '" + this.currentLeaderboard.name + "' was updated.");
                this.list();
                this.mode = LeaderboardMode.List;
            }, err => {
                this.toastr.error("The leaderboard could not be updated: " + err);
                this.list();
            });
        }
    }

    public deleteLeaderboard(): void {
        this.modalRef.close();
        this._apiHandler.deleteLeaderboard(this.currentLeaderboard.name).subscribe(response => {
            this.toastr.success("The leaderboard with id '" + this.currentLeaderboard.name + "' has been deleted.");
            this.list();
            this.mode = LeaderboardMode.List;
        }, err => {
            this.toastr.error("The leaderboard could not be deleted: " + err);
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
        this._apiHandler.deleteScore(this.currentLeaderboard.name, this.currentScore.user).subscribe(response => {
            this.toastr.success("The score for player '" + this.currentScore.user + "' was deleted successfully.");
            this.cache.setObject(this.currentScore, this.currentScore);
            this.currentScores = this.checkLocalScoreCacheForChanges();
            this.isLoadingScores = false;
            this.playerMode = PlayerMode.List;
        }, err => {
            this.playerMode = PlayerMode.List;
            this.toastr.error("The score for could not be deleted: " + err);
        });
    }

    public getBannedPlayers(): void {
        this.isLoadingBannedPlayers = true;
        this._apiHandler.get("player/ban_list").subscribe(response => {
            this.isLoadingBannedPlayers = false;
            let obj = JSON.parse(response.body.text());
            this.bannedPlayers = obj.result.players;
        }, err => {
            this.isLoadingBannedPlayers = false;
            this.toastr.error("Unable to get the list of banned players: " + err);
        });
    }

    public banPlayerModal(score): void {
        this.playerMode = PlayerMode.BanUser;
        this.currentScore = score;
    }

    public banPlayer(): void {
        this.isLoadingScores = true;
        this.modalRef.close();
        this._apiHandler.banPlayer(this.currentScore.user).subscribe(response => {
            let obj = JSON.parse(response.body.text());
            this.toastr.success("The player '" + this.currentScore.user + "' has been banned.  The score will remain until deleted.");            
            this.isLoadingScores = false;
            this.playerMode = PlayerMode.List;
        }, err => {
            this.playerMode = PlayerMode.List;
            this.toastr.error("The player could not be banned: " + err);
        });
    }

    public unbanPlayerModal(model): void {
        this.currentPlayer = model;
        this.mode = LeaderboardMode.Unban;
    }

    public unbanPlayer(): void {
        this.modalRef.close();
        this._apiHandler.unbanPlayer(this.currentPlayer).subscribe(response => {
            let obj = JSON.parse(response.body.text());
            this.toastr.success("The player '" + this.currentPlayer + "' has had their ban lifted");            
            this.cache.remove(this.currentPlayer);
            this.getBannedPlayers();
            this.mode = LeaderboardMode.List;
        }, err => {
            this.mode = LeaderboardMode.List;
            this.toastr.error("The ban could not be removed: " + err);
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
        this._apiHandler.processRecords().subscribe(response => {
            this.toastr.success("The leaderboard processor has been started. Processing should only take a few moments.");
        }, err => {
            this.toastr.error("The leaderboard process failed to start" + err);
        });
    }

    private checkLocalScoreCacheForChanges(): PlayerMeasure[] {
        var currentCachedScores: PlayerMeasure[] = this.currentScores;
        var indexBlacklist = Array<number>();

        for (var i = 0; i < this.currentScores.length; i++) {
            let isScoreDeleted = this.cache.objectExists(this.currentScores[i]);
            let isPlayerBanned = this.cache.exists(this.currentScores[i].user);
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
        return new LeaderboardForm(
            this.currentLeaderboard.name,
            this.currentLeaderboard.mode.toUpperCase(),
            this.currentLeaderboard.min === null || this.currentLeaderboard.min === undefined ? undefined : this.currentLeaderboard.min.toString(),
            this.currentLeaderboard.max === null || this.currentLeaderboard.max === undefined ? undefined : this.currentLeaderboard.max.toString(),
            undefined,
            undefined,
            this.currentLeaderboard.sample_size === null || this.currentLeaderboard.sample_size === undefined ? undefined : this.currentLeaderboard.sample_size.toString());
    }

    private isValidNumber(value : string, lowerbound: number, upperbound : number = undefined): boolean {
        if (value === "" || value === null || value === undefined)
            return true
        let number = Number(value)
        if (number === NaN)
            return false

        return lowerbound && upperbound ? number >= lowerbound && number <= upperbound : number >= lowerbound
    }

    private setNumberValidationObject(object, attr) {
        object[attr] = object[attr] === "" || object[attr] === null ? undefined : object[attr];
        object.validation[attr] = this.isValidNumber(object[attr], 0);
        object.validation[attr] = {
            message: object.validation[attr] ? undefined : "This value must be numeric and greater than zero.",
            valid: object.validation[attr]
        };
    }
}
