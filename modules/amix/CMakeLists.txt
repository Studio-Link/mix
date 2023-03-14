project(amix)

set(SRCS amix.c record.c flac.c)

include_directories(
    ../../include
)

if(STATIC)
    add_library(${PROJECT_NAME} OBJECT ${SRCS})
else()
    add_library(${PROJECT_NAME} MODULE ${SRCS})
endif()

target_link_libraries(${PROJECT_NAME} PRIVATE FLAC slmix-lib)
