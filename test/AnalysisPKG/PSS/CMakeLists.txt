#-------------------------------------------------------------------------
#   Copyright 2002-2025 National Technology & Engineering Solutions of
#   Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
#   NTESS, the U.S. Government retains certain rights in this software.
#
#   This file is part of the Xyce(TM) Parallel Electrical Simulator.
#
#   Xyce(TM) is free software: you can redistribute it and/or modify
#   it under the terms of the GNU General Public License as published by
#   the Free Software Foundation, either version 3 of the License, or
#   (at your option) any later version.
#
#   Xyce(TM) is distributed in the hope that it will be useful,
#   but WITHOUT ANY WARRANTY; without even the implied warranty of
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#   GNU General Public License for more details.
#
#   You should have received a copy of the GNU General Public License
#   along with Xyce(TM).
#   If not, see <http://www.gnu.org/licenses/>.
#-------------------------------------------------------------------------

if(GTest_FOUND)
  # PSS Shooting Method Unit Tests (Phase 0)
  add_executable(PSS_ShootingMethod_UnitTests
    N_ANP_PSSShooting_UnitTest.C
    PSSShootingMethod.C
    SimpleODETest.C
  )
  
  target_link_libraries(PSS_ShootingMethod_UnitTests PUBLIC GTest::gtest)
  
  target_include_directories(PSS_ShootingMethod_UnitTests 
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}
    PUBLIC $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}>/src
  )
  
  # Wrap the GTest tests with CTest
  gtest_discover_tests(
    PSS_ShootingMethod_UnitTests
    TEST_PREFIX PSS_ShootingMethod:
    PROPERTIES "LABELS;nightly;LABELS;unittest;LABELS;pss"
  )

  # PSS Integration Tests (Phase 2)
  # These tests use the full Xyce infrastructure via Simulator API
  add_executable(PSS_Integration_UnitTests
    N_ANP_PSSIntegration_UnitTest.C
  )
  
  target_link_libraries(PSS_Integration_UnitTests PUBLIC 
    GTest::gtest
    XyceLib
  )
  
  target_include_directories(PSS_Integration_UnitTests 
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}
    PUBLIC $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}>/src
  )
  
  # Copy test netlists to build directory so tests can find them
  configure_file(${CMAKE_CURRENT_SOURCE_DIR}/TestNetlist_RC.cir 
                 ${CMAKE_CURRENT_BINARY_DIR}/TestNetlist_RC.cir COPYONLY)
  configure_file(${CMAKE_CURRENT_SOURCE_DIR}/TestNetlist_RLC.cir 
                 ${CMAKE_CURRENT_BINARY_DIR}/TestNetlist_RLC.cir COPYONLY)
  
  # Set working directory for tests to find netlists
  set_tests_properties(PSS_Integration_UnitTests PROPERTIES
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  )
  
  # Wrap the GTest tests with CTest
  gtest_discover_tests(
    PSS_Integration_UnitTests
    TEST_PREFIX PSS_Integration:
    PROPERTIES "LABELS;nightly;LABELS;unittest;LABELS;pss;LABELS;integration"
  )
endif()

