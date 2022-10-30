FetchContent_Declare(
    Catch2
  GIT_REPOSITORY https://github.com/catchorg/Catch2.git
  GIT_TAG        v3.1.0
)
FetchContent_MakeAvailable(Catch2)

add_executable(CrystalEntityTest
  entity_test.cpp
  component_test.cpp
  pool_test.cpp
  manager_test.cpp
)

set_target_properties(CrystalEntityTest
    PROPERTIES
        CXX_STANDARD 20
        CXX_STANDARD_REQUIRED YES
        CXX_EXTENSIONS NO
)

target_include_directories(CrystalEntityTest
    PUBLIC
        $<INSTALL_INTERFACE:include>
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
        ${PROJECT_SOURCE_DIR}
        ${PROJECT_SOURCE_DIR}/src
)
target_link_libraries(CrystalEntityTest CrystalEntityLib Catch2::Catch2WithMain)