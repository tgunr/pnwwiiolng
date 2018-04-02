<?php

////////////////////////////////////////
// Baseline requirements
function early_death(string $message) {
    // This function is for dealing with failures that occur before we have any
    // environment or logging available.
    http_response_code(500);
    header('Content-Type: application/json', true, 500);
    print '{"result":3,"error":"Internal systems failure"}';
    fprintf(STDERR, $message);
    exit(0);
}

if (PHP_VERSION_ID < 70100) {
    early_death("PHP 7.1 or higher is required.\n");
}
if (!extension_loaded('xmlrpc')) {
    early_death("XML-RPC extension is required.\n");
}
if (!extension_loaded('curl')) {
    early_death("libcurl extension is required.\n");
}
if (!isset($_SERVER['PN_INSTALL_PATH'])) {
    early_death("Installation path configuration is required.\n");
}

////////////////////////////////////////
// Steam response code values
// IMPORTANT: Keep these in sync with authSteam.h!
define('LoginSuccess', 0);
define('LoginNotValidUser', 1);
define('LoginNewUser', 2);
define('LoginCallsignInvalid', 3);
define('LoginCallsignDisallowed', 4);
define('LoginCallsignInUse', 5);
define('LoginUnknownError', 6);
define('LoginEmailRequired', 7);
define('LoginEmailInvalid', 8);
define('LoginEmailInUse', 9);

////////////////////////////////////////
// Logging
class dbug {
    private $fh = null;
    private $cached_filename = null;

    public static function Init(array $environment_cfg): dbug {
        return new dbug($environment_cfg);
    }

    private function __construct(array $environment_cfg) {
        $path = implode(DIRECTORY_SEPARATOR, [$environment_cfg['LOG_PATH'] ?? '/tmp', 'authRelay.log']);
        if (!is_resource($this->fh = fopen($path, 'ab'))) {
            $this->fh = STDERR;
        }
        $this->cached_filename = basename(__FILE__);
    }

    private function log(int $line, string $level, string $message, ...$args) {
        global $context_id;
        
        fprintf($this->fh, "%s [%s %s:%u] %s: {$message}\n",
            $level,
            (new DateTime)->format('Y-m-d H:i:s.v'),
            $this->cached_filename, $line, $context_id,
            ...$args
        );
    }
    
    public function debug(int $line, string $message, ...$args) { $this->log($line, 'D', $message, ...$args); }
    public function info(int $line, string $message, ...$args) { $this->log($line, 'I', $message, ...$args); }
    public function notice(int $line, string $message, ...$args) { $this->log($line, 'N', $message, ...$args); }
    public function warn(int $line, string $message, ...$args) { $this->log($line, 'W', $message, ...$args); }
    public function minor(int $line, string $message, ...$args) { $this->log($line, 'M', $message, ...$args); }
    public function abort(int $line, int $status, string $output, string $message, ...$args) {
        global $context_id;
        
        $this->log($line, 'A', $message, ...$args);
        http_response_code($status);
        print json_encode(['result' => LoginUnknownError, 'context_id' => $context_id, 'error' => $output]);
        exit(0);
    }
}

////////////////////////////////////////
// Utility
function load_environment(): array {
    // Dirty evil trick: A cluster's environment.cfg can be parsed as an INI file if you hack
    // the # comments into ; comments.
    $environment_cfg_path = realpath(implode(DIRECTORY_SEPARATOR,
        [$_SERVER['PN_INSTALL_PATH'], 'wwiiol', 'bin', 'environment.cfg']
    ));
    if (!is_string($environment_cfg_raw = @file_get_contents($environment_cfg_path))) {
        early_death("Unable to read environment configuration.\n");
    }
    $environment_cfg_fixed = preg_replace('/^#/smu', ';', $environment_cfg_raw);
    if (!is_array($environment_cfg = parse_ini_string($environment_cfg_fixed, false, INI_SCANNER_RAW))) {
        early_death("Unable to parse environment configuration.\n");
    }
    return $environment_cfg;
}

