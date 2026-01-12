set(srcs "./cJSONx.c" "./cJSON.c")
set(include_dirs "${CMAKE_CURRENT_LIST_DIR}/." "...")
set(requires json)

idf_component_register(SRCS "${srcs}"
                    INCLUDE_DIRS "${include_dirs}"
                    REQUIRES ${requires})