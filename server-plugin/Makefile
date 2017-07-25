##############################################################################
#   Copyright 2012 - Le Padellec Sylvain                                     #
#                                                                            #
#   Licensed under the Apache License, Version 2.0 (the "License");          #
#   you may not use this file except in compliance with the License.         #
#   You may obtain a copy of the License at                                  #
#       http://www.apache.org/licenses/LICENSE-2.0                           #
#                                                                            #
#   Unless required by applicable law or agreed to in writing, software      #
#   distributed under the License is distributed on an "AS IS" BASIS,        #
#   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. #
#   See the License for the specific language governing permissions and      #
#   limitations under the License.                                           #
##############################################################################

SHELL := /bin/bash -o pipefail

BASE = ..
PROJECT = server-plugin

PROJECT_DIR = $(BASE)/$(PROJECT)
SRC_DIR = $(PROJECT_DIR)/Code
SDK_DIR = $(PROJECT_DIR)/SourceSdk

OTHER_PREPROCESSORS = -DGNUC -DPOSIX -DCOMPILER_GCC

###########################################################
# Compiler.                                               #
# You can specify an absolute path to a specific version. #
###########################################################
ifdef CXX
d_CXX = $(CXX) -m32 -std=c++11
else
d_CXX = g++ -m32 -std=c++11
endif

ARCH_CFLAGS =   -mtune=i486 -march=x86-64 -mmmx
USER_CFLAGS =   -DTIXML_USE_TICPP -Dstricmp=strcasecmp -D_stricmp=strcasecmp -Dstrnicmp=strncasecmp
BASE_CFLAGS =   -msse -D_LINUX -DLINUX -D_alloca=alloca \
		-fPIC -Wno-deprecated -fno-exceptions -fpermissive \
		-fno-strict-aliasing \
		-Wno-non-virtual-dtor -Wno-overloaded-virtual \
		-Wall -Werror

OPT_FLAGS = -DNDEBUG -O3 -funroll-loops -pipe $(OTHER_PREPROCESSORS)
DEBUG_FLAGS = -g -ggdb3 -O0 -D_DEBUG -DDEBUG $(OTHER_PREPROCESSORS)

###########################################################
# Output configuration.                                   #
###########################################################

BINARY_NAME = nocheatzX.so

RELEASE_DIR = Release
DEBUG_DIR = Debug

ifeq "$(DEBUG)" "false"
	BIN_DIR = $(RELEASE_DIR)
else
	BIN_DIR = $(DEBUG_DIR)
endif

BUILD_DIR = $(BASE)/Builds/$(BIN_DIR)
BINARY_DIR = $(BUILD_DIR)/addons/NoCheatZ

#######################################
########## Libraries to link ##########

LINK_A = $(SDK_DIR)/libs/libprotobuf.a

####################################
########## Linker options ########## 

LINK = -v -Wl,--as-needed,--no-undefined $(LINK_A) -shared -static-libgcc -ldl -lm

###########################################################
# Sources and includes                                    #
###########################################################
  
