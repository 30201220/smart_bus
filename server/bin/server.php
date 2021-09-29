<?php

require dirname(__DIR__) . '/vendor/autoload.php';

use Ratchet\Server\IoServer;
use Ratchet\Http\HttpServer;
use Ratchet\WebSocket\WsServer;
use MyApp\MessageHandler;
use GuzzleHttp\Client;

$port = 8888;
$handler = new MessageHandler();
$periodic = 5;

$PTX_URL = 'http://ptx.transportdata.tw/MOTC/v2/Bus/';
//$PTX_ID='e0f916de793847d28b51a876c4ac8fe9';
$PTX_ID = '2ab65e55a3364983844f916e7a954fa4';
//$PTX_KEY='cH3sah4kL9Xhs-_C8x-1VUTlebk';
$PTX_KEY = 'pFx5hz9PFDMf5eGuqdhQbJTfWtE';
$PTX_CITY = 'Kaohsiung';

$mockMode = true;
$mockData =
    '[
      {
        "RouteUID": "KHH28",
        "RouteName": {
          "Zh_tw": "28",
          "En": "28 "
        },
        "Direction": 0,
        "StopStatus": 0,
        "EstimateTime": 120,
        "SrcUpdateTime": "2021-04-04T13:08:55+08:00",
        "UpdateTime": "2021-04-04T13:09:15+08:00"
      },
      {
        "RouteUID": "KHH532",
        "RouteName": {
          "Zh_tw": "53B",
          "En": "53B"
        },
        "Direction": 0,
        "StopStatus": 0,
        "EstimateTime": 1680,
        "SrcUpdateTime": "2021-04-04T13:08:55+08:00",
        "UpdateTime": "2021-04-04T13:09:15+08:00"
      },
      {
        "RouteUID": "KHH8023",
        "RouteName": {
          "Zh_tw": "8023",
          "En": "8023 "
        },
        "Direction": 1,
        "StopStatus": 0,
        "EstimateTime": 2580,
        "SrcUpdateTime": "2021-04-04T13:08:55+08:00",
        "UpdateTime": "2021-04-04T13:09:15+08:00"
      },
      {
        "RouteUID": "KHH8025",
        "RouteName": {
          "Zh_tw": "E25高旗六龜快線(08:20前不行經高鐵左營站)",
          "En": "E25 GaoQi Liouguei Express "
        },
        "Direction": 0,
        "StopStatus": 0,
        "EstimateTime": 1800,
        "SrcUpdateTime": "2021-04-04T13:08:55+08:00",
        "UpdateTime": "2021-04-04T13:09:15+08:00"
      },
      {
        "RouteUID": "KHH8032",
        "RouteName": {
          "Zh_tw": "E32高旗甲仙快線",
          "En": "E32 GaoQi Jiasian Express"
        },
        "Direction": 0,
        "StopStatus": 0,
        "EstimateTime": 2880,
        "SrcUpdateTime": "2021-04-04T13:08:55+08:00",
        "UpdateTime": "2021-04-04T13:09:15+08:00"
      },
      {
        "RouteUID": "KHH829",
        "RouteName": {
          "Zh_tw": "紅29",
          "En": "Red29 "
        },
        "Direction": 1,
        "StopStatus": 0,
        "EstimateTime": 120,
        "SrcUpdateTime": "2021-04-04T13:08:55+08:00",
        "UpdateTime": "2021-04-04T13:09:15+08:00"
      },
      {
        "RouteUID": "KHH92",
        "RouteName": {
          "Zh_tw": "92自由幹線",
          "En": "92 Zihyou Main Line"
        },
        "Direction": 0,
        "StopStatus": 0,
        "EstimateTime": 0,
        "SrcUpdateTime": "2021-04-04T13:08:55+08:00",
        "UpdateTime": "2021-04-04T13:09:15+08:00"
      }
    ]';


$server = IoServer::factory(
    new HttpServer(
        new WsServer(
            $handler
        )
    ),
    $port
);

$server->loop->addPeriodicTimer($periodic, function () use ($handler, $PTX_URL, $PTX_ID, $PTX_KEY, $PTX_CITY, $mockMode, $mockData) {
    $time_string = gmdate('D, d M Y H:i:s') . ' GMT';
    $signature = base64_encode(hash_hmac("sha1", "x-date: $time_string", $PTX_KEY, true));

    $httpClient = new Client([
        'base_uri' => $PTX_URL,
        'headers' => [
            'Authorization' => 'hmac username="' . $PTX_ID . '", algorithm="hmac-sha1", headers="x-date", signature="' . $signature . '"',
            'x-date' => $time_string,
            'Accept' => 'application/json',
            'Accept-Encoding' => 'gzip'
        ]
    ]);

    foreach ($handler->clients as $client) {
        echo "Client ID: {$client->resourceId} Station UID: {$handler->stationBind[$client->resourceId]}\n";
        if ($mockMode) {
            $client->send(json_encode(json_decode($mockData, true)));
            continue;
        }
        if (isset($handler->stationBind[$client->resourceId])) {
            $httpResponse = $httpClient->request('GET', 'Station/City/' . $PTX_CITY, [
                'query' => [
                    '$format' => 'JSON',
                    '$filter' => "StationUID eq '" . $handler->stationBind[$client->resourceId] . "'",
                    '$select' => 'StationUID,StationName,Stops',
                ],
            ]);

            $station = json_decode($httpResponse->getBody(), true)[0];
            $stopUIDs = array();
            foreach ($station['Stops'] as $stop) {
                $stopUIDs[] = $stop['StopUID'];
            }
            $stopUIDs = array_unique($stopUIDs);

            $N1Filter = "";
            foreach ($stopUIDs as $stopUID) {
                $N1Filter .= "(StopUID eq '" . $stopUID . "')or";
            }
            $N1Filter = substr($N1Filter, 0, -2);

            $httpResponse = $httpClient->request('GET', 'EstimatedTimeOfArrival/City/' . $PTX_CITY, [
                'query' => [
                    '$format' => 'JSON',
                    '$filter' => $N1Filter,
                    '$select' => 'StopUID,RouteUID,Direction,EstimateTime,StopStatus,Estimates,RouteName',
                ],
            ]);

            $N1Data = json_decode($httpResponse->getBody(), true);

            $output = array();
            foreach ($N1Data as $N1) {
                if ($N1['StopStatus'] == 0) {
                    $output[] = array(
                        'RouteUID' => $N1['RouteUID'],
                        'RouteName' => $N1['RouteName'],
                        'Direction' => $N1['Direction'],
                        'StopStatus' => $N1['StopStatus'],
                        'EstimateTime' => $N1['EstimateTime'],
                        'SrcUpdateTime' => $N1['SrcUpdateTime'],
                        'UpdateTime' => $N1['UpdateTime']);
                }
            }

            $client->send(json_encode($output));
        }
    }
    //echo ".";
});

echo "WebSocketServer Started\n";
$server->run();
