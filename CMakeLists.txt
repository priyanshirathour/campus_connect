cmake_minimum_required(VERSION 3.20)
project(CampusConnect LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(MSVC)
    add_compile_options(/W4 /permissive-)
    add_compile_definitions(_CRT_SECURE_NO_WARNINGS)
endif()

# The package names and imported targets are provided by vcpkg's MongoDB C++ driver.
find_package(mongocxx CONFIG REQUIRED)
find_package(bsoncxx CONFIG REQUIRED)
find_package(httplib CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(unofficial-sodium CONFIG REQUIRED)

file(GLOB_RECURSE CAMPUS_CONNECT_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
)

# main.cpp is deliberately included in the application target. Omitting it causes LNK2019 main.
add_executable(campus_connect ${CAMPUS_CONNECT_SOURCES})
target_include_directories(campus_connect PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_link_libraries(campus_connect PRIVATE
    mongo::mongocxx_shared
    mongo::bsoncxx_shared
    httplib::httplib
    nlohmann_json::nlohmann_json
    unofficial-sodium::sodium
)

include(CTest)
if(BUILD_TESTING)
    add_executable(campus_connect_tests
        tests/test_core.cpp
        src/datastructures/WaitQueue.cpp
        src/datastructures/RecommendationGraph.cpp
        src/utils/Utils.cpp
        src/exceptions/CampusConnectException.cpp
    )
    target_include_directories(campus_connect_tests PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/include")
    add_test(NAME CampusConnectCoreTests COMMAND campus_connect_tests)
endif()
