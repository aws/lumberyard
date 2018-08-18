import { AwsService } from 'app/aws/aws.service';

export class MetricQuery {

    private _directives: Directive[]
    private _table : string

    set tablename(value: string) {
        this._table = value
    }

    get directives(): Directive[]  {
        return this._directives
    }

    constructor(private aws: AwsService, private table: string = "", private tables: string[] = [], private supported_directives = []) {
        this._directives = supported_directives
        this._table = table
    }

    private template = (sql) => {
        for (let idx in this.directives) {
            let directive = this.directives[idx]
            sql = directive.construct(sql)           
        }
        var date = new Date();
        let projectName = this.aws.context.name.replace('-', '_').toLowerCase()
        let deploymentName = this.aws.context.project.activeDeployment.settings.name.replace('-', '_').toLowerCase()
        let table_prefix = `${projectName}_${deploymentName}_table_`
        let table = `${table_prefix}${this._table}`
        let database = `${projectName}_${deploymentName}`
        let table_clientinitcomplete = `${table_prefix}clientinitcomplete`
        let table_sessionstart = `${table_prefix}sessionstart`  
        let str = "let database = '\"" + database + "\"'; "
            + " let database_unquoted = '" + database + "'; "            
            + " let table = '\"" + table + "\"';"            
            + " let table_unquoted = '" + table + "'; "            
            + " let year = " + date.getUTCFullYear() + ";"
            + " let month = " + (date.getUTCMonth() + 1) + ";"
            + " let day = " + date.getDate() + ";"
            + " let hour = " + date.getHours() + ";"            
            + " let table_clientinitcomplete = '\"" + table_clientinitcomplete + "\"';"
            + " let table_sessionstart = '\"" + table_sessionstart + "\"';"            
            + " return `" + sql.replace(/[\r\n]/g, ' ') + "`;"             
        return new Function(    str        )()
    }

    public toString = (sql: string) => {
        return this.template(sql)
    }
   
}


export abstract class Directive {
    protected parser: MetricQuery

    constructor(protected aws: AwsService) {
        this.parser = new MetricQuery(this.aws)
    }

    abstract construct(template: string): string     
}

export class AllTablesUnion extends Directive{

    constructor(protected aws: AwsService, private tables: string[]) {
        super(aws)
    }

    public construct(template: string): string {        
        let pattern = /<ALL_TABLES_UNION>[\s\S]*<\/ALL_TABLES_UNION>/g
        let match = pattern.exec(template)
        if (match == null || match.length == 0)
            return template

        let content = ""
        for (let idz = 0; idz < match.length; idz++) {
            let content_match = match[idz]
            let inner_template = content_match.replace("<ALL_TABLES_UNION>", "").replace("</ALL_TABLES_UNION>", "")        
            let inner_content = ""         
            for (let idx = 0; idx < this.tables.length; idx++) {
                let table = this.tables[idx]
                this.parser.tablename = table                
                inner_content = inner_content + " " + this.parser.toString(inner_template)
                if (idx < this.tables.length - 1) 
                    inner_content = inner_content + " UNION "                
            }
            content = template.replace(content_match, inner_content)
        }        
        return content
    }
}
