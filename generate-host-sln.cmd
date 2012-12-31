@rem Windows script for creating the Visual Studio solution for the game server.
@rem Assumes you have CMAKE 2.8 installed

@PATH=%PATH%;"C:\Program Files (x86)\CMake 2.8\bin";"C:\Program Files\CMake 2.8\bin"

@rem We need to issue commands from the WW2\src\hostproj folder
@cd WW2\src\hostproj

@rem Cmake command to build the Visual Studio 2010 version of the project.
cmake -G "Visual Studio 10" ..

@echo  
@echo //////////////////////////////////////////////////////////////////////
@echo //
@echo  
@echo Good news!
@echo Visual Studio 2010 HostProject built in WW2\src\hostproj
@echo  

@pause
