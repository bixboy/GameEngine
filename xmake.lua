-- ╔══════════════════════════════════════════════════════════════╗
-- ║ 🧱  BixEngine — XMake Build Script (Hard Clean Version)       ║
-- ╚══════════════════════════════════════════════════════════════╝

-- =====================================================================
-- 🏗️ Project Setup
-- =====================================================================
set_xmakever("3.0.4")
add_rules("mode.debug", "mode.release")
set_languages("cxx20")
set_warnings("allextra")
set_optimize("faster")

add_moduledirs(path.join(os.projectdir(), "Tools/xmake"))

-- =====================================================================
-- 📦 Helper functions
-- =====================================================================

local function push(list, value)
    if value and value ~= "" then table.insert(list, value) end
end

local function get_path(...)
    return path.join(...)
end

local function log(tag, msg)
    print(string.format("[%s] %s", tag, msg))
end

local function get_build_root()
    local plat = get_config("plat") or os.host()
    local arch = get_config("arch") or os.arch()
    local mode = get_config("mode") or "debug"
    return plat, arch, mode, path.join("Build", plat, arch, mode)
end

local plat, arch, mode, build_root = get_build_root()
local generated_dir = get_path(build_root, "Intermediate", "GeneratedHeaders")

-- =====================================================================
-- 🧩 SDL3 + SDL3_image Configuration
-- =====================================================================

local function configure_sdk(opt_inc, opt_lib, fallback_inc, fallback_lib)
    local inc = get_config(opt_inc) or ""
    local lib = get_config(opt_lib) or ""

    if inc == "" and lib == "" then
        inc = fallback_inc
        lib = fallback_lib
    elseif inc == "" or lib == "" then
        log("SDK", "❌ Les chemins include + lib doivent être définis ensemble.")
    end

    return inc, lib
end

local sdl3_inc, sdl3_lib = configure_sdk(
    "sdl3_dir", "sdl3_lib_dir",
    get_path(os.projectdir(), "ThirdParty/SDL3-3.2.22/include"),
    get_path(os.projectdir(), "ThirdParty/SDL3-3.2.22/lib/x64")
)

local sdlimg_inc, sdlimg_lib = configure_sdk(
    "sdl3_image_dir", "sdl3_image_lib_dir",
    get_path(os.projectdir(), "ThirdParty/SDL3_image/include/SDL3_image"),
    get_path(os.projectdir(), "ThirdParty/SDL3_image/lib/x64")
)

-- =====================================================================
-- 📚 Global Include Directories
-- =====================================================================

local engine_public_includes = {
    "Runtime/Include",
    "ThirdParty",
    "ThirdParty/ImGui",
    "ThirdParty/ImGui/backends",
    "ThirdParty/stb",
    generated_dir
}

push(engine_public_includes, sdl3_inc)
push(engine_public_includes, sdlimg_inc)

-- =====================================================================
-- 🧩 Module Discovery System (public/private layout)
-- =====================================================================

local function discover_modules(root)
    local modules = {}
    for _, dir in ipairs(os.dirs(path.join(root, "*"))) do
        local private = path.join(dir, "private")
        local public  = path.join(dir, "public")

        if os.isdir(private) or os.isdir(public) then
            table.insert(modules, {
                name = path.basename(dir),
                root = dir,
                private = private,
                public  = public
            })
        end
    end
    table.sort(modules, function(a, b) return a.name < b.name end)
    return modules
end

local all_modules = discover_modules("src")
local engine_modules, main_module = {}, nil

local function collect_all_public_dirs(mods)
    local dirs = {}
    for _, m in ipairs(mods) do
        if os.isdir(m.public) then table.insert(dirs, m.public) end
    end
    return dirs
end

local all_public_dirs = collect_all_public_dirs(all_modules)

-- Identify "Main" module
for _, m in ipairs(all_modules) do
    if m.name:lower() == "main" then
        main_module = m
    else
        table.insert(engine_modules, m)
    end
end

-- =====================================================================
-- 🧩 Module Target Definition
-- =====================================================================

local function define_module_target(m, group)
    target(m.name)
        set_kind("object")
        set_default(false)
        set_group(group or "Modules")

        add_deps("BixHeaderTool", "GenerateHeaders")

        -- GLOBAL includes
        add_includedirs(engine_public_includes, { public = true })
        add_includedirs("src", { public = true })
        if #all_public_dirs > 0 then
            add_includedirs(all_public_dirs, { public = true })
        end

        -- PUBLIC directory
        if os.isdir(m.public) then
            add_includedirs(m.public, { public = true })

            add_headerfiles(path.join(m.public, "**.h"), {
                public = true,
                group = "Public",
                prefixdir = path.join(m.name, "Public")
            })

            add_files(path.join(m.public, "**.cpp"), {
                group = "Private",
                prefixdir = path.join(m.name, "Public")
            })
        end

        -- PRIVATE directory
        if os.isdir(m.private) then
            add_includedirs(m.private)

            add_headerfiles(path.join(m.private, "**.h"), {
                group = "Private",
                prefixdir = path.join(m.name, "Private")
            })

            add_files(path.join(m.private, "**.cpp"), {
                group = "Private",
                prefixdir = path.join(m.name, "Private")
            })
        end

    target_end()
    return m.name
