set(aad_test_sources
    TestAAD.cpp
    TestAccumulatedTree.cpp
    TestAssumptions.cpp
    TestBinaryTree.cpp
    TestBitString.cpp
    TestFrontier.cpp
    TestGroupElementSize.cpp
    TestGroup.cpp
    TestPolyDivision.cpp
    TestPublicParams.cpp
)

foreach(appSrc ${aad_test_sources})
    get_filename_component(appName ${appSrc} NAME_WE)
    set(appDir ../bin/test)

    add_executable(${appName} ${appSrc})
    target_link_libraries(${appName} PUBLIC aad)

    add_test(NAME ${appName} COMMAND ${appName})
    set_target_properties(${appName} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${appDir})
endforeach()
