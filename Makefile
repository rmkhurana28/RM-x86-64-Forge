.PHONY: all build run clean

# Default target when you just type 'make'
all: build

# Target to compile the project
build:
	@mkdir -p build
	@cd build && cmake .. > /dev/null && $(MAKE) --no-print-directory

# Target to run the compiled executable
run: build
	@echo "========================================="
	@./build/rm_x86_64_forge | tee output.txt

# Target to clean up the build directory
clean:
	@echo "Cleaning up build directory..."
	@rm -rf build
