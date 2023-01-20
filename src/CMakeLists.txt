cmake_minimum_required(VERSION 3.20)
project(SvnLog)
set(TARGET_MFC "SvnLog-exe")
set(TARGET_NAME "SvnLog")

# set required c++ version
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# set project source-files
set(SOURCE_FILES
  SvnLog.cpp
  SvnLogDlg.cpp
  SvnRepos.cpp
  FilteredCommits.cpp
  DateConverter.cpp
  CAutoListCtrl.cpp
  CBrowseCtrl.cpp
  pch.cpp)
set(HEADER_FILES
  SvnLog.h
  SvnLogDlg.h
  SvnRepos.h
  FilteredCommits.h
  DateConverter.h
  CAutoListCtrl.h
  CBrowseCtrl.h
  framework.h
  pch.h
  targetver.h)
set(RESOURCE_FILES
  Resource.h
  SvnLog.rc
  res/svn.ico
  res/SvnLog.rc2)

# compile executable
add_executable(${TARGET_MFC} WIN32 ${SOURCE_FILES} ${HEADER_FILES} ${RESOURCE_FILES})
set_target_properties(${TARGET_MFC} PROPERTIES OUTPUT_NAME ${TARGET_NAME})

# fix mfc winmain link issue for mfc unicode program
set_target_properties(${TARGET_MFC} PROPERTIES LINK_FLAGS "/SUBSYSTEM:WINDOWS")
target_link_options(${TARGET_MFC} PRIVATE "/entry:wWinMainCRTStartup")

# list of required third-party libraries
find_package(fmt CONFIG REQUIRED)
find_package(winpp CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)

# set project compile definitions
target_compile_definitions(${TARGET_MFC}
  PRIVATE  
    WIN32_LEAN_AND_MEAN   # prevents Windows.h from adding unnecessary includes
    NOMINMAX              # prevents Windows.h from defining min/max as macros  
    UNICODE               # activate unicode
    _UNICODE
    _AFXDLL               # use MFC library in shared
    FMT_HEADER_ONLY)

# force utf-8 encoding for source-files
add_compile_options($<$<C_COMPILER_ID:MSVC>:/utf-8>)
add_compile_options($<$<CXX_COMPILER_ID:MSVC>:/utf-8>)

# use MFC library in shared
set(CMAKE_MFC_FLAG 2)

# generic configuration
string(CONCAT CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded"
  $<$<CONFIG:Debug>:DebugDLL>
  $<$<NOT:$<CONFIG:Debug>>:DLL>)
string(CONCAT BUILD_PREFIX_PATH
  $<$<CONFIG:Debug>:${PACKAGE_PREFIX_DIR}/debug>
  $<$<NOT:$<CONFIG:Debug>>:${PACKAGE_PREFIX_DIR}>)
target_link_options(${TARGET_MFC}
  PRIVATE
    $<$<NOT:$<CONFIG:Debug>>:/INCREMENTAL:NO>)

# link third-party libraries
target_link_libraries(${TARGET_MFC}
  PRIVATE 
    fmt::fmt-header-only
    winpp::winpp
    nlohmann_json::nlohmann_json)

# compress executable using upx
if(NOT CMAKE_BUILD_TYPE STREQUAL "Debug")
  if(EXISTS ${CMAKE_INSTALL_PREFIX}/bin/upx.exe)
    add_custom_command(
      TARGET ${TARGET_MFC} POST_BUILD 
      COMMAND ${CMAKE_INSTALL_PREFIX}/bin/upx.exe -9q $<TARGET_FILE:${TARGET_MFC}>)
  endif()
endif()

# install executable
install(TARGETS ${TARGET_MFC} EXPORT ${TARGET_MFC}Config
  RUNTIME DESTINATION ${CMAKE_INSTALL_PREFIX}/bin)

# install executable dependencies
install(CODE [[
    file(GET_RUNTIME_DEPENDENCIES 
        DIRECTORIES ${BUILD_PREFIX_PATH}/bin
        PRE_EXCLUDE_REGEXES "api-ms-*"
        POST_EXCLUDE_REGEXES ".*system32/.*\\.dll"
        RESOLVED_DEPENDENCIES_VAR deps
        EXECUTABLES ${CMAKE_ABSOLUTE_DESTINATION_FILES})
    message(STATUS ${deps})
    file(INSTALL DESTINATION ${CMAKE_INSTALL_PREFIX}/bin TYPE EXECUTABLE FILES ${deps})
]])

# organize files for visual-studio
set_property(GLOBAL PROPERTY USE_FOLDERS ON)
source_group("Headers Files" FILES ${HEADER_FILES})
source_group("Resources Files" FILES ${RESOURCE_FILES})
source_group("Sources Files" FILES ${SOURCE_FILES})