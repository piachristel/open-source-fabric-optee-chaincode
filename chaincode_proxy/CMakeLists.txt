project (chaincode_proxy C CXX)

set (SRC host/chaincode_proxy.cpp)

add_executable (${PROJECT_NAME} ${SRC} host/invocation.pb.cc host/invocation.grpc.pb.cc)

target_include_directories(${PROJECT_NAME}
			   PRIVATE host/include
			   PRIVATE host)

target_link_libraries (${PROJECT_NAME} 
	PRIVATE teec
	PRIVATE /home/ubuntu/grpc/libs/opt/libaddress_sorting.a
	PRIVATE /home/ubuntu/grpc/libs/opt/libares.a
	PRIVATE /home/ubuntu/grpc/libs/opt/libboringssl.a
	PRIVATE /home/ubuntu/grpc/libs/opt/libgrpc++_core_stats.a
	PRIVATE /home/ubuntu/grpc/libs/opt/libgrpc++_cronet.a
	PRIVATE /home/ubuntu/grpc/libs/opt/libgrpc++_error_details.a
	PRIVATE /home/ubuntu/grpc/libs/opt/libgrpc++_unsecure.a
	PRIVATE /home/ubuntu/grpc/libs/opt/libgpr.a
	PRIVATE /home/ubuntu/grpc/libs/opt/libgrpc.a
	PRIVATE /home/ubuntu/grpc/libs/opt/libgrpcpp_channelz.a
	PRIVATE /home/ubuntu/grpc/libs/opt/libgrpc_unsecure.a
	PRIVATE /home/ubuntu/grpc/libs/opt/libgrpc_cronet.a
	PRIVATE /home/ubuntu/grpc/libs/opt/libgrpc++_reflection.a
	PRIVATE /home/ubuntu/grpc/libs/opt/libgrpc++.a
	PRIVATE /home/ubuntu/grpc/libs/opt/protobuf/libprotobuf.a
	PRIVATE /home/ubuntu/grpc/libs/opt/protobuf/libprotoc.a
	PRIVATE /home/ubuntu/grpc/libs/opt/libz.a
	PRIVATE pthread
	PRIVATE dl)

install (TARGETS ${PROJECT_NAME} DESTINATION ${CMAKE_INSTALL_BINDIR})
