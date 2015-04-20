# Source code files
set(SOURCE_FILES
	serial/examples/serial_example.cc
)

set(serial_SRCS
	serial/src/serial.cc src/impl/list_ports/list_ports_linux.cc
	serial/include/serial/serial.h
	serial/include/serial/v8stdint.h
)

# Include headers from this paths
include_directories(
	${INTERFACES_CPP_DIR}
	${LIBS_DIR}
	${CMAKE_CURRENT_SOURCE_DIR}
	serial/include
)

add_library(serial SHARED
	${serial_SRCS}
)

set_property(TARGET serial PROPERTY SOVERSION 1.2.0)


# Executable name and its dependencies
add_executable(mavlinkserver
	${SOURCE_FILES}
)

# Needed libraries
target_link_libraries(mavlinkserver
	serial
	JderobotInterfaces
	${ZeroCIce_LIBRARIES}
)

