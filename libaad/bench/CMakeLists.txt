set(aad_bench_sources
    BenchPolyEea.cpp
    BenchPolyFromRoots.cpp
    BenchFrontierSize.cpp
    BenchHashing.cpp
    BenchGroupOps.cpp
    BenchMultiexp.cpp
    BenchNtlConv.cpp
    BenchPolyDiv.cpp
    BenchPolyMult.cpp
    macro/MacroBenchAADAppendOnlyProofs.cpp
    macro/MacroBenchAADAppends.cpp
    macro/MacroBenchAADLookupProofs.cpp
)

foreach(appSrc ${aad_bench_sources})
    get_filename_component(appName ${appSrc} NAME_WE)
    set(appDir ../bin/bench)

    add_executable(${appName} ${appSrc})
    target_link_libraries(${appName} PUBLIC aad)

    set_target_properties(${appName} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${appDir})
endforeach()
