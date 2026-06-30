.PHONY: format format-check

format: 
	cmake --build build --target format

format-check: 
	cmake --build build --target format-check