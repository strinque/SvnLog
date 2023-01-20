cmake_minimum_required(VERSION 3.13)

# compress executable with upx
if(EXECUTE_POST_BUILD)
    if(SELF_PACKER_FOR_EXECUTABLE)
        execute_process(COMMAND ${SELF_PACKER_FOR_EXECUTABLE} -9q ${TARGET_FILE})
    endif()
endif()