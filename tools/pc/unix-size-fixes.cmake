
set(PC_UNIX_FIX_DIR "${CMAKE_CURRENT_BINARY_DIR}/unix-size-fixes")

# uint64_t return type -> unsigned long long
set(PC_UNIX_FIX_RETURN_TYPES
    GetType
    GetSpectateEntity
    GetOwnerPrincipalId
)

set(PC_UNIX_FIX_REPLACEMENTS
    # wut declares its 64-bit atomics with uint64_t, which is 'unsigned long' on LP64
    "vendor/MilesRR.cpp|OSCompareAndSwapAtomicEx64(pTarget, uComparand, uNewValue, &uPrevious)|OSCompareAndSwapAtomicEx64(reinterpret_cast<volatile uint64_t*>(pTarget), static_cast<uint64_t>(uComparand), static_cast<uint64_t>(uNewValue), reinterpret_cast<uint64_t*>(&uPrevious))"

    "vendor/MilesRR.cpp|OSAddAtomic64(pTarget, lValue)|OSAddAtomic64(reinterpret_cast<volatile int64_t*>(pTarget), static_cast<int64_t>(lValue))"
    # Struct sizes differ on 64-bit
    "nn/util/util_StaticAssert.h|#define NN_STATIC_ASSERT(condition) BOOST_STATIC_ASSERT(condition)|#define NN_STATIC_ASSERT(condition)"
)

function(pc_unix_size_fixes_text variable rel)
    set(text "${${variable}}")

    if("${text}" MATCHES "uint64_t")
        string(REPLACE ";" "|" names "${PC_UNIX_FIX_RETURN_TYPES}")
        set(pattern "(^|[^A-Za-z0-9_])uint64_t([ \t]+([A-Za-z_][A-Za-z0-9_]*::)*(${names})[ \t]*\\()")
        while("${text}" MATCHES "${pattern}")
            string(REGEX REPLACE "${pattern}" "\\1unsigned long long\\2" text "${text}")
        endwhile()
    endif()

    foreach(entry IN LISTS PC_UNIX_FIX_REPLACEMENTS)
        string(REPLACE "|" ";" parts "${entry}")
        list(GET parts 0 file)
        if(file STREQUAL rel)
            list(GET parts 1 from)
            list(GET parts 2 to)
            string(REPLACE "${from}" "${to}" text "${text}")
        endif()
    endforeach()

    set(${variable} "${text}" PARENT_SCOPE)
endfunction()

function(pc_unix_size_fixes_apply)
    file(GLOB_RECURSE candidates "${SOURCE_ROOT}/*.cpp" "${SOURCE_ROOT}/*.hpp" "${SOURCE_ROOT}/*.h" "${SOURCE_ROOT}/*.c" "${SOURCE_ROOT}/*.inc")
    list(FILTER candidates EXCLUDE REGEX "/platform/pc/compat/|/platform/wiiu/shaders/|/vendor/dlmalloc\\.cpp$")

    set(fixed_files)
    set(fixed_paths)
    set(original_dirs)
    foreach(path IN LISTS candidates)
        file(RELATIVE_PATH rel "${SOURCE_ROOT}" "${path}")
        file(READ "${path}" original)
        set(text "${original}")
        pc_unix_size_fixes_text(text "${rel}")
        if("${text}" STREQUAL "${original}")
            continue()
        endif()
        set(out "${PC_UNIX_FIX_DIR}/${rel}")
        set(old "")
        if(EXISTS "${out}")
            file(READ "${out}" old)
        endif()
        if(NOT "${old}" STREQUAL "${text}")
            file(WRITE "${out}" "${text}")
        endif()
        list(APPEND fixed_files "${rel}")
        list(APPEND fixed_paths "${path}")
        get_filename_component(dir "${path}" DIRECTORY)
        list(APPEND original_dirs "${dir}")
    endforeach()
    list(REMOVE_DUPLICATES original_dirs)

    foreach(entry IN LISTS PC_UNIX_FIX_REPLACEMENTS)
        string(REPLACE "|" ";" parts "${entry}")
        list(GET parts 0 file)
        if(NOT file IN_LIST fixed_files)
            message(WARNING "unix size fixes: rule for ${file} did not apply")
        endif()
    endforeach()

    set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS ${fixed_paths})

    set(PC_UNIX_FIXED_FILES "${fixed_files}" PARENT_SCOPE)
    set(PC_UNIX_FIX_INCLUDE_FIRST "${PC_UNIX_FIX_DIR}" "${PC_UNIX_FIX_DIR}/platform/pc/overlay" PARENT_SCOPE)
    set(PC_UNIX_FIX_INCLUDE_LAST "${original_dirs}" PARENT_SCOPE)
    list(LENGTH fixed_files count)
    message(STATUS "unix size fixes: ${count} files fixed into ${PC_UNIX_FIX_DIR}")
endfunction()

# Swap entries of a source list for their fixed copies
function(pc_unix_size_fixes_use variable)
    set(result)
    foreach(src IN LISTS ${variable})
        file(RELATIVE_PATH rel "${SOURCE_ROOT}" "${src}")
        if(rel IN_LIST PC_UNIX_FIXED_FILES)
            list(APPEND result "${PC_UNIX_FIX_DIR}/${rel}")
        else()
            list(APPEND result "${src}")
        endif()
    endforeach()
    set(${variable} "${result}" PARENT_SCOPE)
endfunction()
