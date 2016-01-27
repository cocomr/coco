# ===================================================================================
#  coco CMake configuration file
#
#             ** File generated automatically, do not modify **
#
#  Usage from an external project:
#    In your CMakeLists.txt, add these lines:
#
#    FIND_PACKAGE(coco REQUIRED )
#    TARGET_LINK_LIBRARIES(MY_TARGET_NAME )
#
#    This file will define the following variables:
#      - coco_LIBS          : The list of libraries to links against.
#      - coco_LIB_DIR       : The directory where lib files are. Calling LINK_DIRECTORIES
#                                with this path is NOT needed.
#      - coco_VERSION       : The  version of this PROJECT_NAME build. Example: "1.2.0"
#      - coco_VERSION_MAJOR : Major version part of VERSION. Example: "1"
#      - coco_VERSION_MINOR : Minor version part of VERSION. Example: "2"
#      - coco_VERSION_PATCH : Patch version part of VERSION. Example: "0"
#
# ===================================================================================
INCLUDE_DIRECTORIES("/usr/local/include")
SET(coco_INCLUDE_DIRS "/usr/local/include")

LINK_DIRECTORIES("/usr/local/lib")
SET(coco_LIB_DIR "/usr/local/lib")

SET(coco_LIBS  coco)

SET(coco_FOUND 1)
SET(coco_VERSION        0.5.1)
SET(coco_VERSION_MAJOR  0)
SET(coco_VERSION_MINOR  5)
SET(coco_VERSION_PATCH  1)
