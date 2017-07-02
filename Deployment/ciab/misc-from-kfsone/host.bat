start /min wwiiol\bin\controller
start /min wwiiol\bin\dbd
timeout 1 >wwiiol\bin\nul
start /min wwiiol\bin\stratd
start /min wwiiol\bin\chatd
start /min wwiiol\bin\hostd
timeout 5 >nul
start /min wwiiol\bin\mmhostd
echo "hosts started. see c:\temp\ for logs"
