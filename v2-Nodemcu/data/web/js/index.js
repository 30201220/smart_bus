//var HOST = location.host;
var HOST = "http://192.168.55.2";
var conn;
var user

function init() {
  if ($.cookie('user') != undefined)
    user = $.cookie('user');
  else
    getUserHash();
  console.log(user);

  renderBusList();
  setInterval(renderBusList, 5000);
  getBooks();
}

function renderBusList() {
  fetch(HOST + '/getBusList')
    .then(function (response) {
      return response.json()
    }).then(function (data) {
    console.log(data);
    $('#busList').html('');
    data.forEach(function (element) {
      $('#busList').append(
        '<div class="card">' +
        '<div class="card-content">' +
        '<div class="content">' +
        '<div class="columns">' +
        '<div class="column">' +
        //'<li>路線UID：'+element['RouteUID']+'</li>' +
        '<li>路線名稱：' + element['RouteName']['Zh_tw'] + '</li>' +
        //'<li>去返程：'+(element['Direction']==0?'去程':'返程')+'</li>' +
        //'<li>狀態：'+(element['StopStatus']==0?'正常':'其他')+'</li>' +
        '<li>預計到達：' + element['EstimateTime'] / 60 + ' 分</li>' +
        //'<li>更新時間：'+element['UpdateTime']+'</li>' +
        '</div>' +
        '<div class="column">' +
        '<button class="button is-primary" id="btn_bus_'+element['RouteUID']+'" onclick="newBooks(\''+element['RouteUID']+'\')">預約</button>' +
        '</div>' +
        '</div>' +
        '</div>' +
        '</div>' +
        '</div>'
      );
    });
    getBooks();
  }).catch(function (err) {

  })
}

function getUserHash() {
  var hash = md5(Math.random()).substr(0, 6);
  fetch(HOST + '/register?hash=' + hash)
    .then(function (response) {
      return response.text();
    }).then(function (data) {
    console.log(data);
    if (data == "HASH-EXIST") {
      getUserHash();
      return;
    }
    $.cookie('user', hash, {expires: 7});
    user = hash;
  }).catch(function (err) {

  })
}

function getBooks(){
  fetch(HOST + '/getBooks?user='+user)
    .then(function (response) {
      return response.text();
    }).then(function (data) {
    console.log(data);
    $("button[id^='btn_bus_']").removeClass('is-danger');
    $("button[id^='btn_bus_']").addClass('is-primary');
    $("button[id^='btn_bus_']").text('預約');
    if(data!='null'){
      $('#btn_bus_'+data).removeClass('is-primary');
      $('#btn_bus_'+data).addClass('is-danger');
      $('#btn_bus_'+data).text('已預約');
    }
  }).catch(function (err) {

  })
}

function newBooks(UID){
  console.log($('#btn_bus_'+UID).hasClass('is-danger'))
  if($('#btn_bus_'+UID).hasClass('is-danger'))
    UID='null';
  fetch(HOST + '/newBooks?user='+user+'&bus='+UID)
    .then(function (response) {
      return response.text();
    }).then(function (data) {
    console.log(data);
    getBooks();
  }).catch(function (err) {

  })
}

function webSocketTest() {
  /*conn = new WebSocket('ws://localhost:8888');
  conn.onopen = function(e) {
      conn.send('KHH00162');
      console.log("Connection established!");
  };
  conn.onmessage = function(e) {
      console.log(e.data);
  };*/
}