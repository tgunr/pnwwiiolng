<?php

// Implementation note: Using die() is effectively an assertion failure. The user will get
// no response from the server and the error will be visible in Apache's logs instead. It
// means something's gone seriously wrong!

if (PHP_VERSION_ID < 70100) {
    die("PHP 7.1 or higher is required.\n");
}
if (!extension_loaded('xmlrpc')) {
    die("XML-RPC extension is required.\n");
}
if (!extension_loaded('curl')) {
    die("libcurl extension is required.\n");
}

// Dirty evil trick: A cluster's environment.cfg can be parsed as an INI file if you hack
// the # comments into ; comments.
if (!isset($_SERVER['PN_INSTALL_PATH'])) {
    die("Installation path configuration is required.\n");
}
$environment_cfg_path = realpath(implode(DIRECTORY_SEPARATOR,
    [$_SERVER['PN_INSTALL_PATH'], 'wwiiol', 'bin', 'environment.cfg']
));
if (!is_string($environment_cfg_raw = @file_get_contents($environment_cfg_path))) {
    die("Unable to read environment configuration.\n");
}
$environment_cfg_fixed = preg_replace('/^#/smu', ';', $environment_cfg_raw);
if (!is_array($environment_cfg = parse_ini_string($environment_cfg_fixed, false, INI_SCANNER_RAW))) {
    die("Unable to parse environment configuration.\n");
}
if (!isset($environment_cfg['AUTH_RELAY_BILLING_ENDPOINT'])) {
    die("No billing relay endpoint found in environment configuration.\n");
}

$billing_login_endpoint = $environment_cfg['AUTH_RELAY_BILLING_ENDPOINT'];
$billing_xmlrpc_method = 'steam.requestLogin';
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

$billing_request_data = [$steam_ticket, $arena_id];
if (isset($callsign)) {
    $billing_request_data[] = $callsign;
}
$billing_request = xmlrpc_encode_request($billing_xmlrpc_method, $billing_request_data);
$billing_handle = curl_init($billing_login_endpoint);
if (!curl_setopt_array($billing_handle, [
    CURLOPT_CUSTOMREQUEST => 'POST',
    CURLOPT_POSTFIELDS => $billing_request,
    CURLOPT_HTTPHEADER => ['Content-Type: application/xml'],
    CURLOPT_RETURNTRANSFER => true,
])) {
    RESTfail(500, "Internal error -1");
}
if (($billing_response_raw = curl_exec($billing_handle)) === false) {
    RESTfail(500, "Internal error -2");
}
if (($billing_response_code = curl_getinfo($billing_handle, CURLINFO_HTTP_CODE)) !== 200) {
    RESTfail($billing_response_code, $billing_response_raw);
}
$billing_response = xmlrpc_decode($billing_response_raw);
if (is_null($billing_response) || $billing_response === false) {
    RESTfail(500, "Internal error -3");
}
if (xmlrpc_is_fault($billing_response)) {
    RESTfail(500, "Internal error -4");
}
if (!isset($billing_response['_success']) || !is_integer($billing_response['_success'])) {
    RESTfail(500, "Internal error -5");
}
if ($billing_response['_success'] === 0) {
    if (!isset($billing_response['_message']) || !is_string($billing_response['_message'])) {
        RESTfail(500, "Internal error -6");
    }
    // We recognize only a few specific errors
    if ($billing_response['_message'] === 'steam.login.callsign_needed') {
        header('Content-Type: application/json');
        print json_encode(['result' => 2]);
        exit(0);
    } else if (in_array($billing_response['_message'], ['steam.login.invalid', 'steam.login.banned'])) {
        header('Content-Type: application/json');
        print json_encode(['result' => 1]);
        exit(0);
    } else {
        RESTfail(500, "Internal error -7");
    }
} else {
    if (!isset($billing_response['user_id']) || !is_integer($billing_response['user_id'])) {
        RESTfail(500, "Internal error -8");
    }
}

// Do billing XML-RPC here

$auth_data = [
    'user_id' => $billing_response['user_id'],
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
if (!isset($auth_response->otp) || // !is_integer($auth_response->otp) ||
    !isset($auth_response->serverIP) || // !is_integer($auth_response->serverIP) ||
    !isset($auth_response->serverPort)) // || !is_integer($auth_response->serverPort))
{
    RESTfail(500, "Internal error 4");
}

header('Content-Type: application/json');
print json_encode([
    'result' => 0,
    'arenaID' => $arena_id,
    'otp' => strval($auth_response->otp),
    'serverIP' => strval($auth_response->serverIP),
    'serverPort' => $auth_response->serverPort,
]);

?>

