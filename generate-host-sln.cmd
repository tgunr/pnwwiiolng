@echo You need CMake installed to generate the host solution.
@echo See http://www.cmake.org/
@echo Once installed, run cmake-gui
@echo  Source Location: %CD%\WW2\src
@echo  Build Location : %CD%\build\host
@mkdir build

cmake-gui %CD%\WW2\src %CD%\build\host

@pause
