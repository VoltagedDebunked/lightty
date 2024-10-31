cmake_minimum_required(VERSION 3.10)
project(lightty C)

find_package(PkgConfig REQUIRED)
pkg_check_modules(VTE REQUIRED vte-2.91)
pkg_check_modules(GTK REQUIRED gtk+-3.0)
pkg_check_modules(GLIB REQUIRED glib-2.0)

add_executable(lightty src/main.c)
target_include_directories(lightty PRIVATE ${VTE_INCLUDE_DIRS} ${GTK_INCLUDE_DIRS} ${GLIB_INCLUDE_DIRS})
target_link_libraries(lightty ${VTE_LIBRARIES} ${GTK_LIBRARIES} ${GLIB_LIBRARIES})
target_compile_options(lightty PRIVATE ${VTE_CFLAGS_OTHER} ${GTK_CFLAGS_OTHER} ${GLIB_CFLAGS_OTHER})
