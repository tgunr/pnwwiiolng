// Functions for the index.html file.
function rxIndex(data) {
 var str = "" ;
 for ( var i = 0 ; i < data.length ; ++i ) {
  var row = data[i] ;
  str += "<tr class='l"+(i%2)+"'><td>"+row.desc+"</td><td><a href='"+row.url+"'>"+row.url+"</a></td></tr>" ;
 }
 $("#resource-list").html(str) ;
}

var files = [
  { desc:"Track memory usage", file:"memuse.html", host:"All" }
, { desc:"Monitor player counts", file:"player-balance.html", host:"Map" }
, { desc:"Cell/Instance loads", file:"inst-loads.html", host:"Map" }
, { desc:"Instance/Cell distribution", file:"inst-cells.html", host:"Map" }
, { desc:"Instance report", file:"inst-report.html", host:"Map" }
] ;

function rxStatus(data) {
 var types = [ 'Auth', 'Cell', 'Map', 'Chat' ] ;
 var str = "" ;
 var myHost = types[data.type] ;
 $('#html-list').html('') ;
 for ( var i = 0 ; i < files.length ; ++i ) {
  var file = files[i] ;
  var match = (files[i].host == "All" || files[i].host == myHost) ;

  var $desc = $('<td>').text(files[i].desc) ;
  var $link;
  if ( match )
    $link = $('<td>').html("<a href='/"+files[i].file+"'>"+files[i].file+"</a>") ;
  else
    $link = $('<td>').text(files[i].file) ;
  var $host = $('<td>').text(files[i].host) ;
  var $tr = $('<tr>').append($desc, $link, $host) ;
  if ( match == false )
    $tr.css('color', '#a0a0a0') ;
  $('#html-list').append($tr.addClass('l'+(i%2))) ;
 }
}

$(document).ready(function() {
 $.getJSON("/index.json", rxIndex) ;
 $.getJSON("/server-alive.json", rxStatus) ;
}) ;
