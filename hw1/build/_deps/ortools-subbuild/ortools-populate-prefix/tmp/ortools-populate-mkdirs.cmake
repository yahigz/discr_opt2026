# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file LICENSE.rst or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION ${CMAKE_VERSION}) # this file comes with cmake

# If CMAKE_DISABLE_SOURCE_CHANGES is set to true and the source directory is an
# existing directory in our source tree, calling file(MAKE_DIRECTORY) on it
# would cause a fatal error, even though it would be a no-op.
if(NOT EXISTS "/Users/higz/mipt/discr_opt/hw1/build/_deps/ortools-src")
  file(MAKE_DIRECTORY "/Users/higz/mipt/discr_opt/hw1/build/_deps/ortools-src")
endif()
file(MAKE_DIRECTORY
  "/Users/higz/mipt/discr_opt/hw1/build/_deps/ortools-build"
  "/Users/higz/mipt/discr_opt/hw1/build/_deps/ortools-subbuild/ortools-populate-prefix"
  "/Users/higz/mipt/discr_opt/hw1/build/_deps/ortools-subbuild/ortools-populate-prefix/tmp"
  "/Users/higz/mipt/discr_opt/hw1/build/_deps/ortools-subbuild/ortools-populate-prefix/src/ortools-populate-stamp"
  "/Users/higz/mipt/discr_opt/hw1/build/_deps/ortools-subbuild/ortools-populate-prefix/src"
  "/Users/higz/mipt/discr_opt/hw1/build/_deps/ortools-subbuild/ortools-populate-prefix/src/ortools-populate-stamp"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "/Users/higz/mipt/discr_opt/hw1/build/_deps/ortools-subbuild/ortools-populate-prefix/src/ortools-populate-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "/Users/higz/mipt/discr_opt/hw1/build/_deps/ortools-subbuild/ortools-populate-prefix/src/ortools-populate-stamp${cfgdir}") # cfgdir has leading slash
endif()
