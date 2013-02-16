BUILD_DIR := build

all:	| $(BUILD_DIR)/Makefile
	cd $(BUILD_DIR) && $(MAKE)

$(BUILD_DIR)/Makefile:
	mkdir -p "$(BUILD_DIR)"
	cd "$(BUILD_DIR)" && cmake -G "Unix Makefiles" ../WW2/src && cd ".."

clean:
	rm -rf "$(BUILD_DIR)"
