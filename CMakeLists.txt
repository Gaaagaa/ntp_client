# ====================================================================

cmake_minimum_required(VERSION 2.8)

project(ntp_cli)

# ====================================================================

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)

include_directories(src)

# ====================================================================
# xtime

add_executable(xtime src/xtime.c test/xtime_test.c)
if (WIN32)
    target_link_libraries(xtime kernel32.lib)
endif ()

# ====================================================================
# xtime

add_executable(zeller src/xtime.c test/zeller_test.c)
if (WIN32)
    target_link_libraries(zeller kernel32.lib)
endif ()

# ====================================================================
# ntp_cli

add_executable(ntp_cli src/xtime.c src/ntp_client.c test/ntp_test.c)
if (WIN32)
    target_link_libraries(ntp_cli ws2_32.lib kernel32.lib)
endif ()

# ====================================================================

