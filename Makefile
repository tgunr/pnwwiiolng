BUILD_DIR := build

all:	| $(BUILD_DIR)/Makefile
	cd $(BUILD_DIR) && $(MAKE)

build:
	mkdir -p "${BUILD_DIR}"
	cd "$(BUILD_DIR)" && cmake -G "Unix Makefiles" -DCMAKE_EXPORT_COMPILE_COMMANDS=ON ../WW2/src && cd ".."

$(BUILD_DIR)/Makefile: build

distclean:
	rm -rf "$(BUILD_DIR)"

clean:
	cd build
	make clean

