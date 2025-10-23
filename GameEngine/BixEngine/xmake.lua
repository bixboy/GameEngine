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

-- =========================================
-- Reflection helpers (HeaderTool integration)
-- =========================================
local config = import("core.project.config")
local project_root = os.scriptdir()
local include_root = path.join(project_root, "Runtime", "Include")
local samples_root = path.join(project_root, "Samples")

local function compute_generated_dir(target)
    if config and config.load then
        config.load()
    end
    local plat = target and target:plat() or config.get("plat") or os.host()
    local arch = target and target:arch() or config.get("arch") or os.arch()
    local mode = target and target:mode() or config.get("mode") or "debug"
    return path.join(project_root, "Build", plat, arch, mode, "Intermediate", "GeneratedHeaders")
end

local function parse_generated_includes(header_path)
    local results = {}
    local file = io.open(header_path, "r")
    if not file then
        return results
    end
    for line in file:lines() do
        local token = line:match('#include%s+"([%w%._%-%/\]+%.generated%.h)"')
        if token then
            token = token:gsub("\\", "/")
            table.insert(results, token)
        end
    end
    file:close()
    return results
end

local function resolve_generated_relpath(header_path, include_token)
    if include_token:find("/") then
        return include_token
    end
    local relative_header = path.relative(header_path, include_root)
    local directory = path.directory(relative_header)
    if directory and directory ~= "." then
        return path.join(directory, include_token)
    end
    return include_token
end

local function ensure_stub_headers(generated_dir)
    local created = {}
    for _, header in ipairs(os.files(path.join(include_root, "**.h"))) do
        for _, token in ipairs(parse_generated_includes(header)) do
            local relpath = resolve_generated_relpath(header, token)
            local out_path = path.join(generated_dir, relpath)
            local out_dir = path.directory(out_path)
            if out_dir and out_dir ~= "." and not os.isdir(out_dir) then
                os.mkdir(out_dir)
            end
            if not os.isfile(out_path) then
                io.writefile(out_path, "")
            end
            table.insert(created, out_path)
        end
    end
    return created
end

local function newest_header_timestamp()
    local newest = 0
    for _, header in ipairs(os.files(path.join(include_root, "**.h"))) do
        local t = os.mtime(header)
        if t and t > newest then
            newest = t
        end
    end
    return newest
end

local function ensure_header_tool()
    import("core.project.project")
    local tool = project.target("BixHeaderTool")
    assert(tool, "Missing target: BixHeaderTool")
    os.execv("xmake", {"build", "BixHeaderTool"})
    return tool:targetfile()
end

local function write_timestamp(stamp_path, value)
    local file = io.open(stamp_path, "w+")
    if file then
        file:write(tostring(value))
        file:close()
    end
end

local function read_timestamp(stamp_path)
    if not os.isfile(stamp_path) then
        return 0
    end
    local file = io.open(stamp_path, "r")
    if not file then
        return 0
    end
    local line = file:read("*l")
    file:close()
    return tonumber(line or "0") or 0
end

local function cleanup_empty_generated_files(generated_dir)
    for _, file in ipairs(os.files(path.join(generated_dir, "**.generated.h"))) do
        if os.filesize(file) == 0 then
            os.rm(file)
        end
    end
end

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

    add_includedirs(".", "Runtime/Include",
        path.join("$(projectdir)", "Build", "$(plat)", "$(arch)", "$(mode)", "Intermediate", "GeneratedHeaders"),
        {public = true})
    add_headerfiles("Runtime/Include/**.h", "Runtime/Include/**.inl")
    add_files("Runtime/Source/**.cpp")
    add_links("SDL3", {public = true})

    -- =========================================
    -- Build hooks
    -- =========================================
    after_load(function (target)
        for _, header in ipairs(os.files("Runtime/Include/**.h")) do
            target:add("dependfiles", header)
        end
        for _, header in ipairs(os.files("Runtime/Source/**.h")) do
            target:add("dependfiles", header)
        end

        local generated_dir = compute_generated_dir(target)
        target:data_set("bix.generated_dir", generated_dir)
    end)

    before_build(function (target)
        local generated_dir = target:data("bix.generated_dir") or compute_generated_dir(target)
        if not os.isdir(generated_dir) then
            os.mkdir(generated_dir)
        end

        -- Step 1: build the header tool if required
        local tool_path = ensure_header_tool()

        -- Step 2: ensure stub files exist prior to the first C++ compilation
        ensure_stub_headers(generated_dir)

        -- Step 3: determine whether a regeneration is required
        local stamp_path = path.join(generated_dir, ".timestamp")
        local newest_time = newest_header_timestamp()
        local recorded_time = read_timestamp(stamp_path)

        local need_regen = (newest_time > recorded_time)
        if not need_regen then
            -- If we only have stubs, force a regeneration
            for _, file in ipairs(os.files(path.join(generated_dir, "**.generated.h"))) do
                if os.filesize(file) == 0 then
                    need_regen = true
                    break
                end
            end
        end

        if need_regen then
            print("🔧 Regenerating reflection headers...")
            os.execv(tool_path, { include_root, samples_root, generated_dir })
            cleanup_empty_generated_files(generated_dir)
            write_timestamp(stamp_path, newest_time)
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
    set_menu({
        usage = "xmake regen [options]",
        description = "Force regeneration of reflection headers using BixHeaderTool.",
        options = {
            {"f", "force", "k", nil, "Force regeneration (cleans previous generated files)."}
        }
    })

    on_run(function ()
        local option = import("core.base.option")

        local force = option.get("force")
        local generated_dir = compute_generated_dir()

        if force then
            print("🧹 Cleaning previous generated headers...")
            os.tryrm(generated_dir)
        end

        if not os.isdir(generated_dir) then
            os.mkdir(generated_dir)
        end

        local tool_path = ensure_header_tool()
        ensure_stub_headers(generated_dir)

        print("🔧 Regenerating reflection headers...")
        os.execv(tool_path, { include_root, samples_root, generated_dir })
        cleanup_empty_generated_files(generated_dir)
        write_timestamp(path.join(generated_dir, ".timestamp"), newest_header_timestamp())

        print("✅ Done.")
    end)

-- =========================================
-- 🔨 Custom command: Full build (regen + build)
-- =========================================
task("fullbuild")
    set_category("plugin")
    set_menu({
        usage = "xmake fullbuild",
        description = "Regenerate reflection headers, then build all targets."
    })

    on_run(function ()
        local generated_dir = compute_generated_dir()

        if not os.isdir(generated_dir) then
            os.mkdir(generated_dir)
        end

        local tool_path = ensure_header_tool()
        ensure_stub_headers(generated_dir)

        print("🔧 Regenerating reflection headers...")
        os.execv(tool_path, { include_root, samples_root, generated_dir })
        cleanup_empty_generated_files(generated_dir)
        write_timestamp(path.join(generated_dir, ".timestamp"), newest_header_timestamp())

        print("🏗️ Building all targets...")
        os.execv("xmake", {"build"})
    end)
