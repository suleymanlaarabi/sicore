cmake_minimum_required(VERSION 3.25)
project(sicore LANGUAGES C)

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)
option(BUILD_SHARED_LIBS "Build shared libraries" OFF)



add_library(sicore
    src/sicore_map.c
    src/sicore_vec.c)
add_library(sicore::sicore ALIAS sicore)
target_compile_features(sicore PUBLIC c_std_17)
set_target_properties(sicore PROPERTIES C_STANDARD 17 C_STANDARD_REQUIRED YES)
target_include_directories(sicore PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_include_directories(sicore PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/include>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)
target_compile_definitions(sicore PRIVATE sicore_EXPORTS)
if(NOT BUILD_SHARED_LIBS)
    target_compile_definitions(sicore PUBLIC sicore_STATIC)
endif()
target_compile_definitions(sicore PUBLIC SICORE_VEC=1)

install(TARGETS sicore EXPORT sicoreTargets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
install(FILES include/sicore.h DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
install(FILES include/sicore/export.h DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/sicore)
install(EXPORT sicoreTargets NAMESPACE sicore:: DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/sicore)
configure_package_config_file(cmake/sicoreConfig.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/sicoreConfig.cmake
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/sicore)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/sicoreConfig.cmake
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/sicore)

