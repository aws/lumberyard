export class BotEntry {
    bot_name: string;
    current_version: string;
    status: string;
    updated: string;
    created: string;

    constructor(botInfo: any) {
        this.bot_name = botInfo.bot_name;
        this.current_version = botInfo.current_version;
        this.status = botInfo.status;
        this.updated = botInfo.updated;
        this.created = botInfo.created;
    }
}