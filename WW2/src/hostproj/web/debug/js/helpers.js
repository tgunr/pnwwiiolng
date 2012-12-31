// Various helper functions.

function twoDigits(i) {
 if ( i < 10 ) { return "0" + i ; }
 return i ;
}

function threeDigits(i) {
 if ( i < 10 ) { return "00" + i ; }
 if ( i < 100 ) { return "0" + i ; }
 return i ;
}

function makeDate(dateObj) {
 var ds = dateObj.toDateString() ;
 var hr = twoDigits(dateObj.getHours()) ;
 var mi = twoDigits(dateObj.getMinutes()) ;
 var sc = twoDigits(dateObj.getSeconds()) ;
 var mil = threeDigits(dateObj.getMilliseconds()) ;

 return datestr = ds + " " + hr + ":" + mi + ":" + sc + "." + mil ;
}