SRC =   $(wildcard Code/*.cpp) \
	$(wildcard Code/Hooks/*.cpp) \
	$(wildcard Code/Misc/*.cpp) \
	$(wildcard Code/Players/*.cpp) \
	$(wildcard Code/Systems/Blockers/*.cpp) \
	$(wildcard Code/Systems/Testers/*.cpp) \
	$(wildcard Code/Systems/*.cpp) \
	SourceSdk/SourceSdk.cpp \
	$(wildcard SourceSdk/Interfaces/Protobuf/*.cpp)

SRC := $(filter-out Code/Hooks/RayTraceCache.cpp, $(SRC)) 
SRC := $(filter-out Code/Misc/StackWalker.cpp, $(SRC)) 
SRC := $(filter-out Code/Systems/Testers/MouseTester.cpp, $(SRC)) 

INCLUDE =       -I$(SRC_DIR) \
		-I$(SDK_DIR) \
		-I$(SDK_DIR)/Interfaces/Protobuf \
		-I$(SDK_DIR)/Interfaces/Protobuf/protobuf-2.5.0/src

###########################################################
# Output rules                                            #
###########################################################

ifeq "$(DEBUG)" "false"
	CFLAGS = $(OPT_FLAGS)
else
	CFLAGS = $(DEBUG_FLAGS)
endif

CFLAGS += $(USER_CFLAGS) $(BASE_CFLAGS) $(ARCH_CFLAGS)

CPP_MAJOR := $(shell $(d_CXX) -dumpversion >&1 | cut -b1)
CPP_MINOR := $(shell $(d_CXX) -dumpversion >&1 | cut -b3)
ifeq "$(shell expr $(CPP_MAJOR) \>= 4)" "1"
	CFLAGS += -fvisibility=hidden -fvisibility-inlines-hidden
endif
ifeq "$(shell expr $(CPP_MAJOR) \>= 4 \& $(CPP_MINOR) \>= 7)" "1"
	CFLAGS += -Wno-delete-non-virtual-dtor
endif

OBJECTS := $(SRC:%.cpp=$(BIN_DIR)/%.o)

###########################################################
# Targets                                                 #
###########################################################

error:
	@echo "Error : No target specified (debug, release, clean, superclean)"; false
	@exit 2

compile_object = \
	@mkdir -p $(2); \
	echo "$(1) => $(3)"; \
	$(d_CXX) $(INCLUDE) $(CFLAGS) -o $(3) -c $(1) 2>&1 | tee "error_compile.txt"

$(BIN_DIR)/%.o: %.cpp %.h
	$(call compile_object, $<, $(@D), $@)

$(BIN_DIR)/%.o: %.cpp
	$(call compile_object, $<, $(@D), $@)

all: $(OBJECTS)
	@$(d_CXX) $(INCLUDE) $(CFLAGS) $(OBJECTS) $(LINK) -m32 -o $(BINARY_DIR)/$(BINARY_NAME)  2>&1 | tee "error_link.txt"

release:
	@$(MAKE) unit-tests DEBUG=false COVERAGE=false
	@$(MAKE) -j6 all DEBUG=false COVERAGE=false

debug:
	@$(MAKE) unit-tests DEBUG=true COVERAGE=false
	@$(MAKE) -j6 all DEBUG=true COVERAGE=false

coverage:
	@$(MAKE) unit-tests DEBUG=false COVERAGE=false
	@$(MAKE) -j6 all COVERAGE=true

clean:
	rm -rf $(RELEASE_DIR)
	rm -rf $(DEBUG_DIR)

superclean: clean
	rm -rf $(BINARY_DIR)/$(BINARY_NAME)

unit-tests: prepare-build
	@echo "Building unit-tests"
	@mkdir -p ./unit-tests/Release
	@echo "gtest-all.o"
	@$(d_CXX) -I./SourceSdk/Interfaces/Protobuf/protobuf-2.5.0/gtest/include -I./SourceSdk/Interfaces/Protobuf/protobuf-2.5.0/gtest $(ARCH_CFLAGS) -o ./unit-tests/Release/gtest-all.o -c ./SourceSdk/Interfaces/Protobuf/protobuf-2.5.0/gtest/src/gtest-all.cc
	@echo "HeapMemoryManager.o"
	@$(d_CXX) -I./Code/ -I./SourceSdk/ $(ARCH_CFLAGS) -o ./unit-tests/Release/HeapMemoryManager.o -c ./Code/Misc/HeapMemoryManager.cpp
	@echo "unit-tests.o"
	@$(d_CXX) -I./Code/ -I./SourceSdk/ -I./SourceSdk/Interfaces/Protobuf/protobuf-2.5.0/gtest/include -I./SourceSdk/Interfaces/Protobuf/protobuf-2.5.0/gtest $(ARCH_CFLAGS) -D__assume=Assert -o ./unit-tests/Release/unit-tests.o -c ./unit-tests/unit-tests.cpp
	@echo "unit-tests.so"
	@$(d_CXX) $(ARCH_CFLAGS) ./unit-tests/Release/unit-tests.o ./unit-tests/Release/HeapMemoryManager.o ./unit-tests/Release/gtest-all.o -v -Wl,--as-needed,--no-undefined -static-libgcc -ldl -lpthread -lm -m32 -o ./unit-tests/unit-tests
	@echo "Starting unit-tests"
	@chmod +x ./unit-tests/unit-tests
	@./unit-tests/unit-tests
	rm -rf ./unit-tests/Release
	rm -rf ./unit-tests/unit-tests
	
prepare-build:
	mkdir -p $(BUILD_DIR)/addons/NoCheatZ/
	mkdir -p $(BUILD_DIR)/cfg/
	mkdir -p $(BUILD_DIR)/logs/NoCheatZ_4_Logs/
	mkdir -p $(BUILD_DIR)/NoCheatZ-autorecords
	@touch $(BUILD_DIR)/logs/NoCheatZ_4_Logs/.gitignore
	@touch $(BUILD_DIR)/NoCheatZ-autorecords/.gitignore
	cp ./Res/nocheatz.vdf $(BUILD_DIR)/addons/
	cp ./Res/config.ini $(BUILD_DIR)/addons/NoCheatZ/
	cp ./Res/nocheatz.cfg $(BUILD_DIR)/cfg/nocheatz.cfg ;true
	@sh $(BASE)/scripts/make-protobuf.sh $(d_CXX)
	cat ./Code/version.cpp
