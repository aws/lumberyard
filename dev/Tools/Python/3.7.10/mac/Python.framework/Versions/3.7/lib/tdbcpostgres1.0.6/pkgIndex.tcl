# Package index file for tdbc::postgres

if {[catch {package require Tcl 8.6}]} {
    return
}
package ifneeded tdbc::postgres 1.0.6 \
    "[list source [file join $dir tdbcpostgres.tcl]]\;\
    [list load [file join $dir libtdbcpostgres1.0.6.dylib] tdbcpostgres]"
