-- =========================================
-- Project
-- =========================================
set_project("BixEngine")
set_version("0.1.0")
set_languages("c++20")
set_warnings("allextra")
set_optimize("faster")
add_rules("mode.debug", "mode.release")

-- =========================================
-- SDL3 setup (reprend ton bloc complet)
-- =========================================
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

local function resolve_library_dir(candidates)
    local markers = {"SDL3.lib", "SDL3.dll", "libSDL3.a", "libSDL3.so"}
    for _, candidate in ipairs(candidates) do
        if candidate and os.isdir(candidate) then
            for _, marker in ipairs(markers) do
                if os.isfile(path.join(candidate, marker)) then
                    return candidate
                end
            end
        end
    end
end

local function collect_sdl3_paths()
    local project_root = os.scriptdir()
    local vendor_root = path.join(project_root, "ThirdParty", "SDL3-3.2.22")

    local sdl3_root = get_config("sdl3_dir") or os.getenv("SDL3_DIR") or vendor_root
    local include_dir = sdl3_root and path.join(sdl3_root, "include") or nil

    local candidates = {
        get_config("sdl3_lib_dir"),
        os.getenv("SDL3_LIB_DIR"),
        path.join(sdl3_root, "lib"),
        path.join(sdl3_root, "lib", "x64"),
        path.join(sdl3_root, "lib", "Win64"),
        path.join(sdl3_root, "lib", "Release"),
    }

    local lib_dir = resolve_library_dir(candidates)
    return include_dir, lib_dir, sdl3_root
end

local sdl3_include_dir, sdl3_lib_dir, sdl3_root = collect_sdl3_paths()

if sdl3_include_dir then
    add_includedirs(sdl3_include_dir, {public = true})
else
    print("⚠️ SDL3 include directory not found. Set SDL3_DIR or --sdl3_dir.")
end

if sdl3_lib_dir then
    add_linkdirs(sdl3_lib_dir)
else
    print("⚠️ SDL3 library directory not found. Set SDL3_LIB_DIR or --sdl3_lib_dir.")
end

add_defines("SDL_MAIN_HANDLED", {public = true})

if is_plat("windows") then
    add_syslinks("user32", "gdi32", "shell32", "ole32", "oleaut32", "version", "winmm", "imm32", "advapi32")
else
    add_syslinks("pthread", "dl")
end

-- =========================================
-- HeaderTool
-- =========================================
target("BixHeaderTool")
    set_kind("binary")
    set_languages("c++20")
    add_files("Tools/BixHeaderTool/**.cpp")
    -- set_policy("build.auto_depend", false) -- optionnel

-- =========================================
-- ImGui
-- =========================================
target("bixengine_imgui")
    set_kind("static")
    add_includedirs("ThirdParty/ImGui", "ThirdParty/ImGui/backends", {public = true})
    add_files(
        "ThirdParty/ImGui/imgui.cpp",
        "ThirdParty/ImGui/imgui_draw.cpp",
        "ThirdParty/ImGui/imgui_widgets.cpp",
        "ThirdParty/ImGui/imgui_tables.cpp",
        "ThirdParty/ImGui/imgui_demo.cpp",
        "ThirdParty/ImGui/backends/imgui_impl_sdl3.cpp",
        "ThirdParty/ImGui/backends/imgui_impl_sdlrenderer3.cpp"
    )

