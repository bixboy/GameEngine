set_project("GameEngine")
set_version("0.1.0")

add_rules("mode.debug", "mode.release")
set_languages("c++20")
set_warnings("allextra")
set_optimize("faster")

option("sdl3_dir")
    set_default(os.getenv("SDL3_DIR"))
    set_showmenu(true)
    set_description("Root directory of the SDL3 SDK (expects include/ and lib/ subfolders)")
option_end()

option("sdl3_lib_dir")
    set_default(os.getenv("SDL3_LIB_DIR"))
    set_showmenu(true)
    set_description("Directory containing SDL3 libraries to link against")
option_end()

local function first_existing_dir(dirs)
    for _, dir in ipairs(dirs) do
        if dir and os.isdir(dir) then
            return dir
        end
    end
end

local function collect_sdl3_paths()
    local project_root = os.scriptdir()
    local vendor_root = path.join(project_root, "Libs", "SDL3-3.2.22")

    local sdl3_root = get_config("sdl3_dir") or os.getenv("SDL3_DIR")
    if not sdl3_root and os.isdir(vendor_root) then
        sdl3_root = vendor_root
    end

    local include_dir
    if sdl3_root then
        local candidate = path.join(sdl3_root, "include")
        if os.isdir(candidate) then
            include_dir = candidate
        end
    end

    local lib_dir = get_config("sdl3_lib_dir") or os.getenv("SDL3_LIB_DIR")
    local lib_candidates = {}
    if lib_dir then
        table.insert(lib_candidates, lib_dir)
    end
    if sdl3_root then
        table.insert(lib_candidates, path.join(sdl3_root, "lib"))
        table.insert(lib_candidates, path.join(sdl3_root, "lib", "x64"))
        table.insert(lib_candidates, path.join(sdl3_root, "lib", "x86_64"))
        table.insert(lib_candidates, path.join(sdl3_root, "lib", "Release"))
        table.insert(lib_candidates, path.join(sdl3_root, "lib64"))
    end
    table.insert(lib_candidates, path.join(project_root, "x64", "Release"))
    table.insert(lib_candidates, path.join(project_root, "x64", "Debug"))

    lib_dir = first_existing_dir(lib_candidates)

    return include_dir, lib_dir
end

local sdl3_include_dir, sdl3_lib_dir = collect_sdl3_paths()

if sdl3_include_dir then
    add_includedirs(sdl3_include_dir, {public = true})
else
    wprint("SDL3 include directory not found. Set SDL3_DIR or --sdl3_dir when configuring.")
end

if sdl3_lib_dir then
    add_linkdirs(sdl3_lib_dir)
else
    wprint("SDL3 library directory not found. Set SDL3_LIB_DIR or --sdl3_lib_dir when configuring.")
end

add_defines("SDL_MAIN_HANDLED", {public = true})

if is_plat("windows") then
    add_syslinks("user32", "gdi32", "shell32", "ole32", "oleaut32", "version", "winmm", "imm32", "advapi32")
else
    add_syslinks("pthread", "dl")
end

-- ImGui renderer integration -------------------------------------------------
target("imgui")
    set_kind("static")
    add_includedirs("Libs/ImGui", "Libs/ImGui/backends", {public = true})
    add_headerfiles("Libs/ImGui/**.h")
    add_files(
        "Libs/ImGui/imgui.cpp",
        "Libs/ImGui/imgui_draw.cpp",
        "Libs/ImGui/imgui_widgets.cpp",
        "Libs/ImGui/imgui_tables.cpp",
        "Libs/ImGui/imgui_demo.cpp",
        "Libs/ImGui/backends/imgui_impl_sdl3.cpp",
        "Libs/ImGui/backends/imgui_impl_sdlrenderer3.cpp"
    )

-- Engine core ----------------------------------------------------------------
target("engine")
    set_kind("static")
    add_deps("imgui")
    add_headerfiles(
        "Engine/Source/**/Public/**.h",
        "Engine/Source/**/Public/**.inl"
    )
    add_includedirs(
        "Engine/Source/Core/Public",
        "Engine/Source/Engine/Public",
        "Engine/Source/Game/Public",
        "Engine/Source/Graphics/Public",
        "Engine/Source/Input/Public",
        "Engine/Source/Math/Public"
    , {public = true})
    add_files("Engine/Source/**.cpp")
    add_links("SDL3", {public = true})

-- Game executable ------------------------------------------------------------
target("GameEngine")
    set_kind("binary")
    add_deps("engine")
    add_files("GameEngine/Exemple/main.cpp")
