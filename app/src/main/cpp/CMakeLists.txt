cmake_minimum_required(VERSION 3.22.1)

project("projectgen")

add_library(
  projectgen
  SHARED
  project_generator.cpp
  zip_writer.cpp
)

target_include_directories(projectgen PRIVATE .)

find_library(log-lib log)

target_link_libraries(
  projectgen
  ${log-lib}
  z
)
