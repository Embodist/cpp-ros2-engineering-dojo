# Ported cpp_testing reference projects inside Stage 1.
# Build only when -DDOJO_BUILD_PROJECTS=ON and -DDOJO_USE_GTEST=ON.

if(NOT DOJO_USE_GTEST)
  message(FATAL_ERROR "DOJO_BUILD_PROJECTS=ON requires DOJO_USE_GTEST=ON")
endif()

find_package(Threads REQUIRED)

add_library(project_common STATIC common/src/new_counter.cpp)
target_include_directories(project_common PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(project_common PUBLIC dojo_warnings Threads::Threads)

file(GLOB common_tests CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/common/tests/*.cpp)
if(common_tests)
  add_executable(project_common_tests ${common_tests})
  target_link_libraries(project_common_tests PRIVATE project_common GTest::gtest_main)
  if(DOJO_SANITIZE_THREAD AND CMAKE_SYSTEM_NAME STREQUAL "Linux")
    set_target_properties(project_common_tests PROPERTIES
      TEST_LAUNCHER "setarch;${CMAKE_SYSTEM_PROCESSOR};-R")
  endif()
  gtest_discover_tests(project_common_tests
    TEST_PREFIX "projects.common."
    PROPERTIES LABELS "projects" TIMEOUT 30)
endif()

function(add_port_project name)
  set(dir ${CMAKE_CURRENT_SOURCE_DIR}/reference/${name})
  if(NOT IS_DIRECTORY ${dir})
    return()
  endif()
  file(GLOB srcs CONFIGURE_DEPENDS ${dir}/src/*.cpp)
  file(GLOB tests CONFIGURE_DEPENDS ${dir}/tests/*.cpp)
  if(NOT srcs AND NOT tests)
    return()
  endif()

  if(srcs)
    add_library(${name} STATIC ${srcs})
    target_include_directories(${name} PUBLIC ${dir}/include)
    target_link_libraries(${name} PUBLIC dojo_warnings project_common Threads::Threads)
  endif()

  if(tests)
    add_executable(${name}_tests ${tests})
    target_include_directories(${name}_tests PRIVATE ${dir}/include)
    if(TARGET ${name})
      target_link_libraries(${name}_tests PRIVATE ${name} project_common GTest::gtest_main Threads::Threads)
    else()
      target_link_libraries(${name}_tests PRIVATE project_common GTest::gtest_main Threads::Threads)
    endif()
    if(DOJO_SANITIZE_THREAD AND CMAKE_SYSTEM_NAME STREQUAL "Linux")
      set_target_properties(${name}_tests PROPERTIES
        TEST_LAUNCHER "setarch;${CMAKE_SYSTEM_PROCESSOR};-R")
    endif()
    gtest_discover_tests(${name}_tests
      TEST_PREFIX "projects.${name}."
      PROPERTIES LABELS "projects" TIMEOUT 60)
  endif()
endfunction()

add_port_project(p01_student)
add_port_project(p02_smart_ptr)
add_port_project(p03_string)
add_port_project(p04_list)
add_port_project(p05_vector)
add_port_project(p06_arena)
add_port_project(p07_function)
add_port_project(p08_thread_queue)
add_port_project(p09_spsc_queue)
add_port_project(p10_pubsub)
add_port_project(p11_realtime)

# 能力链：p06 的测试把分配器接进 p04/p05 的容器。
if(TARGET p06_arena_tests)
  target_include_directories(p06_arena_tests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/reference/p04_list/include
    ${CMAKE_CURRENT_SOURCE_DIR}/reference/p05_vector/include)
endif()

# 能力链：p08 的任务类型用 p07 的 MoveOnlyFunction。
if(TARGET p08_thread_queue_tests)
  target_include_directories(p08_thread_queue_tests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/reference/p07_function/include)
endif()

# 能力链：p11 控制环用 p06 的 Arena + p09 的 SPSC 队列。
if(TARGET p11_realtime_tests)
  target_include_directories(p11_realtime_tests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/reference/p06_arena/include
    ${CMAKE_CURRENT_SOURCE_DIR}/reference/p09_spsc_queue/include)
endif()
