add_library(CrystalEntityLib
  manager.cpp
)

set_target_properties(CrystalEntityLib
    PROPERTIES
        CXX_STANDARD 20
        CXX_STANDARD_REQUIRED YES
        CXX_EXTENSIONS NO
)
target_compile_options(CrystalEntityLib PRIVATE -Wall -Wextra -Werror)
target_include_directories(CrystalEntityLib
    PUBLIC
        $<INSTALL_INTERFACE:include>
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
        ${PROJECT_SOURCE_DIR}/src
)