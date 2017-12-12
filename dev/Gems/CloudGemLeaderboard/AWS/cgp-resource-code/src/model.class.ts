//View models
export class PageModel {
    public users: string[]
    public page: number
    public page_size: number
}

export class LeaderboardForm {
    name?: string;
    mode?: string;
    min?: number;
    max?: number;
    leaderboards?: LeaderboardForm[]
    state?: string;
    additional_data?: PageModel;
    numberOfPages?: number;
    sample_size: number;

    constructor(name?: string, mode?: string, min?: string, max?: string, leaderboards?: LeaderboardForm[], state?: string, sample_size?: string) {
        this.name = name;
        this.mode = mode;
        this.min = min == "" || min === undefined || min === null ? undefined : Number(min);
        this.max = max == "" || max === undefined || max === null ? undefined : Number(max);
        this.leaderboards = leaderboards;
        this.state = state;
        this.sample_size = sample_size == "" || sample_size === undefined || sample_size === null ? undefined : Number(sample_size);
    }
}

export class PlayerMeasure {
    stat: string;
    user: string;
    value: number;
}
//end view models