# severed-chains — the thin host executable (build-system.md §1).
add_executable(severed-chains main.cpp)
target_link_libraries(severed-chains PRIVATE scengine)
target_link_libraries(severed-chains PRIVATE sc_noexcept sc_warnings)