-- =========================================
-- Engine Runtime
-- =========================================
target("bixengine_runtime")
    set_kind("static")
    add_deps("bixengine_imgui", "BixHeaderTool")

    -- Dossier stable pour les headers générés
    local generated_dir = path.join(os.scriptdir(), "Intermediate", "GeneratedHeaders")

    add_includedirs(".", "Runtime/Include", generated_dir, {public = true})
    add_headerfiles("Runtime/Include/**.h", "Runtime/Include/**.inl")
    add_files("Runtime/Source/**.cpp")
    add_links("SDL3", {public = true})

    -- ✅ Compat XMake 3.0.4 : on ajoute les dépendances fichiers après le chargement
    after_load(function (target)
        for _, header in ipairs(os.files("Runtime/Include/**.h")) do
            target:add("dependfiles", header)
        end
        -- (optionnel) si tu as des headers dans Source :
        for _, header in ipairs(os.files("Runtime/Source/**.h")) do
            target:add("dependfiles", header)
        end
    end)

    before_build(function (target)
        import("core.project.project")

        local tool = project.target("BixHeaderTool")
        assert(tool, "Missing target: BixHeaderTool")

        local tool_path = tool:targetfile()
        if not os.isfile(tool_path) then
            print("⚙️  Building BixHeaderTool...")
            os.execv("xmake", {"build", "BixHeaderTool"})
            tool_path = tool:targetfile()
        end

        if not os.isdir(generated_dir) then
            os.mkdir(generated_dir)
        end

        -- Vérification timestamps
        local stamp_path = path.join(generated_dir, ".timestamp")
        local newest_time = 0
        for _, header in ipairs(os.files("Runtime/Include/**.h")) do
            local t = os.mtime(header)
            if t > newest_time then newest_time = t end
        end

        local need_regen = true
        if os.isfile(stamp_path) then
            local f = io.open(stamp_path, "r")
            local last = tonumber(f:read("*l") or "0")
            f:close()
            need_regen = newest_time > last
        end

        if need_regen then
            print("🔧 Regenerating reflection headers...")
            -- ❗ os.execv lève l’erreur si ça échoue (ne pas capturer ok/err)
            os.execv(tool_path, { "Runtime/Include", "Samples", generated_dir })

            local f = io.open(stamp_path, "w+")
            if f then f:write(tostring(newest_time)) f:close() end
        else
            print("✔ Headers up-to-date.")
        end
    end)

-- =========================================
-- Executable
-- =========================================
target("BixEngine")
    set_kind("binary")
    add_deps("bixengine_runtime")
    add_files("Samples/main.cpp")

    after_build(function (target)
        if not sdl3_root then return end
        local dll_candidates = {
            path.join(sdl3_root, "bin", "x64", "SDL3.dll"),
            path.join(sdl3_root, "bin", "Win64", "SDL3.dll"),
            path.join(sdl3_root, "lib", "x64", "SDL3.dll"),
            path.join(sdl3_root, "lib", "Win64", "SDL3.dll"),
        }
        for _, dll in ipairs(dll_candidates) do
            if os.isfile(dll) then
                os.cp(dll, path.join(target:targetdir(), "SDL3.dll"))
                os.cp(dll, path.join(os.scriptdir(), "SDL3.dll"))
                print("✅ Copied SDL3.dll from: " .. dll)
                break
            end
        end
    end)

-- =========================================
-- 🔧 Custom command: Regenerate reflection headers
-- =========================================
task("regen")
    set_category("plugin")
    set_menu {
        usage = "xmake regen [options]",
        description = "Force regeneration of reflection headers using BixHeaderTool.",
        options = {
            {'f', "force", "k", nil, "Force regeneration (clears previous generated files)."}
        }
    }

    on_run(function ()
        import("core.project.project")

        local force = option.get("force")
        local generated_dir = path.join(os.scriptdir(), "Intermediate", "GeneratedHeaders")

        if force then
            print("🧹 Cleaning previous generated headers...")
            os.tryrm(generated_dir)
        end

        -- Build HeaderTool si nécessaire
        os.execv("xmake", {"build", "BixHeaderTool"})
        local tool = project.target("BixHeaderTool")
        assert(tool, "Missing target: BixHeaderTool")

        local tool_path = tool:targetfile()
        os.mkdir(generated_dir)

        print("🔧 Regenerating reflection headers...")
        os.execv(tool_path, { "Runtime/Include", "Samples", generated_dir })

        -- Met à jour le timestamp
        local newest_time = 0
        for _, header in ipairs(os.files("Runtime/Include/**.h")) do
            local t = os.mtime(header)
            if t > newest_time then newest_time = t end
        end
        local stamp_path = path.join(generated_dir, ".timestamp")
        local f = io.open(stamp_path, "w+")
        if f then f:write(tostring(newest_time)) f:close() end

        print("✅ Done.")
    end)
