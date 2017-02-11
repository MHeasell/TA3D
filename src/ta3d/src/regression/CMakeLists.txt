
#
# AutoTest, if enabled
#
if(${TA3D_REGRESSION_TEST} EQUAL 1)
    
    # Strings
    add_executable(regression-string  string.cpp)
    target_link_libraries(regression-string stdafx logs threads misc)

    # TDF Parser
    add_executable(regression-tdfparser  tdf.cpp)
    target_link_libraries(regression-tdfparser stdafx tdf logs threads misc)

endif()

