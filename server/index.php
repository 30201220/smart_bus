<?php
require 'vendor/autoload.php';

use GuzzleHttp\Client;

header('Content-Type: application/json; charset=utf-8');
echo '[{"RouteUID":"KHH28","RouteName":{"Zh_tw":"28","En":"28 "},"Direction":0,"StopStatus":0,"EstimateTime":120,"SrcUpdateTime":"2021-04-04T13:08:55+08:00","UpdateTime":"2021-04-04T13:09:15+08:00"}]';
exit;

$StationUID='KHH00162';

$PTX_URL='http://ptx.transportdata.tw/MOTC/v2/Bus/';
//$PTX_ID='e0f916de793847d28b51a876c4ac8fe9';
$PTX_ID='2ab65e55a3364983844f916e7a954fa4';
//$PTX_KEY='cH3sah4kL9Xhs-_C8x-1VUTlebk';
$PTX_KEY='pFx5hz9PFDMf5eGuqdhQbJTfWtE';
$PTX_CITY='Kaohsiung';

$time_string = gmdate('D, d M Y H:i:s') . ' GMT';
$signature = base64_encode(hash_hmac(   "sha1", "x-date: $time_string", $PTX_KEY, true));

$client = new Client([
    'base_uri' => $PTX_URL,
	'headers' => [
        'Authorization' => 'hmac username="' . $PTX_ID . '", algorithm="hmac-sha1", headers="x-date", signature="' . $signature.'"',
		'x-date'=>$time_string,
        'Accept'=> 'application/json',
        'Accept-Encoding'=> 'gzip'
    ]
]);

$response = $client->request('GET', 'Station/City/'.$PTX_CITY,[
	'query'=>[
		'$format'=>'JSON',
		'$filter'=>"StationUID eq '".$StationUID."'",
		'$select'=>'StationUID,StationName,Stops',
	],
]);

$station=json_decode($response->getBody(),true)[0];
$stopUIDs=array();
foreach($station['Stops'] as $stop){
	$stopUIDs[]=$stop['StopUID'];
}
$stopUIDs=array_unique($stopUIDs);

$N1Filter="";
foreach($stopUIDs as $stopUID){
	$N1Filter.="(StopUID eq '".$stopUID."')or";
}
$N1Filter=substr($N1Filter, 0, -2);

$response = $client->request('GET', 'EstimatedTimeOfArrival/City/'.$PTX_CITY,[
	'query'=>[
		'$format'=>'JSON',
		'$filter'=>$N1Filter,
		'$select'=>'StopUID,RouteUID,Direction,EstimateTime,StopStatus,Estimates,RouteName',
	],
]);
//echo $response->getStatusCode();
//echo $response->getBody();
$N1Data=json_decode($response->getBody(),true);

$output=array();
foreach($N1Data as $N1){
	if($N1['StopStatus']==0){
		$output[]=array(
			'RouteUID'=>$N1['RouteUID'],
			'RouteName'=>$N1['RouteName'],
			'Direction'=>$N1['Direction'],
			'StopStatus'=>$N1['StopStatus'],
			'EstimateTime'=>$N1['EstimateTime'],
			'SrcUpdateTime'=>$N1['SrcUpdateTime'],
			'UpdateTime'=>$N1['UpdateTime']);
	}
}


//var_dump($stops);


header('Content-Type: application/json; charset=utf-8');
echo json_encode($output);


?>
