#---------------------------------------------------------------------------------------------------------------------
# TARGET is the name of the output.
# BUILD is the directory where object files & intermediate files will be placed.
# LIBBUTANO is the main directory of butano library (https://github.com/GValiente/butano).
# PYTHON is the path to the python interpreter.
# SOURCES is a list of directories containing source code.
# INCLUDES is a list of directories containing extra header files.
# DATA is a list of directories containing binary data.
# GRAPHICS is a list of files and directories containing files to be processed by grit.
# AUDIO is a list of files and directories containing files to be processed by mmutil.
# DMGAUDIO is a list of files and directories containing files to be processed by mod2gbt and s3m2gbt.
# ROMTITLE is a uppercase ASCII, max 12 characters text string containing the output ROM title.
# ROMCODE is a uppercase ASCII, max 4 characters text string containing the output ROM code.
# USERFLAGS is a list of additional compiler flags:
#     Pass -flto to enable link-time optimization.
#     Pass -O0 or -Og to try to make debugging work.
# USERCXXFLAGS is a list of additional compiler flags for C++ code only.
# USERASFLAGS is a list of additional assembler flags.
# USERLDFLAGS is a list of additional linker flags:
#     Pass -flto=<number_of_cpu_cores> to enable parallel link-time optimization.
# USERLIBDIRS is a list of additional directories containing libraries.
#     Each libraries directory must contains include and lib subdirectories.
# USERLIBS is a list of additional libraries to link with the project.
# DEFAULTLIBS links standard system libraries when it is not empty.
# STACKTRACE enables stack trace logging when it is not empty.
# USERBUILD is a list of additional directories to remove when cleaning the project.
# EXTTOOL is an optional command executed before processing audio, graphics and code files.
#
# All directories are specified relative to the project directory where the makefile is found.
#---------------------------------------------------------------------------------------------------------------------
TARGET      	:=  $(notdir $(CURDIR))
BUILD       	:=  build
LIBBUTANO   	:=  butano/butano
PYTHON      	:=  python
SOURCES     	:=  src src/core src/core/dialog src/core/minimap src/viewer src/viewer/runtime src/viewer/math butano/common/src
INCLUDES    	:=  $(BUILD)/generated/include include butano/common/include butano/games/varooom-3d/include butano/butano/hw/include
DATA        	:=
GRAPHICS    	:=  butano/common/graphics graphics/bg graphics/sprite/player graphics/sprite/npc graphics/sprite/hud graphics/sprite/decor graphics/sprite/interior_props graphics/shape_group_textures
AUDIO       	:=
DMGAUDIO    	:=
ROMTITLE    	:=  ROM TITLE
ROMCODE     	:=  SBTP
PROFILE_ENGINE	?=  0
PROFILER_LOG_ENGINE := false
USERCXXFLAGS	:=
USERASFLAGS 	:=
USERLDFLAGS 	:=  -flto
USERLIBDIRS 	:=  
USERLIBS    	:=  
DEFAULTLIBS 	:=  
STACKTRACE		:=	true
USERBUILD   	:=  
EXTTOOL     	:=  $(PYTHON) scripts/generate_room_shell_header.py --output-dir $(BUILD)/generated/include/models

ifeq ($(PROFILE_ENGINE),1)
PROFILER_LOG_ENGINE := true
endif

USERFLAGS   	:=  -flto -DBN_CFG_PROFILER_ENABLED=true -DBN_CFG_PROFILER_LOG_ENGINE=$(PROFILER_LOG_ENGINE) -DBN_CFG_PROFILER_MAX_ENTRIES=16

#---------------------------------------------------------------------------------------------------------------------
# Export absolute butano path:
#---------------------------------------------------------------------------------------------------------------------
ifndef LIBBUTANOABS
	export LIBBUTANOABS	:=	$(realpath $(LIBBUTANO))
endif

#---------------------------------------------------------------------------------------------------------------------
# Include main makefile:
#---------------------------------------------------------------------------------------------------------------------
include $(LIBBUTANOABS)/butano.mak