////////////////////////////////////////
// Iniitialization
$environment_cfg = load_environment();
$dbugLog = dbug::Init($environment_cfg);
$context_id = preg_replace('/(.{8})(....)(....)(....)(.+)/uAD', '$1-$2-$3-$4-$5', bin2hex(random_bytes(16)));

////////////////////////////////////////
// Configuration
if (!isset($environment_cfg['AUTH_RELAY_BILLING_ENDPOINT'])) {
    early_death("No billing relay endpoint found in environment configuration.\n");
}
$billing_login_endpoint = $environment_cfg['AUTH_RELAY_BILLING_ENDPOINT'];
$billing_xmlrpc_method = 'steam.requestLogin';
$auth_otp_endpoint = 'http://localhost:6081/v1/cluster/{arena_id}/dispatch';

////////////////////////////////////////
// Common headers
header("X-Request-ID: {$context_id}", true);
header("Content-Type: application/json", true);

////////////////////////////////////////
// Input processing and validation
function bad_input(int $line, string $details, ...$args) {
    global $dbugLog;
    
    $dbugLog->abort($line, 400, "Bad input", $details, ...$args);
}

function internal_error(int $line, string $details, ...$args) {
    global $dbugLog;
    
    $dbugLog->abort($line, 500, "Internal error", $details, ...$args);
}

$content = file_get_contents("php://input");
if (!is_object($data = @json_decode($content, false, 512, JSON_BIGINT_AS_STRING))) {
    bad_input(__LINE__, "Input data was not valid JSON: %s", $content);
}
if (!isset($data->steam_ticket)) {
    bad_input(__LINE__, "Steam ticket missing in %s", var_export($data, true));
}
if (!is_string($data->steam_ticket)) {
    bad_input(__LINE__, "Steam ticket not a string: %s", var_export($data->steam_ticket, true));
}

if (!isset($data->arena_id)) {
    bad_input(__LINE__, "Arena ID missing in %s", var_export($data, true));
}
if (!is_integer($data->arena_id)) {
    bad_input(__LINE__, "Arena ID not an integer: %s", var_export($data->arena_id, true));
}

if (!isset($data->app_version)) {
    bad_input(__LINE__, "App version missing in %s", var_export($data, true));
}
if (!is_integer($data->app_version)) {
    bad_input(__LINE__, "App version not an integer: %s", var_export($data->app_version, true));
}

if (!isset($data->app_platform)) {
    bad_input(__LINE__, "App platform missing in %s", var_export($data, true));
}

if (!is_integer($data->app_platform)) {
    bad_input(__LINE__, "App platform not an integer: %s", var_export($data->app_platform, true));
}

if (isset($data->callsign) && !is_string($data->callsign)) {
    bad_input(__LINE__, "Callsign provided but not a string: %s", var_export($data->callsign, true));
}
if (isset($data->email) && !is_string($data->email)) {
    bad_input(__LINE__, "Email provided but not a string: %s", var_export($data->email, true));
}

$steam_ticket = filter_var($data->steam_ticket, FILTER_VALIDATE_REGEXP, ['options' => ['regexp' => '/^([a-zA-Z0-9]+)$/']]);
$arena_id = filter_var($data->arena_id, FILTER_VALIDATE_INT);
$app_version = filter_var($data->app_version, FILTER_VALIDATE_INT);
$app_platform = filter_var($data->app_platform, FILTER_VALIDATE_INT, ['options' => ['min_range' => 0, 'max_range' => 2]]);
if (isset($data->callsign)) {
    // We are choosing not to filter the callsign at this time so that billing can provide
    // full validation. Shortcutting the call to billing by detecting a bad callsign here
    // would prevent proper detection of the incorrect "callsign provided for account
    // which already has one" scenario, which could result in degraded user experience.
    $callsign = strval($data->callsign); //filter_var($data->callsign, FILTER_VALIDATE_REGEXP, ['options' => ['regexp' => '/^[a-z][a-z0-9]{1,9}$/']]);
}
if (isset($data->email)) {
    // Same as callsign above.
    $email = strval($data->email); //filter_var($data->email, FILTER_VALIDATE_EMAIL, []);
}

