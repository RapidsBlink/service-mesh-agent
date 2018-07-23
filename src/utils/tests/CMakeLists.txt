find_package(Threads REQUIRED)
add_executable(test_blockQueue test_blockQueue.c ../blockQueue.h ../blockQueue.c)
target_link_libraries(test_blockQueue ${LIBUV_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})


add_executable(test_etcd test_etcd.c ../etcd.c ../etcd.h ../log.h ../log.c ../../../3rd-deps/jsmn/jsmn.c
        ../../../3rd-deps/jsmn/jsmn.h ../../../3rd-deps/base64/b64.h ../../../3rd-deps/base64/decode.c
        ../../../3rd-deps/base64/encode.c ../utils.c ../jvm_cmd.c ../jvm_cmd.h)
target_compile_options(test_etcd PRIVATE -g -O0)
target_link_libraries(test_etcd ${CURL_LIBRARIES} ${LIBUV_LIBRARIES})



add_executable(test_utils test_utils.c ../etcd.h ../log.h ../log.c ../../../3rd-deps/jsmn/jsmn.c
        ../../../3rd-deps/jsmn/jsmn.h ../../../3rd-deps/base64/b64.h ../../../3rd-deps/base64/decode.c
        ../../../3rd-deps/base64/encode.c ../utils.c ../jvm_cmd.c ../jvm_cmd.h)
target_compile_options(test_utils PRIVATE -g -O0)
target_link_libraries(test_utils ${CURL_LIBRARIES} ${LIBUV_LIBRARIES})


add_executable(echo_server echo_server.c ../etcd.h ../log.h ../log.c )
target_compile_options(echo_server PRIVATE -g -O0)
target_link_libraries(echo_server ${LIBUV_LIBRARIES})

add_executable(read_named_pipe read_named_pipe.c ../etcd.h ../log.h ../log.c )
target_compile_options(read_named_pipe PRIVATE -g -O0)
target_link_libraries(read_named_pipe ${LIBUV_LIBRARIES})

add_executable(jvm_gc jvm_gc.c ../etcd.h ../log.h ../log.c )
target_compile_options(jvm_gc PRIVATE -g -O0)
target_link_libraries(jvm_gc ${LIBUV_LIBRARIES})