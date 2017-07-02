// Various helper functions.

// Gets the browser specific XmlHttpRequest Object
function getAjaxObject() {
	if (window.XMLHttpRequest) {
		return new XMLHttpRequest();
	} else if(window.ActiveXObject) {
		return new ActiveXObject("Microsoft.XMLHTTP");
	} else {
		return null;
	}
}

// Send a request on a specific reqobj.
function sendAjaxRequest(reqobj, url, handler) {
	if (reqobj.readyState != 0 && reqobj.readyState != 4) {
		return false;
	}
	reqobj.open("GET", url, true);
	reqobj.onreadystatechange = function () { if (reqobj.readyState == 4) { handler(reqobj); } };
	reqobj.send(null);
	return true;
}

// Send a json-specific request (requires a json parsing handler)
function sendJsonRequest(reqobj, url, successParser, failParser) {
	return sendAjaxRequest(reqobj, url, function (o) {
		if ( o.status == 200 ) {	// Success
      			successParser(eval("("+o.responseText+")"));
		} else {
			if ( failParser ) {
				failParser(o) ;
			}
		}
	}) ;
}

// Make a <td> element with some content.
function makeTD(content, styles) {
	var td = document.createElement("td");
	td.innerHTML = content ;
	if ( styles ) {
		for ( key in styles ) {
			td.style[key] = styles[key] ;
		}
	}
	return td;
}

// Make a <th> element with some content.
function makeTH(content, styles) {
	var td = document.createElement("th");
	td.innerHTML = content ;
	if ( styles ) {
		for ( key in styles ) {
			td.style[key] = styles[key] ;
		}
	}
	return td;
}

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