if (!is_string($steam_ticket)) {
    bad_input(__LINE__, "Steam ticket did not pass validation: %s", $data->steam_ticket);
}
if (!is_integer($arena_id)) {
    bad_input(__LINE__, "Arena ID did not pass validation: %u", $data->arena_id);
}
if (!is_integer($app_version)) {
    bad_input(__LINE__, "App version did not pass validation: %u", $data->app_version);
}
if (!is_integer($app_platform)) {
    bad_input(__LINE__, "App platform did not pass validation: %u", $data->app_platform);
}
if (isset($data->callsign) && !is_string($callsign)) {
    bad_input(__LINE__, "Callsign provided but did not pass validation: %s", $data->callsign);
}
if (isset($data->email) && !is_string($email)) {
    bad_input(__LINE__, "Email provided but did not pass validation: %s", $data->email);
}

////////////////////////////////////////
// Call to billing
$billing_request_data = [$steam_ticket, $arena_id];
if (isset($callsign)) {
    $billing_request_data[] = $callsign;
}
if (isset($email)) {
    if (!isset($callsign)) {
        // Awful hack to deal with existing users who have callsigns but no emails yet.
        $billing_request_data[] = "";
    }
    $billing_request_data[] = $email;
}
$billing_request = xmlrpc_encode_request($billing_xmlrpc_method, $billing_request_data, ['verbosity' => 'no_white_space']);
$billing_handle = curl_init($billing_login_endpoint);
if (!curl_setopt_array($billing_handle, [
    CURLOPT_CUSTOMREQUEST => 'POST',
    CURLOPT_POSTFIELDS => $billing_request,
    CURLOPT_HTTPHEADER => ['Content-Type: application/xml'],
    CURLOPT_RETURNTRANSFER => true,
])) {
    internal_error(__LINE__, "Setting options for billing request failed");
}
$dbugLog->debug(__LINE__, "Sending billing request %s", $billing_request);
if (($billing_response_raw = curl_exec($billing_handle)) === false) {
    internal_error(__LINE__, "Billing request failed: req:%s res:%s", $billing_request, curl_error($billing_handle));
}
if (($billing_response_code = curl_getinfo($billing_handle, CURLINFO_HTTP_CODE)) !== 200) {
    internal_error(__LINE__, "Billing request returned HTTP error: req:%s rstat:%u res:%s", $billing_request, $billing_response_code, $billing_response_raw);
}
$billing_response = xmlrpc_decode($billing_response_raw);
if (is_null($billing_response) || $billing_response === false) {
    internal_error(__LINE__, "Billing response was not valid XML-RPC: req:%s res:%s", $billing_request, $billing_response_raw);
}
$dbugLog->debug(__LINE__, "Got billing response %s", var_export($billing_response, true));
if (xmlrpc_is_fault($billing_response)) {
    internal_error(__LINE__, "Billing response was XML-RPC fault: req:%s res:%s", $billing_request, var_export($billing_response, true));
}
if (!isset($billing_response['_success']) || !is_integer($billing_response['_success'])) {
    internal_error(__LINE__, "Billing returned non-success response: req:%s res:%s", $billing_request, var_export($billing_response, true));
}
if ($billing_response['_success'] === 0) {
    if (!isset($billing_response['_message']) || !is_string($billing_response['_message'])) {
        internal_error(__LINE__, "Billing returned failure response with no message: req:%s res:%s", $billing_request, var_export($billing_response, true));
    }
    // We recognize only a few specific errors
    if ($billing_response['_message'] === 'steam.login.callsign_needed') {
        print json_encode(['result' => LoginNewUser]);
        exit(0);
    } else if ($billing_response['_message'] === 'validate.invalid.wwiionline') {
        print json_encode(['result' => LoginCallsignInvalid]);
        exit(0);
    } else if ($billing_response['_message'] === 'validate.invalid.wwiionline.bad') {
        print json_encode(['result' => LoginCallsignDisallowed]);
        exit(0);
    } else if ($billing_response['_message'] === 'validate.invalid.wwiionline.taken') {
        print json_encode(['result' => LoginCallsignInUse]);
        exit(0);
    } else if (in_array($billing_response['_message'], ['steam.login.invalid', 'steam.login.banned'])) {
        print json_encode(['result' => LoginNotValidUser]);
        exit(0);
    } else if ($billing_response['_message'] === 'steam.login.email_needed') {
        print json_encode(['result' => LoginEmailRequired]);
        exit(0);
    } else if ($billing_response['_message'] === 'validate.invalid.profile') {
        print json_encode(['result' => LoginEmailInvalid]);
        exit(0);
    } else if ($billing_response['_message'] === 'validate.invalid.profile.email.inuse') {
        print json_encode(['result' => LoginEmailInUse]);
        exit(0);
    } else {
        internal_error(__LINE__, "Billing returned unrecognized failure response: req:%s res:%s", $billing_request, var_export($billing_response, true));
    }
} else {
    if (!isset($billing_response['user_id'])) {
        internal_error(__LINE__, "Billing returned success response with no user ID: req:%s res:%s", $billing_request, var_export($billing_response, true));
    }
    if (!is_integer($billing_response['user_id'])) {
        internal_error(__LINE__, "Billing returned success response with non-integer user ID: req:%s res:%s", $billing_request, var_export($auth_response, true));
    }
}

