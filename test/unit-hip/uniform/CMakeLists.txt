# Object libraries require CMAKE 2.8.8 version 
CMAKE_MINIMUM_REQUIRED (VERSION 2.8.8) 
MESSAGE(STATUS "CMAKE VERSION ${CMAKE_VERSION}")

SET(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/../../../cmake ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

FILE (GLOB HIPSRCS *.cpp) 

IF(${PLATFORM} MATCHES "hcc")
  # Find HCC compiler
  FIND_PACKAGE(HC++ 1.0 REQUIRED)

  execute_process(COMMAND ${HCC_CONFIG} --install --cxxflags
                          OUTPUT_VARIABLE HCC_CXXFLAGS)
  execute_process(COMMAND ${HCC_CONFIG}  --install --ldflags
                          OUTPUT_VARIABLE HCC_LDFLAGS)

  SET(HIPRNG_INCLUDE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../../../lib/include/")
  SET(HIPRNG_LIBRARY_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../../../build/lib/src")

  string(STRIP "${HCC_CXXFLAGS}" HCC_CXXFLAGS)
  set (HCC_CXXFLAGS "${HCC_CXXFLAGS} -I${HIPRNG_INCLUDE_PATH}")
  string(STRIP "${HCC_LDFLAGS}" HCC_LDFLAGS)
  set (HCC_LDFLAGS "${HCC_LDFLAGS} -L${HIPRNG_LIBRARY_PATH} -L${HIP_PATH}/lib")

  INCLUDE_DIRECTORIES(${HIP_PATH}/include)
  SET (LINK "-lhiprng_hcc -lm -lhip_hcc")

  set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "bin/")
  FOREACH(test_file ${HIPSRCS})
    SET_PROPERTY(SOURCE ${test_file} ../gtest_main.cpp ../gtest-all.cpp APPEND_STRING PROPERTY COMPILE_FLAGS " ${HCC_CXXFLAGS} -DGTEST_HAS_TR1_TUPLE=0 -D__HIP_PLATFORM_HCC__")
    get_filename_component (name_without_extension ${test_file} NAME_WE)
    ADD_EXECUTABLE(${name_without_extension} ${test_file} ../gtest_main.cpp ../gtest-all.cpp)
    SET_PROPERTY(TARGET ${name_without_extension} APPEND_STRING PROPERTY LINK_FLAGS " ${HCC_LDFLAGS} ${LINK}")
  ENDFOREACH()

ELSEIF(${PLATFORM} MATCHES "nvcc")
  SET(HIPRNG_INCLUDE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../../../lib/include/")
  SET(HIPRNG_LIBRARY_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../../../build/lib/src")

  set (HIP_CXXFLAGS "${HIP_CXXFLAGS} -I${HIPRNG_INCLUDE_PATH} -I/usr/local/cuda/include/ -I${HIP_PATH}/include")
  set (HIP_LDFLAGS "${HIP_LDFLAGS} -L${HIPRNG_LIBRARY_PATH} -L/usr/local/cuda/lib64")

  INCLUDE_DIRECTORIES(${HIP_PATH}/include)

  set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "bin/")
  FOREACH(test_file ${HIPSRCS})
    SET_PROPERTY(SOURCE ${test_file} ../gtest_main.cpp ../gtest-all.cpp APPEND_STRING PROPERTY COMPILE_FLAGS " ${HIP_CXXFLAGS} -DGTEST_HAS_TR1_TUPLE=0 -D__HIP_PLATFORM_NVCC__")
    get_filename_component (name_without_extension ${test_file} NAME_WE)
    ADD_EXECUTABLE(${name_without_extension} ${test_file} ../gtest_main.cpp ../gtest-all.cpp)
    SET_PROPERTY(TARGET ${name_without_extension} APPEND_STRING PROPERTY LINK_FLAGS " ${HIP_LDFLAGS} ${LINK}")
    TARGET_LINK_LIBRARIES(${name_without_extension} cudart hiprng_nvcc pthread)
    add_test(NAME ${name_without_extension} COMMAND ./bin/${name_without_extension})
  ENDFOREACH()
ENDIF()
