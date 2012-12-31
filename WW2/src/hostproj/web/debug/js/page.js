var pageServerInfoUrl = "/server-alive.json" ;
var pageServerInfoTimer = null ;
function getPageServerInfo() {
    clearTimeout(pageServerInfoTimer) ;
    $('.host-page-info').css('color', 'gray') ;
    $.getJSON(pageServerInfoUrl).success(function(data) {
     $('.host-page-info').css('color', 'black') ;
     var values = { 'pagejs-app' : data.app.toUpperCase(), 'pagejs-ips' : data.ip, 'pagejs-src' : data.src_rev } ;
     for ( var i in values )
     {
       $('#'+i).show().html(values[i]) ;
     }
     pageServerInfoTimer = setTimeout('getPageServerInfo()', 60 * 1000) ;
    }).error(function() {
     $('.host-page-info').css('color', 'black') ;
     $('#pagejs-app').html('<small><i>Server&nbsp;not&nbsp;responding</i></small>') ;
     $('#pagejs-ips').html("").hide() ;
     $('#pagejs-src').html("").hide() ;
     pageServerInfoTimer = setTimeout('getPageServerInfo()', 10 * 1000) ;
    }) ;
}
setTimeout(function () {
	var headings =
	[
		{ 'id' : 'home-link', 'class' : 'home-link', 'html' : "<a href='/'>[Index]</a>" }
	,	{ 'id' : 'pagejs-heading', 'class' : 'heading', 'html' : document.title }
	,	{ 'id' : 'pagejs-app', 'class' : 'host-page-info' }
	,	{ 'id' : 'pagejs-ips', 'class' : 'host-page-info' }
	,	{ 'id' : 'pagejs-src', 'class' : 'host-page-info' }
	] ;
	var $headline = $('<div>').addClass("page-heading") ;
	for ( var i in headings )
	{
		var heading = headings[i] ;
		var $div = $('<div id="'+heading.id+'" class="'+heading['class']+'">') ;
		if ( heading.html )
			$div.html(heading.html) ;
		$headline.append($div) ;
	}
	$(document.body).prepend($headline) ;
    getPageServerInfo() ;
}, 1) ;
