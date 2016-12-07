#! /bin/bash
# Create and populate the /playnet directory structure.
# Arguments: $0 <binary_dir>

set -e

######################################################################
## HELPERS

function error() {
  echo -e "ERROR: $*"
}

function echo_prefixed() {
  local prefix=$1; shift
  local output="$*"
  if [ -z "${output}" ]; then echo; else echo -e "${prefix}: $*"; fi
}

function ack() {
  echo_prefixed "GOOD" "$*"
}

function note() {
  echo_prefixed "NOTE" "$*"
}

function die() {
  error $*
  exit 1
}

function check_accessible() {
  if [[ ! -r "${1}" || ! -w "${1}" || ( -d "${1}" && ! -x "${1}" ) ]]; then
    die "${1} exists but permissions are wrong."
  fi
}

function check_executable() {
  if [[ ! -e "${1}" || ! -x "${1}" ]]; then
    die "${1} does not exist or is not executable."
  fi
}

function check_not_exists() {
  if [[ -e "${1}" ]]; then
    die "${1} already exists. This is a bootstrap script."
  fi
}

function check_exists_and_accessible() {
  if [[ ! -e "${1}" ]]; then die "${1} does not exist."; fi
  check_accessible $1
}

function create_dir() {
  # -m 0770, set the mode to u=rwx,g=rwx,o=none
  # -p create parents
  if ! mkdir -m 0770 -p "${1}"; then
    die "Failed creating ${1}: $?"
  fi
  check_accessible "${1}"
  ack "created ${1}"
}

function create_link() {
  local target="$1"
  local link_location="$2"
  if ! ln -s "${target}" "${link_location}"; then
    die "Failed to link ${target} -> ${link_location}: $?"
  fi
  ack "Linked ${target} -> ${link_location}"
}

function get_cfg_value() {
  local src="${1}"
  local dst="${2}"
  # reads config-envar ${1} into ${2}
  eval ${dst}="$(${controller_exe} --getenv ${src})"
  if [[ -z "${!dst}" ]]; then die "Unable to read ${src}"; fi
  ack "'${src}' is '${!dst}'"
}

function read_prompted() {
  local prompt="$1"
  local dest_var="$2"
  local default="$3"
  if [[ ! -z "${default}" ]]; then prompt="${prompt} ($default)"; fi
  while read -p "* Enter ${prompt}:: " "${dest_var}"; do
    eval ${dest_var}="${!dest_var:-${default}}"
    if [[ ! -z "${!dest_var}" ]]; then return; fi
    error "Can't proceed without ${1}."
  done
  die "Input terminated - can't continue."
}

function execute_sql() {
  local schema="$1"; shift
  local sql="$*"
  mysql --defaults-file="${mysql_defaults}" -BbAqrsNn \
    -h "${db_addr}" "${schema}" -e "${sql}"
}

function is_empty_database() {
  if [[ -z "$(execute_sql 2>/dev/null wwiionline "SHOW TABLES LIKE 'wwii_player'")" ]]; then
    return 0
  else
    return 1
  fi
}

######################################################################
## ROUTINES

function identify_binary_dir() {
  binary_dir="${1%%\\/}"; shift
  check_accessible "${binary_dir}"
}

function identify_executables() {
  # The binary directory should contain *at least* controler and stratLoader
  # we don't check for .exe extensions, so this won't work on windows :(
  controller_exe="${CONTROLLER_EXE:-${binary_dir}/controller}"
  check_executable "${controller_exe}"
  stratLoader_exe="${STRATLOADER_EXE:-${binary_dir}/stratLoader}"
  check_executable "${stratLoader_exe}"
  have_ifconfig=/bin/false
  if [[ -x "/sbin/ifconfig" ]]; then
    ifconfig_exe="/sbin/ifconfig"
    if /sbin/ifconfig lo >/dev/null 2>/dev/null; then have_ifconfig=:; fi
  fi
}

