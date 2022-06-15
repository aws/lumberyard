# Package index file for tdbc::mysql

if {[catch {package require Tcl 8.6}]} {
    return
}
package ifneeded tdbc::mysql 1.0.6 \
    "[list source [file join $dir tdbcmysql.tcl]]\;\
    [list load [file join $dir libtdbcmysql1.0.6.dylib] tdbcmysql]"
