<?php

if (PHP_VERSION_ID < 70100) {
    die("PHP 7.1 or higher is required.\n");
}
if (!extension_loaded('xmlrpc')) {
    die("XML-RPC extension is required.\n");
}
if (!extension_loaded('curl')) {
    die("libcurl extension is required.\n");
}

$billing_login_endpoint = 'http://billing.playnet.com:9000';
$auth_otp_endpoint = 'http://localhost:6081/v1/cluster/{arena_id}/dispatch';

function RESTfail(int $status, string $message): void {
    header("Content-Type: application/json", true, $status);
    print json_encode(['result' => 3, "error" => $message]);
    exit(0);
}

// Validate inputs
$content = file_get_contents("php://input");
if (!is_object($data = @json_decode($content, false, 512, JSON_BIGINT_AS_STRING))) {
    RESTfail(400, "Bad input");
}
if (!isset($data->steam_ticket) || !is_string($data->steam_ticket) ||
    !isset($data->arena_id) || !is_integer($data->arena_id) ||
    !isset($data->app_version) || !is_integer($data->app_version) ||
    !isset($data->app_platform) || !is_integer($data->app_platform) ||
    (isset($data->callsign) && !is_string($data->callsign))
) {
    RESTfail(400, "Bad input");
}

$steam_ticket = filter_var($data->steam_ticket, FILTER_VALIDATE_REGEXP, ['options' => ['regexp' => '/^([a-zA-Z0-9]+)$/']]);
$arena_id = filter_var($data->arena_id, FILTER_VALIDATE_INT);
$app_version = filter_var($data->app_version, FILTER_VALIDATE_INT);
$app_platform = filter_var($data->app_platform, FILTER_VALIDATE_INT, ['options' => ['min_range' => 0, 'max_range' => 2]]);
if (isset($data->callsign)) {
    $callsign = filter_var($data->callsign, FILTER_VALIDATE_REGEXP, ['options' => ['regexp' => '/^[a-z][a-z0-9]{1,9}$/']]);
}

if (!is_string($steam_ticket) || !is_integer($arena_id) ||
    !is_integer($app_version) || !is_integer($app_platform) ||
    (isset($data->callsign) && !is_string($callsign))
) {
    RESTfail(400, "Bad input");
}

// Do billing XML-RPC here

$auth_data = [
    'user_id' => 0,
    'app_version' => $app_version,
    'app_platform' => $app_platform,
];
$auth_url = str_replace('{arena_id}', $arena_id, $auth_otp_endpoint);
$auth_handle = curl_init($auth_url);
if (!curl_setopt_array($auth_handle, [
    CURLOPT_CUSTOMREQUEST => 'POST',
    CURLOPT_POSTFIELDS => json_encode($auth_data),
    CURLOPT_HTTPHEADER => ['Content-Type: application/json'],
    CURLOPT_RETURNTRANSFER => true,
])) {
    RESTfail(500, "Internal error 1");
}
if (($auth_response_raw = curl_exec($auth_handle)) === false) {
    RESTfail(500, "Internal error 2");
}
if (($auth_response_code = curl_getinfo($auth_handle, CURLINFO_HTTP_CODE)) !== 200) {
    RESTfail($auth_response_code, $auth_response_raw);
}
if (!is_object($auth_response = @json_decode($auth_response_raw, false, 512, JSON_BIGINT_AS_STRING))) {
    RESTfail(500, "Internal error 3");
}
if (!isset($auth_response->otp) || !is_integer($auth_response->otp) ||
    !isset($auth_response->serverIP) || !is_integer($auth_response->serverIP) ||
    !isset($auth_response->serverPort) || !is_integer($auth_response->serverPort))
{
    RESTfail(500, "Internal error 4");
}

header('Content-Type: application/json');
print json_encode([
    'result' => 0,
    'arenaID' => $arena_id,
    'otp' => $auth_response->otp,
    'serverIP' => $auth_response->serverIP,
    'serverPort' => $auth_response->serverPort,
]);

?>

