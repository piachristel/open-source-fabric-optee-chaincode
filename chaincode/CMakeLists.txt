# source: file bases on https://github.com/linaro-swg/optee_examples/tree/master/hello_world

project (chaincode C CXX)

set (SRC dummy_main.cpp)

add_executable (${PROJECT_NAME} ${SRC} )

target_include_directories(${PROJECT_NAME}
			   PRIVATE ta/include)

target_link_libraries (${PROJECT_NAME} 
	PRIVATE teec
	PRIVATE pthread
	PRIVATE dl)

install (TARGETS ${PROJECT_NAME} DESTINATION ${CMAKE_INSTALL_BINDIR})
