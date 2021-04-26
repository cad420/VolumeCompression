option(BUILD_TEST "whether build test" ON)
include(FetchContent)
FetchContent_Declare(
        VolumeReader
        GIT_REPOSITORY https://github.com/wyzwzz/VolumeReader.git
        #        GIT_TAG 3.0.0.0
        GIT_SHALLOW TRUE
        GIT_PROGRESS TRUE
)
FetchContent_MakeAvailable(VolumeReader)