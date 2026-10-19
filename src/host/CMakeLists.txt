# severed-chains — the thin host executable (build-system.md §1).
add_executable(severed-chains main.cpp)
target_link_libraries(severed-chains PRIVATE lodengine)
target_link_libraries(severed-chains PRIVATE lod_noexcept lod_warnings)