set_project("BixEngine")
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

-- ---------------------------------------------------------------------------
-- SDL3 path resolution
-- ---------------------------------------------------------------------------

local function resolve_library_dir(candidates)
    local library_markers = {"SDL3.lib", "SDL3.dll", "libSDL3.a", "libSDL3.so"}
    local subdirs = {"", "x64", "Win64", "x86", "Win32", "x86_64"}
    for _, candidate in ipairs(candidates) do
        if candidate then
            for _, subdir in ipairs(subdirs) do
                local dir = candidate
                if subdir ~= "" then
                    dir = path.join(candidate, subdir)
                end
                if os.isdir(dir) then
                    for _, marker in ipairs(library_markers) do
                        if os.isfile(path.join(dir, marker)) then
                            return dir
                        end
                    end
                end
            end
        end
    end
end

local function collect_sdl3_paths()
    local project_root = os.scriptdir()
    local vendor_root = path.join(project_root, "ThirdParty", "SDL3-3.2.22")

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
        table.insert(lib_candidates, path.join(sdl3_root, "lib", "Win64"))
        table.insert(lib_candidates, path.join(sdl3_root, "lib", "x86"))
        table.insert(lib_candidates, path.join(sdl3_root, "lib", "Win32"))
        table.insert(lib_candidates, path.join(sdl3_root, "lib", "x86_64"))
        table.insert(lib_candidates, path.join(sdl3_root, "lib", "Release"))
        table.insert(lib_candidates, path.join(sdl3_root, "lib64"))
    end
    table.insert(lib_candidates, path.join(project_root, "Build", "Release"))
    table.insert(lib_candidates, path.join(project_root, "Build", "Debug"))

    lib_dir = resolve_library_dir(lib_candidates)

    return include_dir, lib_dir, sdl3_root
end

local sdl3_include_dir, sdl3_lib_dir, sdl3_root = collect_sdl3_paths()

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

-- ---------------------------------------------------------------------------
-- ImGui
-- ---------------------------------------------------------------------------

target("bixengine_imgui")
    set_kind("static")
    add_includedirs("ThirdParty/ImGui", "ThirdParty/ImGui/backends", {public = true})
    add_headerfiles("ThirdParty/ImGui/**.h")
    add_files(
        "ThirdParty/ImGui/imgui.cpp",
        "ThirdParty/ImGui/imgui_draw.cpp",
        "ThirdParty/ImGui/imgui_widgets.cpp",
        "ThirdParty/ImGui/imgui_tables.cpp",
        "ThirdParty/ImGui/imgui_demo.cpp",
        "ThirdParty/ImGui/backends/imgui_impl_sdl3.cpp",
        "ThirdParty/ImGui/backends/imgui_impl_sdlrenderer3.cpp"
    )

-- ---------------------------------------------------------------------------
-- Engine Core
-- ---------------------------------------------------------------------------

target("bixengine_runtime")
    set_kind("static")
    add_deps("bixengine_imgui")
    add_headerfiles("Runtime/Source/**/Public/**.h", "Runtime/Source/**/Public/**.inl")
    add_includedirs(
        "Runtime/Source/Core/Public",
        "Runtime/Source/Engine/Public",
        "Runtime/Source/Game/Public",
        "Runtime/Source/Graphics/Public",
        "Runtime/Source/Input/Public",
        "Runtime/Source/Math/Public",
        {public = true}
    )
    add_files("Runtime/Source/**.cpp")
    add_links("SDL3", {public = true})

-- ---------------------------------------------------------------------------
-- Executable
-- ---------------------------------------------------------------------------

target("BixEngine")
    set_kind("binary")
    add_deps("bixengine_runtime")
    add_files("Samples/main.cpp")

    -- Copie la bonne SDL3.dll dans le bon répertoire après compilation
    after_build(function (target)
        if not sdl3_root then
            return
        end

        local dll_candidates = {
            path.join(sdl3_root, "bin", "x64", "SDL3.dll"),
            path.join(sdl3_root, "bin", "Win64", "SDL3.dll"),
            path.join(sdl3_root, "lib", "x64", "SDL3.dll"),
            path.join(sdl3_root, "lib", "Win64", "SDL3.dll"),
        }

        for _, dll_path in ipairs(dll_candidates) do
            if os.isfile(dll_path) then
                -- 1️⃣ Copie vers le dossier de l’exécutable (xmake run)
                os.cp(dll_path, path.join(target:targetdir(), "SDL3.dll"))
                -- 2️⃣ Copie aussi vers la racine du projet (pour le bouton Run)
                os.cp(dll_path, path.join(os.scriptdir(), "SDL3.dll"))
                print("✅ Copied SDL3.dll from: " .. dll_path)
                break
            end
        end
    end)
