cmake_minimum_required (VERSION 2.8)
project (octopus)

set(CMAKE_CXX_FLAGS "-g -Wall")
set(CMAKE_C_FLAGS "-DTHREADED -g -Wall")
add_executable(zookeeper_register zookeeper_register.c config.c)

target_link_libraries(zookeeper_register zookeeper_mt proc czmq)

add_executable(zookeeper_unregister zookeeper_unregister.c)

target_link_libraries(zookeeper_unregister zookeeper_mt proc )

add_executable(zookeeper_global_init zookeeper_global_init.c)

target_link_libraries(zookeeper_global_init zookeeper_mt proc )

add_executable(zookeeper_control zookeeper_control.c)

target_link_libraries(zookeeper_control zookeeper_mt proc )


add_subdirectory(MurmurHash)


include_directories ("${PROJECT_SOURCE_DIR}/MurmurHash")

include_directories ("${PROJECT_SOURCE_DIR}/tree")

include_directories ("${PROJECT_SOURCE_DIR}/hash")

add_executable(node_exec 
aknowledgements.c
common.c
config.c
localdb.c
main.c
router.c
sleep.c
vertex.c
worker.c
db.c
worker_update.c
zookeeper.c
main.c
)

target_link_libraries(node_exec MurmurHash zookeeper_mt proc czmq leveldb)