function identify_pn_root() {
  # Invoke controller to get the PLAYNET_PATH parameter, since it's hardcoded
  pn_root="$(${controller_exe} --getenv PLAYNET_PATH)"
  if [[ -z "${pn_root}" ]]; then
    die "'${controller_exe} --getenv PLAYNET_PATH' didn't return anything."
  fi
  pn_root="$(realpath "${pn_root}")"
  check_not_exists "${pn_root}"
  note "USING: pn_root=${pn_root}"
}

function identify_data_source() {
  # things we expect it to contain
  check_exists_and_accessible "${data_dir}"
  data_dir="$(realpath "${data_dir}")"
  check_exists_and_accessible "${data_dir}"

  note "USING: data_dir=${data_dir}"

  check_exists_and_accessible "${data_dir}/th_blitz/weapon/weapClasses.csv"
  check_exists_and_accessible "${data_dir}/vehicles/ground/groundman/ppo.xml"
  check_exists_and_accessible "${data_dir}/xml"
  check_exists_and_accessible "${data_dir}/blitzWor.csv"
  check_exists_and_accessible "${data_dir}/host.loc"
  check_exists_and_accessible "${data_dir}/terrain"

  sdm_files="$(ls -1 ${data_dir}/terrain/*.sdm)"
  if [[ -z "${sdm_files}" ]]; then
    die "${data_dir}/terrain does not contain any .sdm files."
  fi

  read_prompted "path to .lua files" lua_dir
  lua_dir="$(realpath ${lua_dir})"
  check_exists_and_accessible "${lua_dir}"
  check_exists_and_accessible "${lua_dir}/stratlib.lua"
}

function create_pn_root() {
  # Create pn_root folder itself.
  parent="$(dirname "${pn_root}")"
  if [ ! -d "${parent}" ]; then
    die "Can't create ${pn_root}: ${parent} doesn't exist." \
        "Create parent paths and re-run script."
  fi
  if [ -w "${parent}" ]; then
    create_dir "${pn_root}"
  else
    note "Can't create ${pn_root}: ${parent} not writeable.\n" \
       "\n" \
         "If you want me to fix this with 'sudo', enter the appropriate\n" \
       "password at the sudo prompt to follow.\n" \
         "\n"
    # -k forces a prompt for password
    sudo -k
    if ! sudo mkdir -m 0770 "${pn_root}"; then
      die "Bailing out."
    fi
    note "Changing ownership of ${pn_root} to ${USER}."
    sudo chown "${USER}" "${pn_root}"
    note
    check_accessible "${pn_root}"
  fi
}

function use_ifconfig_to_get_iface_values() {
  # if they have ifconfig (windows linux subsystem doesn't), then use it
  # to look up candidates for interfaces, and thus ip addresses.
  interfaces="$(echo $(${ifconfig_exe} -a|grep -v '^lo'|sed -ne 's/^\(.*\): .*/\1/p'))"
  if [[ $(echo "${interfaces}" | wc -w) -eq 1 ]]; then
    note "Using DEFAULT_IFACE=${interfaces}"
    DEFAULT_IFACE="${interfaces}"
  else
    note "Interfaces are: $interfaces."
    read_prompted "default interface" DEFAULT_IFACE
  fi
  # get the first ipv4 address tha tifconfig presents for this interface.
  base_ip="$(${ifconfig_exe} ${DEFAULT_IFACE} | awk '/inet.*netmask/ {print $2}' | head -1)"
  if [ -z "${base_ip}" ]; then
    die "Could not find IP address for ${DEFAULT_IFACE}"
  fi
  note "Using IP address of ${DEFAULT_IFACE} ($base_ip) as BASE_ADDRESS"
  BASE_ADDRESS="${base_ip}"
}

