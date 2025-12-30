cmake_minimum_required(VERSION 3.16)
project(ChatServer LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(SRC_LIST
    main.cpp
    server.cpp
    session.cpp
    logger.cpp
    user_store.cpp
    message_store.cpp
)
set(HDR_LIST
    db_pool.hpp
    logger.hpp
    protocol.hpp
    server.hpp
    session.hpp
    user_store.hpp
    message_store.hpp
)

# ========== 依赖查找 ==========
find_package(Boost REQUIRED COMPONENTS system thread)

find_package(nlohmann_json QUIET)
if(NOT nlohmann_json_FOUND)
    include(FetchContent)
    FetchContent_Declare(
      json
      URL https://github.com/nlohmann/json/releases/download/v3.11.3/json.hpp
    )
    FetchContent_GetProperties(json)
    if(NOT json_POPULATED)
      FetchContent_Populate(json)
    endif()
    set(NLOHMANN_JSON_INCLUDE_DIR "${json_SOURCE_DIR}")
else()
    set(NLOHMANN_JSON_INCLUDE_DIR "${nlohmann_json_INCLUDE_DIRS}")
endif()

# ==== MySQL Connector/C++ 8.0.32 路径 ====
set(MYSQL_CONNECTOR_CPP_INCLUDE_DIR "D:/tools/mysql-connector-c++-8.0.32-winx64/include")
set(MYSQL_CONNECTOR_CPP_LIB_DIR "D:/tools/mysql-connector-c++-8.0.32-winx64/lib64/vs14")

add_executable(chatserver ${SRC_LIST} ${HDR_LIST})

target_include_directories(chatserver PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${NLOHMANN_JSON_INCLUDE_DIR}
    "D:/tools/vcpkg/installed/x64-windows/include"
    ${MYSQL_CONNECTOR_CPP_INCLUDE_DIR}
)

target_link_directories(chatserver PRIVATE
    "D:/tools/vcpkg/installed/x64-windows/lib"
    ${MYSQL_CONNECTOR_CPP_LIB_DIR}
)

target_link_libraries(chatserver PRIVATE
    Boost::system
    Boost::thread
    nlohmann_json::nlohmann_json
    mysqlcppconn8
)

target_compile_definitions(chatserver PRIVATE
    BOOST_ASIO_NO_DEPRECATED
    BOOST_ASIO_DISABLE_STD_STRING_VIEW
)

# -------- MSVC警告屏蔽（强制所有C4996、C4005） --------
if(MSVC)
    target_compile_options(chatserver PRIVATE /wd4996 /wd4005)
endif()

install(TARGETS chatserver DESTINATION bin)

# -------- 自动DLL拷贝到输出目录 --------
set(MYSQL_DLL_DIR "D:/tools/mysql-connector-c++-8.0.32-winx64/lib64")
set(MYSQL_DLL_LIST
    "${MYSQL_DLL_DIR}/mysqlcppconn8-2-vs14.dll"
    "${MYSQL_DLL_DIR}/libcrypto-1_1-x64.dll"
    "${MYSQL_DLL_DIR}/libssl-1_1-x64.dll"
)

add_custom_command(TARGET chatserver POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    ${MYSQL_DLL_LIST}
    $<TARGET_FILE_DIR:chatserver>
    COMMENT "Auto copying MySQL DLLs to output directory"
)

# ---------- DLL运行说明 -----------
# 运行 chatserver.exe 时会自动拷贝 mysqlcppconn8-2-vs14.dll、libssl-1_1-x64.dll、libcrypto-1_1-x64.dll 到输出目录
# 如Debug用不同DLL版本，只需在 MYSQL_DLL_LIST 里替换对应 dll 路径即可