end

-- Register all module targets
local engine_targets = {}
for _, m in ipairs(engine_modules) do
    table.insert(engine_targets, define_module_target(m))
end

local main_target = main_module and define_module_target(main_module, "Executable") or nil

-- =====================================================================
-- 🔨 BixHeaderTool
-- =====================================================================

target("BixHeaderTool")
    set_kind("binary")
    set_default(false)
    set_plat(os.host())
    set_arch(os.arch())
    set_policy("build.fence", true)

    before_build(function(t)
        t:set("targetdir", get_path(build_root))
        log("HeaderTool", "Mode = " .. mode)
    end)

    add_files("Tools/BixHeaderTool/**.cpp")
    add_includedirs("Tools/BixHeaderTool")
target_end()

-- =====================================================================
-- 🧩 GenerateHeaders (Reflection Pre-Build)
-- =====================================================================

target("GenerateHeaders")
    set_kind("phony")
    add_deps("BixHeaderTool")

    before_build(function()
        import("Tools.xmake.reflection")

        if not os.isdir(generated_dir) then os.mkdir(generated_dir) end

        local header_patterns = {
            "src/**.h",
            get_path(build_root, "Content", "**.h")
        }

        -- detect changes
        local newest_header = 0
        for _, pat in ipairs(header_patterns) do
            for _, h in ipairs(os.files(pat)) do
                newest_header = math.max(newest_header, os.mtime(h))
            end
        end

        local newest_gen = 0
        for _, gh in ipairs(os.files(path.join(generated_dir, "*.generated.h"))) do
            newest_gen = math.max(newest_gen, os.mtime(gh))
        end

        if newest_gen == 0 or newest_header > newest_gen then
            log("GenerateHeaders", "🔄 Regeneration required")
            reflection.generate_headers(false, generated_dir)
        else
            log("GenerateHeaders", "⏩ Up to date")
        end
    end)
target_end()

-- =====================================================================
-- ⚙️ BixEngine (Static Library)
-- =====================================================================

target("BixEngine")
    set_kind("static")
    set_default(false)

    add_deps("BixHeaderTool", "GenerateHeaders")
    if #engine_targets > 0 then
        add_deps(table.unpack(engine_targets))
    end

    add_includedirs(engine_public_includes, { public = true })
    add_includedirs("src", { public = true })

    -- ImGui
    add_files("ThirdParty/ImGui/*.cpp")
    add_files("ThirdParty/ImGui/backends/imgui_impl_sdl3.cpp")
    add_files("ThirdParty/ImGui/backends/imgui_impl_sdlrenderer3.cpp")

    add_linkdirs(sdl3_lib, sdlimg_lib)
    add_links("SDL3", "SDL3_image")
target_end()

-- =====================================================================
-- 🚀 BixRun executable
-- =====================================================================

target("BixRun")
    set_kind("binary")
    set_default(true)

    add_deps("BixEngine")
    if main_target then add_deps(main_target) end

    add_includedirs(engine_public_includes)
    add_linkdirs(sdl3_lib, sdlimg_lib)
    add_links("SDL3", "SDL3_image")

    local content_dir = get_path(build_root, "Content")
    if os.isdir(content_dir) then
        add_files(get_path(content_dir, "**.cpp"))
        add_includedirs(content_dir)
        log("Run", "Scripts loaded from Content/")
    end

    after_build(function(t)
        local exe_dir = path.directory(t:targetfile())
        for _, dll in ipairs({
            get_path(sdl3_lib, "SDL3.dll"),
            get_path(sdlimg_lib, "SDL3_image.dll")
        }) do
            if os.isfile(dll) then os.cp(dll, exe_dir) end
        end
    end)
target_end()

-- =====================================================================
-- 🔧 Tasks: regen / fullbuild / cleanbuild
-- =====================================================================

task("regen")
    on_run(function()
        import("Tools.xmake.reflection")
        reflection.generate_headers(true, generated_dir)
    end)
task_end()

task("fullbuild")
    on_run(function()
        task.run("regen")
        os.exec("xmake build")
    end)
task_end()

task("cleanbuild")
    on_run(function()
        os.exec("xmake clean -a")
        os.tryrm(".xmake")
        os.tryrm("Build")

        task.run("regen")
        os.exec("xmake build BixRun")
        os.exec("xmake run BixRun")
    end)
task_end()
