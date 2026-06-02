# CMake generated Testfile for 
# Source directory: /home/smorod1na/kursovaia_Sinelnikov/tests
# Build directory: /home/smorod1na/kursovaia_Sinelnikov/tests
# 
# This file includes the relevant testing commands required for 
# testing this directory and lists subdirectories to be tested as well.
include("/home/smorod1na/kursovaia_Sinelnikov/tests/test_my_player_unit[1]_include.cmake")
add_test([=[smoke_test_player]=] "./test_my_player")
set_tests_properties([=[smoke_test_player]=] PROPERTIES  _BACKTRACE_TRIPLES "/home/smorod1na/kursovaia_Sinelnikov/tests/CMakeLists.txt;6;add_test;/home/smorod1na/kursovaia_Sinelnikov/tests/CMakeLists.txt;0;")
add_test([=[test_player_stats]=] "./test_stats")
set_tests_properties([=[test_player_stats]=] PROPERTIES  _BACKTRACE_TRIPLES "/home/smorod1na/kursovaia_Sinelnikov/tests/CMakeLists.txt;10;add_test;/home/smorod1na/kursovaia_Sinelnikov/tests/CMakeLists.txt;0;")
