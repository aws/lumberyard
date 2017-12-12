var TestAccounts = module.exports = {
  user: {
    username: "zzztestUser1" + makeId(),
    email: "cgp-integ-test" + makeId() + "@amazon.com",
    password: "Test01)!",
    newPassword: "Test02)@"
  },
  admin: {
    username: "administrator",
    password: "T3mp1@34"
  }
}

/*
 * Generate an ID to append to user names so multiple browsers running in parallel don't use the same user name.
*/
function makeId()
{
    var text = "";
    var possible = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";

    for( var i=0; i < 5; i++ )
        text += possible.charAt(Math.floor(Math.random() * possible.length));

    return text;
}