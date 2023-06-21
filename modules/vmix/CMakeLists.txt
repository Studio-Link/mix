project(vmix)

set(SRCS vmix.c src.c disp.c record.c codec.c)

include_directories(
    ../../include
)

if(STATIC)
    add_library(${PROJECT_NAME} OBJECT ${SRCS})
else()
    add_library(${PROJECT_NAME} MODULE ${SRCS})
endif()

target_link_libraries(${PROJECT_NAME} PRIVATE slmix-lib)
