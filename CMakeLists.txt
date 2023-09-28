cmake_minimum_required(VERSION 3.10)

project(win_capture)

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

include_directories(include third-party/cdk/include)
link_directories(third-party/cdk/lib)

set(SRCS
	src/crypto/cdk-sha1.c
	src/crypto/cdk-sha256.c
	src/encoding/cdk-varint.c
	src/encoding/cdk-base64.c
	src/container/cdk-list.c
	src/container/cdk-queue.c
	src/container/cdk-stack.c
	src/container/cdk-rbtree.c
	src/container/cdk-ringbuffer.c
	src/sync/cdk-rwlock.c
	src/sync/cdk-spinlock.c
	src/net/cdk-net.c
	src/net/cdk-poller.c
	src/net/cdk-channel.c
	src/net/cdk-unpack.c
	src/net/cdk-tls.c
	src/cdk-threadpool.c
	src/cdk-loader.c
	src/cdk-logger.c
	src/cdk-process.c
	src/cdk-utils.c
	src/cdk-time.c
	src/cdk-timer.c
)

if(WIN32)
	list(APPEND SRCS 
		src/platform/win/platform-process.c
		src/platform/win/platform-time.c
		src/platform/win/platform-socket.c
		src/platform/win/platform-utils.c
		src/platform/win/platform-loader.c
		src/platform/win/wepoll/wepoll.c
		src/platform/win/platform-event.c
	)
endif()

if(UNIX)
	list(APPEND SRCS 
		src/platform/unix/platform-process.c
		src/platform/unix/platform-time.c
		src/platform/unix/platform-socket.c
		src/platform/unix/platform-utils.c
		src/platform/unix/platform-loader.c
		src/platform/unix/platform-event.c
	)
endif()

add_executable(win_capture "src/platform/win/win-desktop-capture.c" "src/platform/win/win-desktop-capture.h" "src/platform/desktop-capture.c" "include/desktop-capture.h" "include/desktop-capture-types.h")
target_link_libraries(win_capture PUBLIC Threads::Threads cdk)
