cmake_minimum_required (VERSION 2.8)
    project (octopus)

    set (CMAKE_CXX_FLAGS "-g -Wall")
    set (CMAKE_C_FLAGS "-DTHREADED -g -Wall")
    add_executable (zookeeper_register zookeeper_register.c config.c)

    target_link_libraries (zookeeper_register zookeeper_mt procps czmq)

    add_executable (zookeeper_unregister zookeeper_unregister.c)

    target_link_libraries (zookeeper_unregister zookeeper_mt procps)

    add_executable (zookeeper_global_init zookeeper_global_init.c)

    target_link_libraries (zookeeper_global_init zookeeper_mt procps)

    add_executable (zookeeper_control zookeeper_control.c)

    target_link_libraries (zookeeper_control zookeeper_mt)

    add_subdirectory (MurmurHash)

    add_subdirectory (gproject)

    include_directories ("${PROJECT_SOURCE_DIR}/MurmurHash")

    include_directories ("${PROJECT_SOURCE_DIR}/tree")

    include_directories ("${PROJECT_SOURCE_DIR}/hash")
   
    include_directories ("${PROJECT_SOURCE_DIR}/api")

    add_executable (node_exec
                hkey.c
                action.c
                actions.c
                events.c
                event.c
                interval.c
                intervals.c
                zk_common.c
                config.c
                localdb.c
                node.c
                nodes.c
                router.c
                sleep.c
                worker.c
                workers.c
                compute.c
                balance.c
                zk_sync.c
                on_give.c on_receive.c update.c zookeeper.c zk_updater.c main.c)

    target_link_libraries (node_exec MurmurHash gproject zookeeper_mt procps czmq leveldb
                       tcmalloc)
