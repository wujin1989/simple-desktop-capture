cmake_minimum_required(VERSION 3.10)

project(simple-desktop-capture)

set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(THREADS_PREFER_PTHREAD_FLAG TRUE)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS ON)
set(CMAKE_CXX_STANDARD 20)

if(WIN32)
add_compile_options("/experimental:c11atomics")
add_compile_options("/D_CRT_SECURE_NO_WARNINGS")
endif()

find_package(Threads REQUIRED)
find_package(OpenSSL REQUIRED)

include_directories(include src third-party/cdk/include ${OPENSSL_INCLUDE_DIR})
link_directories(third-party/cdk/lib)

set(SRCS
	src/simple-desktop-capture.c
)

if(WIN32)
	list(APPEND SRCS 
		src/platform/win/desktop-capture-dxgi.c
		src/platform/win/platform-desktop-capture.c
	)
endif()

if(UNIX)
	
endif()

add_executable(simple-desktop-capture ${SRCS})
target_link_libraries(simple-desktop-capture PUBLIC Threads::Threads cdk ${OPENSSL_LIBRARIES})
