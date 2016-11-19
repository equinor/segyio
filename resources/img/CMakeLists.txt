set(RESOURCES
    layouts_four_grid.png
    layouts_single.png
    layouts_three_bottom_grid.png
    layouts_three_horizontal_grid.png
    layouts_three_top_grid.png
    layouts_three_vertical_grid.png
    layouts_two_horizontal_grid.png
    layouts_two_vertical_grid.png
)

add_custom_target(install_resources ALL)

foreach (file ${RESOURCES})
    add_custom_command(TARGET install_resources
            COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/resources
            COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/${file} ${CMAKE_BINARY_DIR}/resources/img
            )
endforeach ()
get_target_property(INSTALL_PATH package_segyview PACKAGE_INSTALL_PATH)
install(FILES ${RESOURCES} DESTINATION ${INSTALL_PATH}/resources/img)
