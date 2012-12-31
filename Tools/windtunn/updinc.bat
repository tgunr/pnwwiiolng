rem cd include
rem deltree /Y *.*
rem cd ..
delete /Y
copy .\fio\*.h .\include
copy .\math\*.h .\include
copy .\math\math3d.c .\include
copy .\mem\*.h .\include
copy .\pi\*.h .\include
copy .\sys\*.h .\include
copy .\ml\*.h .\include
copy .\vars.h .\include
