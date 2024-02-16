# Fetch Catch2 from GitHub
include(FetchContent)
FetchContent_Declare(
  catch2
  GIT_REPOSITORY https://github.com/catchorg/Catch2.git
  GIT_TAG v2.13.7 # Specify the desired version
)
FetchContent_MakeAvailable(catch2)

# Add an executable for the tests
add_executable(VertFragTests vertfrag_tests.cpp)

target_link_libraries(VertFragTests Catch2::Catch2)

enable_testing()
add_test(NAME VertFragTests COMMAND VertFragTests)
