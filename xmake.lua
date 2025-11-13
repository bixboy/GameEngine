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
-- Règle includes globaux
-- ────────────────────────────────────────────────────────────────
rule("bix.global_includes")
    on_load(function(target)
        local config = import("core.project.config")
        local plat = config.get("plat") or os.host()
        local arch = config.get("arch") or os.arch()
        local mode = config.get("mode") or "debug"
        local gen_dir = path.join("Build", plat, arch, mode, "Intermediate", "GeneratedHeaders")
        target:add("includedirs", "src/Reflection/Public", {public = true})
        target:add("includedirs", gen_dir, {public = true})
    end)

-- ────────────────────────────────────────────────────────────────
-- Includes communs
-- ────────────────────────────────────────────────────────────────
local plat = get_config("plat") or os.host()
local arch = get_config("arch") or os.arch()
local mode = get_config("mode") or "debug"
local gen_dir = path.join("Build", plat, arch, mode, "Intermediate", "GeneratedHeaders")

local engine_public_includes = {
    "src",
    "ThirdParty",
    "ThirdParty/ImGui",
    "ThirdParty/ImGui/backends",
    "ThirdParty/stb",
    "src/Reflection/Public",
    gen_dir,
    sdl3_inc,
    sdl3_image_inc
}
for _, dir in ipairs(os.dirs("src/*")) do
    local public_dir = path.join(dir, "Public")
    if os.isdir(public_dir) then table.insert(engine_public_includes, public_dir) end
end

-- ────────────────────────────────────────────────────────────────
-- BixHeaderTool
-- ────────────────────────────────────────────────────────────────
target("BixHeaderTool")
    set_kind("binary")
    set_group("Tools/HeaderTool")
    add_files("Tools/BixHeaderTool/**.cpp")
    add_includedirs("Tools/BixHeaderTool", {public = true})
    before_build(function(t)
        local config = import("core.project.config")
        local mode = config.get("mode") or "debug"
        t:set("targetdir", path.join("Build", os.host(), os.arch(), mode))
    end)

-- ────────────────────────────────────────────────────────────────
-- Modules automatiques
-- ────────────────────────────────────────────────────────────────
local function create_engine_module(folder)
    local name = "Bix" .. path.basename(folder)
    local private_dir = path.join(folder, "Private")
    local public_dir  = path.join(folder, "Public")

    target(name)
        set_group("Engine/" .. path.basename(folder))
        set_kind("static")
        add_rules("bix.global_includes")
        add_deps("BixHeaderTool")

        if os.isdir(private_dir) then add_files(private_dir .. "/**.cpp") end
        if os.isdir(public_dir) then add_headerfiles(public_dir .. "/**.h", {public = true}) end
        add_includedirs(public_dir, private_dir, table.unpack(engine_public_includes))
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
    add_deps("BixHeaderTool")

    for _, dir in ipairs(os.dirs("src/*")) do
        local mod = path.basename(dir)
        if mod:lower() ~= "main" then add_deps("Bix" .. mod) end
    end

    add_files("src/Main/Private/**.cpp")
    add_headerfiles("src/Main/Public/**.h")
    add_includedirs("src/Main/Public", "src/Main/Private", table.unpack(engine_public_includes))
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
        _ENV.__bix_headers_generated = nil
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
