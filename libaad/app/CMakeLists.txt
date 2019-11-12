set(aad_app_sources
    ParamsGenTrapdoors.cpp
    ParamsGenPowers.cpp
    ParamsFix.cpp
    ParamsValidate.cpp
)

foreach(appSrc ${aad_app_sources})
    get_filename_component(appName ${appSrc} NAME_WE)
    set(appDir ../bin/app)

    add_executable(${appName} ${appSrc})
    target_link_libraries(${appName} PUBLIC aad)

    set_target_properties(${appName} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${appDir})
endforeach()

#install(TARGETS PlayApp DESTINATION bin)
