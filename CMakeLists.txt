file(GLOB_RECURSE SOURCES "src/*.*")
idf_component_register(
    SRCS ${SOURCES}
    INCLUDE_DIRS src
    REQUIRES M5Stack ezTime
)

list(APPEND DEFINITIONS "ARDUINO_TTGO_T1")

target_compile_definitions(${COMPONENT_TARGET} PUBLIC ${DEFINITIONS})
