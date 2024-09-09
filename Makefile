# Builds libraries and application
.PHONY: all lib1 lib2 src clean

all: libdaisy daisysp daisygran

libdaisy: libdaisy/.build

libdaisy/.build: 
	@echo "Building libdaisy..."
	cd libDaisy && $(MAKE)
	@touch libDaisy/.build

daisysp: daisysp/.build

daisysp/.build:
	@echo "Building daisysp..."
	cd DaisySP && $(MAKE)
	@touch DaisySP/.build

daisygran: libdaisy daisysp
	@echo "Building src..."
	cd src && $(MAKE)

clean:
	cd DaisySP && $(MAKE) clean
	cd libDaisy && $(MAKE) clean
	cd src && $(MAKE) clean