////////////////////////////////////////
// Request auth OTP dispatch
$auth_data = [
    'user_id' => $billing_response['user_id'],
    'app_version' => $app_version,
    'app_platform' => $app_platform,
];
$auth_data_encoded = json_encode($auth_data);
$auth_url = str_replace('{arena_id}', $arena_id, $auth_otp_endpoint);
$auth_handle = curl_init($auth_url);
if (!curl_setopt_array($auth_handle, [
    CURLOPT_CUSTOMREQUEST => 'POST',
    CURLOPT_POSTFIELDS => $auth_data_encoded,
    CURLOPT_HTTPHEADER => ['Content-Type: application/json'],
    CURLOPT_RETURNTRANSFER => true,
])) {
    internal_error(__LINE__, "Setting options for billing request failed");
}
if (($auth_response_raw = curl_exec($auth_handle)) === false) {
    internal_error(__LINE__, "Auth request failed: req:%s res:%s", $auth_data_encoded, curl_error($auth_handle));
}
if (($auth_response_code = curl_getinfo($auth_handle, CURLINFO_HTTP_CODE)) !== 200) {
    internal_error(__LINE__, "Auth request returned HTTP error: req:%s rstat:%u res:%s", $auth_data_encoded, $auth_response_code, $auth_response_raw);
}
if (!is_object($auth_response = @json_decode($auth_response_raw, false, 512, JSON_BIGINT_AS_STRING))) {
    internal_error(__LINE__, "Auth response was not valid JSON: req:%s res:%s", $auth_data_encoded, $auth_response_raw);
}
if (!isset($auth_response->otp)) {
    internal_error(__LINE__, "Auth response was missing OTP: req:%s res:%s", $auth_data_encoded, var_export($auth_response, true));
}
// if (!is_integer($auth_response->otp)) {
//     internal_error(__LINE__, "Auth returned success response with non-integer OTP: req:%s res:%s", $auth_data_encoded, var_export($auth_response, true));
// }
if (!isset($auth_response->serverIP)) {
    internal_error(__LINE__, "Auth response was missing server IP: req:%s res:%s", $auth_data_encoded, var_export($auth_response, true));
}
// if (!is_integer($auth_response->serverIP)) {
//     internal_error(__LINE__, "Auth returned success response with non-integer server IP: req:%s res:%s", $auth_data_encoded, var_export($auth_response, true));
// }
if (!isset($auth_response->serverPort)) {
    internal_error(__LINE__, "Auth response was missing server port: req:%s res:%s", $auth_data_encoded, var_export($auth_response, true));
}
// if (!is_integer($auth_response->serverPort)) {
//     internal_error(__LINE__, "Auth returned success response with non-integer server port: req:%s res:%s", $auth_data_encoded, var_export($auth_response, true));
// }

////////////////////////////////////////
// Final reply
print json_encode([
    'result' => LoginSuccess,
    'arenaID' => $arena_id,
    'otp' => strval($auth_response->otp),
    'serverIP' => strval($auth_response->serverIP),
    'serverPort' => $auth_response->serverPort,
]);

?>
