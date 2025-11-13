-- ╔══════════════════════════════════════════════════════════════╗
-- ║ 🧱 BixEngine - build with reflection header generation       ║
-- ╚══════════════════════════════════════════════════════════════╝

set_xmakever("3.0.4")
add_rules("mode.debug", "mode.release")
set_languages("cxx20")
set_warnings("allextra")
set_optimize("faster")
add_moduledirs(path.join(os.projectdir(), "Tools/xmake"))

-- ────────────────────────────────────────────────────────────────
-- SDL3
-- ────────────────────────────────────────────────────────────────
local root = os.projectdir()
local sdl3_inc = path.join(root, "ThirdParty/SDL3-3.2.22/include")
local sdl3_lib = path.join(root, "ThirdParty/SDL3-3.2.22/lib/x64")
local sdl3_image_inc = path.join(root, "ThirdParty/SDL3_image/include/SDL3_image")
local sdl3_image_lib = path.join(root, "ThirdParty/SDL3_image/lib/x64")

-- ────────────────────────────────────────────────────────────────
-- Helpers
-- ────────────────────────────────────────────────────────────────
local function current_triplet()
    local plat = get_config("plat") or os.host()
    local arch = get_config("arch") or os.arch()
    local mode = get_config("mode") or "debug"
    return plat, arch, mode
end

local function get_generated_dir()
    local plat, arch, mode = current_triplet()
    return path.join("Build", plat, arch, mode, "Intermediate", "GeneratedHeaders")
end

-- ────────────────────────────────────────────────────────────────
-- Règle includes globaux
-- ────────────────────────────────────────────────────────────────
rule("bix.global_includes")
    on_load(function(target)
        target:add("includedirs", "src/Reflection/Public", {public = true})
        target:add("includedirs", get_generated_dir(), {public = true})
    end)

-- ────────────────────────────────────────────────────────────────
-- Includes communs
-- ────────────────────────────────────────────────────────────────
local engine_public_includes = {
    "src",
    "ThirdParty",
    "ThirdParty/ImGui",
    "ThirdParty/ImGui/backends",
    "ThirdParty/stb",
    "src/Reflection/Public",
    get_generated_dir(),
    sdl3_inc,
    sdl3_image_inc
}

for _, dir in ipairs(os.dirs("src/*")) do
    local public_dir = path.join(dir, "Public")
    if os.isdir(public_dir) then
        table.insert(engine_public_includes, public_dir)
    end
    local lowercase_public = path.join(dir, "public")
    if os.isdir(lowercase_public) then
        table.insert(engine_public_includes, lowercase_public)
    end
end

do
    local dedup = {}
    local filtered = {}
    for _, dir in ipairs(engine_public_includes) do
        if dir and dir ~= "" and not dedup[dir] then
            dedup[dir] = true
            table.insert(filtered, dir)
        end
    end
    engine_public_includes = filtered
end

-- ────────────────────────────────────────────────────────────────
-- BixHeaderTool
-- ────────────────────────────────────────────────────────────────
target("BixHeaderTool")
    set_kind("binary")
    set_default(false)
    set_group("Tools/HeaderTool")
    add_files("Tools/BixHeaderTool/**.cpp")
    add_includedirs("Tools/BixHeaderTool", {public = true})
    before_build(function(t)
        local _, _, mode = current_triplet()
        t:set("targetdir", path.join("Build", os.host(), os.arch(), mode))
    end)

-- ────────────────────────────────────────────────────────────────
-- Phony target to drive header generation before compilation
-- ────────────────────────────────────────────────────────────────
target("GenerateHeaders")
    set_kind("phony")
    set_default(false)
    add_deps("BixHeaderTool")
    on_build(function()
        local reflection = import("Tools.xmake.reflection")
        reflection.generate_headers(false, get_generated_dir())
    end)

-- ────────────────────────────────────────────────────────────────
-- Modules automatiques
-- ────────────────────────────────────────────────────────────────
local function create_engine_module(folder)
    local name = "Bix" .. path.basename(folder)
    local private_dir = path.join(folder, "Private")
    local lowercase_private = path.join(folder, "private")
    local public_dir  = path.join(folder, "Public")
    local lowercase_public = path.join(folder, "public")

    target(name)
        set_group("Engine/" .. path.basename(folder))
        set_kind("static")
        add_rules("bix.global_includes")
        add_deps("GenerateHeaders")

        if os.isdir(private_dir) then add_files(private_dir .. "/**.cpp") end
        if os.isdir(lowercase_private) then add_files(lowercase_private .. "/**.cpp") end
        if os.isdir(public_dir) then add_headerfiles(public_dir .. "/**.h", {public = true}) end
        if os.isdir(lowercase_public) then add_headerfiles(lowercase_public .. "/**.h", {public = true}) end

        local include_dirs = {}
        if os.isdir(public_dir) then table.insert(include_dirs, public_dir) end
        if os.isdir(lowercase_public) then table.insert(include_dirs, lowercase_public) end
        if os.isdir(private_dir) then table.insert(include_dirs, private_dir) end
        if os.isdir(lowercase_private) then table.insert(include_dirs, lowercase_private) end
        for _, inc in ipairs(engine_public_includes) do
            table.insert(include_dirs, inc)
        end
        if #include_dirs > 0 then
            add_includedirs(table.unpack(include_dirs))
        end
end

for _, dir in ipairs(os.dirs("src/*")) do
    if path.basename(dir):lower() ~= "main" then
        create_engine_module(dir)
    end
end

-- ────────────────────────────────────────────────────────────────
-- Exécutable principal
-- ────────────────────────────────────────────────────────────────
target("BixMain")
    set_kind("binary")
    set_default(true)
    set_group("Runtime")
    add_rules("bix.global_includes")
    add_deps("GenerateHeaders")

    for _, dir in ipairs(os.dirs("src/*")) do
        local mod = path.basename(dir)
        if mod:lower() ~= "main" then add_deps("Bix" .. mod) end
    end

    add_files("src/Main/Private/**.cpp")
    add_files("src/Main/private/**.cpp")
    add_headerfiles("src/Main/Public/**.h")
    add_headerfiles("src/Main/public/**.h")
    local main_includes = {}
    for _, dir in ipairs({
        "src/Main/Public",
        "src/Main/public",
        "src/Main/Private",
        "src/Main/private"
    }) do
        if os.isdir(dir) then
            table.insert(main_includes, dir)
        end
    end
    for _, inc in ipairs(engine_public_includes) do
        table.insert(main_includes, inc)
    end
    if #main_includes > 0 then
        add_includedirs(table.unpack(main_includes))
    end
    add_linkdirs(sdl3_lib, sdl3_image_lib)
    add_links("SDL3", "SDL3_image")

    after_build(function(target)
        local exe_dir = path.directory(target:targetfile())
        for _, dll in ipairs({
            path.join(sdl3_lib, "SDL3.dll"),
            path.join(sdl3_image_lib, "SDL3_image.dll")
        }) do
            if os.isfile(dll) then os.cp(dll, exe_dir) end
        end
    end)

-- ────────────────────────────────────────────────────────────────
-- Tâches utilitaires
-- ────────────────────────────────────────────────────────────────
task("regen")
    set_category("action")
    on_run(function()
        local reflection = import("Tools.xmake.reflection")
        reflection.generate_headers(true, get_generated_dir())
    end)

task("cleanbuild")
    set_category("action")
    on_run(function()
        print("[BixEngine] 🧹 Clean + rebuild")
        os.tryrm("Build")
        os.tryrm(".xmake")
        os.exec("xmake f -c")
        os.exec("xmake build")
    end)
