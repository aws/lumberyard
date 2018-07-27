export class Queries {

    /**
    * Contains query for Athena that defines the defects generated in the
    * last X hours, where X is the passed in parameter.
    * @param hour Number that represents the number of hours to count back from.
    * @returns String query ready to be passed into the MetricQuery class. Note that MetricQuery class
    * will replace \${database} and \${table} with the appropriate values.
    **/
    public static lastXHours(hour:number): string {

        return `SELECT date_format(from_unixtime(server_timestamp), '%m-%d %Hh') as Hour, count(distinct universal_unique_identifier) AS count 
            FROM \${database}.\${table}
            WHERE p_server_timestamp_strftime >= date_format((current_timestamp - interval '` + String(hour) + `' hour), '%Y%m%d%H0000')  
            GROUP BY date_format(from_unixtime(server_timestamp), '%m-%d %Hh') 
            ORDER BY date_format(from_unixtime(server_timestamp), '%m-%d %Hh') ASC`;
    } 

    /**
    * Contains query for Athena that defines the defects generated in the
    * last X days, where X is the passed in parameter.
    * @param day Number that represents the number of days to count back from.
    * @returns String query ready to be passed into the MetricQuery class. Note that MetricQuery class
    * will replace \${database} and \${table} with the appropriate values.
    **/    
    public static lastXDays(day:number): string {

        return `SELECT date_format(from_unixtime(server_timestamp), '%Y-%m-%d') as Day, count(distinct universal_unique_identifier) AS count 
            FROM \${database}.\${table}
            WHERE p_server_timestamp_strftime >= date_format((current_timestamp - interval '`+ String(day) +`' day), '%Y%m%d%H0000') 
            GROUP BY date_format(from_unixtime(server_timestamp), '%Y-%m-%d') 
            ORDER BY date_format(from_unixtime(server_timestamp), '%Y-%m-%d') ASC`;
    } 

    /**
    * Contains query for Athena that defines the defects generated in the    
    * last X months, where X is the passed in parameter.
    * @param month Number that represents the number of months to count back from.
    * @returns String query ready to be passed into the MetricQuery class. Note that MetricQuery class
    * will replace \${database} and \${table} with the appropriate values.
    **/    
    public static lastXMonths(month:number): string {

        return `SELECT date_format(from_unixtime(server_timestamp), '%Y-%m') as Month, count(distinct universal_unique_identifier) AS count 
            FROM \${database}.\${table}
            WHERE p_server_timestamp_strftime >= date_format((current_timestamp - interval '`+ String(month) +`' month), '%Y%m%d%H0000') 
            GROUP BY date_format(from_unixtime(server_timestamp), '%Y-%m')
            ORDER BY date_format(from_unixtime(server_timestamp), '%Y-%m') ASC`;
    }      

}