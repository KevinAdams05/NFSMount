## Haiku Generic Makefile v2.6 ##

## Fill in this file to specify the project being created, and the referenced
## Haiku Makefile engine will do all the hard work for you.

# The name of the created binary.
NAME = NFSMount

# The type of binary: APP, SHARED, STATIC, DRIVER, KERNEL_ADDON
TYPE = APP

# If defined, the application's MIME signature.
APP_MIME_SIG = application/x-vnd.NFSMount

# The following lines list the source files.
SRCS = \
	AboutBox.cpp \
	App.cpp \
	Constants.cpp \
	EditShareWindow.cpp \
	ImportExport.cpp \
	MainWindow.cpp \
	Settings.cpp \
	ShareItem.cpp \
	ShareManager.cpp

# Resource definition files (.rdef)
RDEFS = NFSMount.rdef

# Additional resource files (.rsrc) -- usually left empty when using RDEFS
RSRCS =

# Libraries to link against
#   be            - libbe.so (core Haiku/BeOS API)
#   columnlistview - BColumnListView / BColumnTypes
#   tracker       - Tracker API (for shared headers)
LIBS = be columnlistview tracker localestub

# Additional paths for header files
LOCAL_INCLUDE_PATHS =

# System include paths for private Haiku headers
SYSTEM_INCLUDE_PATHS = \
	/boot/system/develop/headers/private/interface

# Compiler flags for both C and C++
COMPILER_FLAGS =

# Linker flags
LINKER_FLAGS =

# Build optimization level: NONE, SOME, FULL
OPTIMIZE := FULL

# Locales for localization support (empty = no localization)
LOCALES =

# Use the Haiku makefile engine
BUILDHOME ?= /boot/system/develop
include $(BUILDHOME)/etc/makefile-engine
