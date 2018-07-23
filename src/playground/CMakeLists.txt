add_executable(test_split test_split.c ../utils/tokenizer.c ../utils/utils.c ../utils/log.c ../utils/jvm_cmd.c)
target_compile_options(test_split PRIVATE -march=nehalem -O2)
target_link_libraries(test_split ${LIBUV_LIBRARIES})

add_executable(test_split_avx test_split.c ../utils/tokenizer.c ../utils/utils.c ../utils/log.c ../utils/jvm_cmd.c)
target_compile_options(test_split_avx PRIVATE -march=haswell -O2)
target_link_libraries(test_split_avx ${LIBUV_LIBRARIES})

add_executable(test_simd test_simd_instructions.c ../utils/utils.c ../utils/log.c ../utils/jvm_cmd.c)
target_compile_options(test_simd PRIVATE -march=haswell -O2)
target_link_libraries(test_simd ${LIBUV_LIBRARIES})

add_executable(test_fast_req_package test_fast_req_package.c ../utils/tokenizer_fast_req.c ../utils/log.c ../utils/utils.c ../utils/tokenizer.c ../utils/jvm_cmd.c)
target_compile_options(test_fast_req_package PRIVATE -march=nehalem -O2)
target_link_libraries(test_fast_req_package ${LIBUV_LIBRARIES})