function ask_user_for_iface_values() {
  # if ifconfig is not available, we'll have to prompt the user.
  note "'ifconfig' not available, manual inputs required."
  read_prompted "primary interface" DEFAULT_IFACE "eth0"
  note "Base address should be your client-facing IP. Do not use localhost."
  read_prompted "primary IP address" BASE_ADDRESS
  if [[ "${BASE_ADDRESS}" = "localhost" ]]; then
    die "BASE_ADDRESS can't be localhost. TODO: make this loop."
  fi
}

function identify_database() {
  read_prompted "IP of MySQL database" WWII_DB_ADDRESS "127.0.0.1"
  read_prompted "Database root username" sql_user "root"
  read_prompted "Database root password" sql_passwd

  mysql_defaults="/var/tmp/bootstrap.$$.msd"
  (
    echo "[client]"
    echo "user=${sql_user}"
    echo "password=${sql_passwd}"
    echo "host=${WWII_DB_ADDRESS}"
  )>"${mysql_defaults}"

  trap "rm -f ${mysql_defaults}" 0

  if [[ -z "$(execute_sql mysql "SELECT NULL;")" ]]; then
    die "Unable to access MySQL Database"
  fi
}

function get_config_values() {
  get_cfg_value "WWII_DB_USER" db_user
  read_prompted "game db password (for ${db_user})" WWII_DB_PASSWD

  db_addr="${WWII_DB_ADDRESS}"
  db_pass="${WWII_DB_PASSWD}"

  if ${have_ifconfig} ; then
    use_ifconfig_to_get_iface_values
  else
    ask_user_for_iface_values
  fi

  export WWII_CLUSTER="${WWII_CLUSTER:-dev}"
  export APP_ID=12
  export ARENA_ID=250
  export STRAT_LANGUAGE=en
  export LOG_PATH="${pn_root}/tmp"
  export DATA_DIR="${pn_root}/wwiiol/data"
  export LUA_BASE_PATH="${pn_root}/wwiiol/lua"
  export TEUL_UWEB_PATH="${pn_root}/wwiiol/web/debug"
  export WWII_DB_ADDRESS="${db_addr}"
  export WWII_DB_USER="${db_user}"
  export WWII_DB_PASSWD="${db_pass}"
  export DBSE_PRIMARY_ADDRESS="localhost"
  export DBSE_CONNECTION_TIMEOUT_MS="60000"
  export DATA_DIR="${pn_root}/wwiiol/data"
  export DEFAULT_IFACE="${DEFAULT_IFACE:-eth0}"
  export BASE_ADDRESS="${BASE_ADDRESS}"
  export BASE_UPDT_ADDRESS="0.0.0.0"
  export MANAGE_PROCESSES="dbd;strat;chat;map;cell"
}

function bootstrap_database() {
  if ! is_empty_database; then
    note "Database contains wwiionline.wwii_player, assuming populated."
    return
  fi

  create_stmt="CREATE USER IF NOT EXISTS"
  user_id="\`${db_user}\`@\`%%\`"
  passwd_stmt="IDENTIFIED BY '${db_pass}'"
  if ! execute_sql mysql "${create_stmt} ${user_id} ${passwd_stmt}"; then
    die "Could not create ${user_id}."
  fi
  ack "Created ${db_user} user."

  # Now import data
  note "Populating database"
  if ! mysql \
        --defaults-file=${mysql_defaults} \
        --batch \
        --default-character-set="utf8" \
        --no-auto-rehash \
        --quick \
        --unbuffered \
        <mysqldb.sql; then
    die "Failed importing database."
  fi
  ack "Database populated"

  grant_stmt="GRANT ALL PRIVILEGES ON *.* TO ${user_id} WITH GRANT OPTION;"
  echo ${grant_stmt}
  if ! execute_sql mysql "${grant_stmt}"; then
    die "Could not perform grant on ${user_id}."
  fi
  ack "Granted permissions to ${db_user} user."
  execute_sql mysql "FLUSH PRIVILEGES;"
}

function is_missing_strat_data() {
  cp_count=$(execute_sql wwiionline "SELECT COUNT(*) FROM strat_cp;")
  if [[ -z "${cp_count}" || ${cp_count} -eq 0 ]]; then return 0; else return 1; fi
}

