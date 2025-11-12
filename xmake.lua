-- ╔══════════════════════════════════════════════════════════════╗
-- ║ 🧱 BixEngine - Unreal-style project layout for Rider/VS      ║
-- ╚══════════════════════════════════════════════════════════════╝

set_xmakever("3.0.4")
add_rules("mode.debug", "mode.release")
set_languages("cxx20")
set_warnings("allextra")
set_optimize("faster")
add_moduledirs(path.join(os.projectdir(), "Tools/xmake"))

rule("bix.header")
    set_extensions(".h", ".hpp", ".inl")
    on_build(function(target, batchcmds) end)
    on_clean(function(target, batchcmds) end)

-- ╔══════════════════════════════════════════════════════════════╗
-- ║ 🔧 SDL3 / SDL3_image Configuration                           ║
-- ╚══════════════════════════════════════════════════════════════╝
local function configure_sdl()
    local sdl3_inc = path.join(os.projectdir(), "ThirdParty/SDL3-3.2.22/include")
    local sdl3_lib = path.join(os.projectdir(), "ThirdParty/SDL3-3.2.22/lib/x64")
    local sdl3_image_inc = path.join(os.projectdir(), "ThirdParty/SDL3_image/include/SDL3_image")
    local sdl3_image_lib = path.join(os.projectdir(), "ThirdParty/SDL3_image/lib/x64")
    return sdl3_inc, sdl3_lib, sdl3_image_inc, sdl3_image_lib
end

local sdl3_inc, sdl3_lib, sdl3_image_inc, sdl3_image_lib = configure_sdl()

-- ╔══════════════════════════════════════════════════════════════╗
-- ║ 🌍 Global include rule (Reflection + Generated headers)       ║
-- ╚══════════════════════════════════════════════════════════════╝
rule("bix.global_includes")
    on_load(function(target)
        local plat = get_config("plat") or os.host()
        local arch = get_config("arch") or os.arch()
        local mode = get_config("mode") or "debug"
        local generated_dir = path.join("Build", plat, arch, mode, "Intermediate", "GeneratedHeaders")

        target:add("includedirs", "src/Reflection/Public")
        target:add("includedirs", generated_dir)
    end)

-- ╔══════════════════════════════════════════════════════════════╗
-- ║ 📁 Common Include Paths                                      ║
-- ╚══════════════════════════════════════════════════════════════╝
local plat = get_config("plat") or os.host()
local arch = get_config("arch") or os.arch()
local mode = get_config("mode") or "debug"
local generated_dir = path.join("Build", plat, arch, mode, "Intermediate", "GeneratedHeaders")

local engine_public_includes = {
    "src",
    "ThirdParty",
    "ThirdParty/ImGui",
    "ThirdParty/ImGui/backends",
    "ThirdParty/stb",
    "src/Reflection/Public",
    generated_dir,
    sdl3_inc,
    sdl3_image_inc
}

-- Inclure tous les sous-dossiers Public
for _, dir in ipairs(os.dirs("src/*")) do
    local public_dir = path.join(dir, "Public"):gsub("\\", "/")
    if os.isdir(public_dir) then
        table.insert(engine_public_includes, public_dir)
    end
end

local global_includes = engine_public_includes

-- ╔══════════════════════════════════════════════════════════════╗
-- ║ 📂 Helper: Module (auto Public + Private discovery)           ║
-- ╚══════════════════════════════════════════════════════════════╝
function create_engine_module(name, folder, deps)
    local folder_fixed = folder:gsub("\\", "/")
    local module_display = path.basename(folder_fixed)
    local private_dir = folder_fixed .. "/Private"
    local public_dir  = folder_fixed .. "/Public"

    target(name)
        add_rules("bix.global_includes")
        set_group("Engine/" .. module_display)
        set_kind("static")
        add_deps("GenerateHeaders")

        if deps then
            add_deps(table.unpack(deps))
        end

        if os.isdir(private_dir) then
            add_files(private_dir .. "/**.cpp", { group = module_display .. "/Private" })
            add_headerfiles(private_dir .. "/**.h", { group = module_display .. "/Private", rule = "bix.header" })
        end

        if os.isdir(public_dir) then
            add_headerfiles(public_dir .. "/**.h", { group = module_display .. "/Public", install = false, rule = "bix.header" })
        end

        add_includedirs(public_dir, private_dir, table.unpack(global_includes))
end

-- ╔══════════════════════════════════════════════════════════════╗
-- ║ 🔍 Automatically create one target per module in src/        ║
-- ╚══════════════════════════════════════════════════════════════╝
for _, dir in ipairs(os.dirs("src/*")) do
    local module_name = path.basename(dir)
    if module_name:lower() ~= "main" then
        local target_name = "Bix" .. module_name
        create_engine_module(target_name, dir)
    end
end

-- ╔══════════════════════════════════════════════════════════════╗
-- ║ 🧩 Header Tool                                               ║
-- ╚══════════════════════════════════════════════════════════════╝
target("BixHeaderTool")
    add_rules("bix.global_includes")
    set_group("Tools/HeaderTool")
    set_kind("binary")
    add_files("Tools/BixHeaderTool/**.cpp")
    add_files("Tools/BixHeaderTool/**.h", { rule = "bix.header" })
    add_includedirs("Tools/BixHeaderTool", { public = true })
    before_build(function(t)
        local mode = import("core.project.config").get("mode") or "debug"
        t:set("targetdir", path.join("Build", os.host(), os.arch(), mode))
    end)

-- ╔══════════════════════════════════════════════════════════════╗
-- ║ ⚙️ GenerateHeaders (prebuild reflection step)                ║
-- ╚══════════════════════════════════════════════════════════════╝
target("GenerateHeaders")
    add_rules("bix.global_includes")
    set_group("Tools/Reflection")
    set_kind("phony")
    add_deps("BixHeaderTool")
    before_build(function()
        import("Tools.xmake.reflection").generate_headers(false, generated_dir)
    end)

-- ╔══════════════════════════════════════════════════════════════╗
-- ║ 🎮 Main Executable (src/Main)                                ║
-- ╚══════════════════════════════════════════════════════════════╝
target("BixMain")
    add_rules("bix.global_includes")
    set_group("Runtime")
    set_kind("binary")
    set_default(true)
    add_deps("GenerateHeaders")

    local engine_modules = {}
    for _, dir in ipairs(os.dirs("src/*")) do
        local mod = path.basename(dir)
        if mod:lower() ~= "main" then
            table.insert(engine_modules, "Bix" .. mod)
        end
    end
    add_deps(table.unpack(engine_modules))

    local main_private = "src/Main/Private"
    local main_public  = "src/Main/Public"

    add_files(main_private .. "/**.cpp", { group = "Private" })
    add_headerfiles(main_private .. "/**.h", { group = "Private" })
    add_headerfiles(main_public .. "/**.h", { group = "Public" })

    add_includedirs(main_public, main_private, table.unpack(global_includes))
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

-- ╔══════════════════════════════════════════════════════════════╗
-- ║ 🧰 Utility Tasks                                             ║
-- ╚══════════════════════════════════════════════════════════════╝
task("regen")
    set_category("action")
    on_run(function()
        print("[BixEngine] ♻️ Regénération forcée des headers...")
        import("Tools.xmake.reflection").generate_headers(true, generated_dir)
    end)

task("cleanbuild")
    set_category("action")
    on_run(function()
        print("[BixEngine] 🧹 Clean + rebuild complet")
        os.tryrm("Build"); os.tryrm(".xmake")
        os.exec("xmake f -c && xmake build")
    end)
