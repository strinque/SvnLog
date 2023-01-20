cmake_minimum_required(VERSION 3.13)
set(TARGET_MFC "SvnLog" CACHE INTERNAL "")

# set required c++ version
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# set project source-files
set(SOURCE_FILES
  SvnLog.cpp
  SvnLogDlg.cpp
  SvnRepos.cpp
  pch.cpp)
set(HEADER_FILES
  SvnLog.h
  SvnLogDlg.h
  SvnRepos.h
  framework.h
  pch.h
  Resource.h
  targetver.h)
set(RESOURCE_FILES
  SvnLog.rc
  res/program.ico
  res/SvnLog.rc2)
  
if(MSVC)
  # force utf-8 encoding for source-files
  add_compile_options("$<$<C_COMPILER_ID:MSVC>:/utf-8>")
  add_compile_options("$<$<CXX_COMPILER_ID:MSVC>:/utf-8>")

  # set unicode
  add_definitions(-DUNICODE)
  add_definitions(-D_UNICODE)

  # activate MFC in shared
  add_definitions(-D_AFXDLL)
  set(CMAKE_MFC_FLAG 2)
endif()

# compile executable
add_executable(${TARGET_MFC} WIN32 ${SOURCE_FILES} ${HEADER_FILES} ${RESOURCE_FILES})

# set the path to the third-party libraries include
target_include_directories(${TARGET_MFC} PRIVATE
  $<BUILD_INTERFACE:${CMAKE_PREFIX_PATH}/include>)

# link third-party libraries
find_package(libutf8 REQUIRED)
if(CMAKE_BUILD_TYPE MATCHES "Debug")
  target_link_directories(${TARGET_MFC} PRIVATE "${CMAKE_PREFIX_PATH}/debug/lib")
  target_link_libraries(${TARGET_MFC} 
    PRIVATE libutf8d.lib)
else()
  target_link_directories(${TARGET_MFC} PRIVATE "${CMAKE_PREFIX_PATH}/lib")
  target_link_libraries(${TARGET_MFC}
    PRIVATE libutf8.lib)
endif()

# fix mfc winmain link issue for mfc unicode program
set_target_properties(${TARGET_MFC} PROPERTIES LINK_FLAGS "/SUBSYSTEM:WINDOWS")
target_link_options(${TARGET_MFC} PRIVATE "/entry:wWinMainCRTStartup")

# rename executable
set_target_properties(${TARGET_MFC} PROPERTIES OUTPUT_NAME "SvnLog")

# rename and pack executable
add_custom_command(
  TARGET ${TARGET_MFC} 
  POST_BUILD 
    COMMAND ${CMAKE_COMMAND} 
      -D EXECUTE_POST_BUILD=$<CONFIG:Release>
      -D TARGET_FILE="$<TARGET_FILE:${TARGET_MFC}>"
      -D SELF_PACKER_FOR_EXECUTABLE="${CMAKE_PREFIX_PATH}/bin/upx.exe"
      -P ${CMAKE_SOURCE_DIR}/cmake/PackExePostBuild.cmake)

# install files
install(TARGETS ${TARGET_MFC} EXPORT ${TARGET_MFC}Config
  RUNTIME DESTINATION bin)

# organize files for visual-studio
set_property(GLOBAL PROPERTY USE_FOLDERS ON)
source_group("Resource File/" FILES
  ${RESOURCE_FILES})
source_group("Header File/" FILES
  ${HEADER_FILES})
source_group("Source File/" FILES
  ${SOURCE_FILES})
