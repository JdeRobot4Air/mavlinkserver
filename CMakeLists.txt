# Source code files
set(SOURCE_FILES
	mavlinkserver.cpp
	autopilot_interface.cpp
	serial_port.cpp
)

# Include headers from this paths
include_directories(
	${INTERFACES_CPP_DIR}
	${LIBS_DIR}
	${CMAKE_CURRENT_SOURCE_DIR}
	mavlink
)

# Executable name and its dependencies
add_executable(mavlinkserver
	${SOURCE_FILES}
)

# Needed libraries
target_link_libraries(mavlinkserver
	JderobotInterfaces
	pthread
	${ZeroCIce_LIBRARIES}
)

