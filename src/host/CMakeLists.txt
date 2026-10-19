# lodsc — the thin host executable (build-system.md §1).
#
# The app is named `lodsc` — LoD: Severed Chains (the game: The Legend of
# Dragoon; the project: Severed Chains).
add_executable(lodsc main.cpp)
target_link_libraries(lodsc PRIVATE scengine)
target_link_libraries(lodsc PRIVATE sc_noexcept sc_warnings)
