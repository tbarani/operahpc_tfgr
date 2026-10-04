cmake_minimum_required(VERSION 3.16)

project(TFGR
    VERSION     1.0.0
    DESCRIPTION ""
    LANGUAGES   CXX
)

set(CMAKE_CXX_STANDARD          17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS        OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS True)
# Delauney variant  →  cmake -DDELAUNEY_MODEL=A  or  =B
set(DELAUNEY_MODEL "A" CACHE STRING "Delauney model variant: A (Arrhenius) or B (power-law)")
set_property(CACHE DELAUNEY_MODEL PROPERTY STRINGS "A" "B")

if(NOT DELAUNEY_MODEL STREQUAL "A" AND NOT DELAUNEY_MODEL STREQUAL "B")
    message(FATAL_ERROR "DELAUNEY_MODEL must be 'A' or 'B', got '${DELAUNEY_MODEL}'")
endif()

# LibTorch backend  →  cmake -DUSE_LIBTORCH=ON -DCMAKE_PREFIX_PATH=/opt/libtorch
option(USE_LIBTORCH "Enable LibTorch backend for the NN model" OFF)

# ── LibTorch (optional) ───────────────────────────────────────────────────────
if(USE_LIBTORCH)
    find_package(Torch REQUIRED)
    message(STATUS "LibTorch version   : ${Torch_VERSION}")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${TORCH_CXX_FLAGS}")
endif()

add_library(fuel_warnings INTERFACE)
target_compile_options(fuel_warnings INTERFACE
    $<$<CXX_COMPILER_ID:GNU,Clang,AppleClang>:
        -Wall -Wextra -Wpedantic -Wconversion -Wshadow>
    $<$<CXX_COMPILER_ID:MSVC>:/W4>
)

# ── Executable ────────────────────────────────────────────────────────────────
add_executable(tfgr
    src/Main.cxx
    src/InputReader.cxx
    src/DelauneyModel.cxx
    src/ResultsWriter.cxx
    src/FRInput.cxx
    src/PowerHistory.cxx
    src/RadFG.cxx
    src/RelapPlot.cxx
    src/Eos.cxx
    src/Microstructure.cxx
    src/Driver.cxx
    src/NNModel.cxx
    src/Validator.cxx
)

target_include_directories(tfgr PRIVATE include)

target_link_libraries(tfgr
    PRIVATE fuel_warnings
    $<$<BOOL:${USE_LIBTORCH}>:${TORCH_LIBRARIES}>
)

target_compile_definitions(tfgr PRIVATE
    DELAUNEY_MODEL_${DELAUNEY_MODEL}
    $<$<BOOL:${USE_LIBTORCH}>:USE_LIBTORCH>
)

target_compile_options(tfgr PRIVATE
    $<$<AND:$<CXX_COMPILER_ID:GNU,Clang,AppleClang>,$<CONFIG:Release>>:
        -O3 -march=native>
    $<$<AND:$<CXX_COMPILER_ID:MSVC>,$<CONFIG:Release>>:/O2>
)

# ── Installation ──────────────────────────────────────────────────────────────
include(GNUInstallDirs)

install(TARGETS tfgr
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

install(
    FILES
        ${CMAKE_CURRENT_SOURCE_DIR}/examples/history.csv
        ${CMAKE_CURRENT_SOURCE_DIR}/examples/config.txt
    DESTINATION ${CMAKE_INSTALL_DATADIR}/tfgr/examples
    OPTIONAL
)

# ── CPack ─────────────────────────────────────────────────────────────────────
set(CPACK_PACKAGE_NAME    "tfgr")
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
set(CPACK_GENERATOR       "TGZ;ZIP")
include(CPack)

# ── Tests (Jernkvist validation harness) ──────────────────────────────────────
enable_testing()
add_executable(test_validator
    tests/test_validator.cxx
    src/Validator.cxx
)
target_include_directories(test_validator PRIVATE include)
target_compile_options(test_validator PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang,AppleClang>:
        -Wall -Wextra -Wpedantic -Wconversion -Wshadow>
    $<$<CXX_COMPILER_ID:MSVC>:/W4>
)
add_test(NAME validator_smoke COMMAND test_validator)

# ── Build summary ─────────────────────────────────────────────────────────────
message(STATUS "──────────────────────────────────────────────────")
message(STATUS " FuelModel  ${PROJECT_VERSION}")
message(STATUS " Delauney variant: ${DELAUNEY_MODEL}")
message(STATUS " LibTorch backend: ${USE_LIBTORCH}")
message(STATUS " Install prefix  : ${CMAKE_INSTALL_PREFIX}")
message(STATUS "──────────────────────────────────────────────────")