function load_strat_data() {
  note "Strat data does not appear to be loaded."
  # -I don't use IRC, -O don't use origin file.
  if ! ${stratLoader_exe} -I -O ; then
    die "Failed loading strat data."
  fi
  ack "Strat data loaded"
}

function build_playnet_directory_tree() {
  for subdir in \
        bin \
        etc \
        tmp \
        wwiiol \
          wwiiol/lua \
          wwiiol/web/debug
  do
    path="${pn_root}/${subdir}"
    if [[ ! -d "${path}" ]]; then
      create_dir "${path}"
    else
      ack "${path} exists."
    fi
  done

  # use symlinks for wwiiol/{bin,data}
  create_link "${binary_dir}" "${pn_root}/wwiiol/bin"
  create_link "${data_dir}" "${pn_root}/wwiiol/data"

  cp "${lua_dir}"/*.lua "${pn_root}/wwiiol/lua"
}

function generate_environment_cfg() {
  # we've export'd our environment overloads so let's go ahead and
  # generate a complete config using our values.
  env_cfg="${pn_root}/etc/environment.cfg"
  check_not_exists "${env_cfg}"

  note
  note "Generating ${env_cfg}."

  if ! ${controller_exe} --make-config-file "${env_cfg}"; then
    die "Failed generating ${env_cfg}: $?"
  fi
  ack "Generated ${env_cfg}"

  read -p "Edit ${env_cfg} now? (Y/n) " yes_or_no
  if [[ ! "x${yes_or_no}" =~ ^xn ]]; then
    ${EDITOR:-${VISUAL:-vim}} "${env_cfg}"
  else
    ack "Using default-generated ${env_cfg}."
  fi
}

function generate_ticket_files() {
  note "Generating ticket file(s). Enter callsigns in lowercase."
  while read -p "Enter gamename or enter to end: " callsign; do
    if [[ -z "${callsign}" ]]; then break; fi
		player_id=$(execute_sql wwiionline "\
			SELECT  p.playerid \
				FROM  wwii_player p \
			 WHERE  p.callsign = '${callsign}'")
		if [[ -z "${player_id}" ]]; then
			error "'${callsign}' not found in wwii_player table."
			continue
		fi
		ack "${callsign} = ${player_id}"
		tkt_file="${data_dir}/${callsign}.tkt"
		if [[ -e "${tkt_file}" ]]; then
			error "${tkt_file} already exists - skipping."
			continue
		fi
		(
			echo "apID: ${app_id}"
			echo "data: Login Accepted"
			echo "serverIP: ${base_addr}"
			echo "port: 27015"
			echo "protocol: 2"
			echo "OTP: 2342426"
			echo "bypass: ${player_id}"
		) >"${tkt_file}"
		ack "${tkt_file} generated."
	done
}

######################################################################
## main()

function main() {
  local binary_dir="${1}"
  local data_dir="${2}"
  identify_binary_dir "${binary_dir}"
  identify_executables
  identify_pn_root
  identify_data_source "${data_dir}"

  read_prompted "cluster name" WWII_CLUSTER "dev"

  identify_database

  get_config_values

  create_pn_root
  build_playnet_directory_tree
  generate_environment_cfg

  # now reload values from the environment
  get_cfg_value APP_ID app_id
  get_cfg_value BASE_ADDRESS base_addr
  get_cfg_value WWII_DB_ADDRESS db_addr
  get_cfg_value WWII_DB_USER db_user
  get_cfg_value WWII_DB_PASSWD db_pass

  bootstrap_database
  if is_missing_strat_data; then load_strat_data; fi

  generate_ticket_files
}


######################################################################

# check for two and only two arguments.
if [[ -z "${1}" || -z "${2}" || ! -z "${3}" ]]; then
  echo "USAGE: ${0} <binary_dir> <mods/DATA dir>"
  exit 1
fi
main "$1" "$2"
