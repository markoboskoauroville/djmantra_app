# DJ Mantra: assembles the Android package source directory (Qt's
# QT_ANDROID_PACKAGE_SOURCE_DIR) in the build tree:
#   - packaging/android/package (manifest, icon, Java helpers)
#   - assets/res: the resources the app reads at run time (skins, controller
#     mappings, effects, fonts, keyboard mappings, QML, translations), plus an
#     index and a stamp for src/util/bundledresources.cpp
#
# cmake -DSOURCE_DIR=<repo> -DPACKAGE_DIR=<build>/android-package -P stage_android_package.cmake

if(NOT SOURCE_DIR OR NOT PACKAGE_DIR)
  message(FATAL_ERROR "SOURCE_DIR and PACKAGE_DIR are required")
endif()

file(COPY "${SOURCE_DIR}/packaging/android/package/" DESTINATION "${PACKAGE_DIR}")

set(_res "${PACKAGE_DIR}/assets/res")
file(REMOVE_RECURSE "${_res}")
file(MAKE_DIRECTORY "${_res}")
foreach(_dir skins controllers effects fonts keyboard qml)
  if(EXISTS "${SOURCE_DIR}/res/${_dir}")
    # Files the APK tools would drop (hidden files) are left out
    file(COPY "${SOURCE_DIR}/res/${_dir}" DESTINATION "${_res}" PATTERN ".*" EXCLUDE)
  endif()
endforeach()
file(COPY "${SOURCE_DIR}/res/translations" DESTINATION "${_res}"
  FILES_MATCHING PATTERN "*.qm")

file(GLOB_RECURSE _files RELATIVE "${_res}" "${_res}/*")
list(SORT _files)
string(JOIN "\n" _index ${_files})
file(WRITE "${_res}/djmantra-res.index" "${_index}\n")
list(LENGTH _files _count)

# Changes with every build, so a new APK always refreshes the copy on the phone
string(TIMESTAMP _now "%Y%m%dT%H%M%S" UTC)
string(SHA1 _hash "${_index}")
file(WRITE "${_res}/djmantra-res.stamp" "${_now}-${_hash}\n")
message(STATUS "Android package: ${_count} resource files in assets/res")
