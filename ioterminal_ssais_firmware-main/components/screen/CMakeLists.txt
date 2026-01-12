set(srcs "screen_uart.c" "screen_driver.c" "screen_queue.c")
set(include_dirs "${CMAKE_CURRENT_LIST_DIR}/." "...")
set(requires driver)

idf_component_register(SRCS "screen_queue.c" "${srcs}"
                    INCLUDE_DIRS "${include_dirs}"
                    REQUIRES ${requires})