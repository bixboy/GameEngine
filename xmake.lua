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

local function canonical_path(dir)
    if not dir or dir == "" then
        return nil
    end
    local abs = path.absolute(dir)
    if not abs then
        return dir
    end
    return abs:gsub("\\", "/"):lower()
end

local function dedupe_paths(paths)
    local seen = {}
    local result = {}
    for _, dir in ipairs(paths) do
        if dir and dir ~= "" then
            local key = canonical_path(dir) or dir
            if not seen[key] then
                seen[key] = true
                table.insert(result, dir)
            end
        end
    end
    return result
end

local function existing_dirs(...)
    local result = {}
    local seen = {}
    for _, dir in ipairs({...}) do
        if dir and os.isdir(dir) then
            local key = canonical_path(dir) or dir
            if not seen[key] then
                seen[key] = true
                table.insert(result, dir)
            end
        end
    end
    return result
end

local reflection_generator
local function load_reflection_generator()
    if not reflection_generator then
        local reflection = import("Tools.xmake.reflection")
        assert(type(reflection) == "table", "Tools.xmake.reflection doit retourner une table")
        assert(type(reflection.generate_headers) == "function", "Tools.xmake.reflection doit exposer la fonction generate_headers")
        reflection_generator = reflection.generate_headers
    end
    return reflection_generator
end

local function run_header_generation(force)
    local generator = load_reflection_generator()
    local ok, errmsg = generator(force or false, get_generated_dir())
    if not ok then
        if errmsg and errmsg ~= "" then
            raise("échec de la génération des headers : %s", errmsg)
        else
            raise("échec de la génération des headers (raison inconnue)")
        end
    end
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
    local lowercase_public = path.join(dir, "public")
    for _, candidate in ipairs(existing_dirs(public_dir, lowercase_public)) do
        table.insert(engine_public_includes, candidate)
    end
end

engine_public_includes = dedupe_paths(engine_public_includes)

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
    set_policy("build.fence", true)
    on_build(function()
        run_header_generation(false)
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

        for _, dir in ipairs(existing_dirs(private_dir, lowercase_private)) do
            add_files(path.join(dir, "**.cpp"))
        end
        for _, dir in ipairs(existing_dirs(public_dir, lowercase_public)) do
            add_headerfiles(path.join(dir, "**.h"), {public = true})
        end

        local include_dirs = existing_dirs(public_dir, lowercase_public, private_dir, lowercase_private)
        for _, inc in ipairs(engine_public_includes) do
            table.insert(include_dirs, inc)
        end
        include_dirs = dedupe_paths(include_dirs)
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

    for _, dir in ipairs(existing_dirs("src/Main/Private", "src/Main/private")) do
        add_files(path.join(dir, "**.cpp"))
    end
    for _, dir in ipairs(existing_dirs("src/Main/Public", "src/Main/public")) do
        add_headerfiles(path.join(dir, "**.h"))
    end
    local main_includes = existing_dirs(
        "src/Main/Public",
        "src/Main/public",
        "src/Main/Private",
        "src/Main/private"
    )
    for _, inc in ipairs(engine_public_includes) do
        table.insert(main_includes, inc)
    end
    main_includes = dedupe_paths(main_includes)
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
        run_header_generation(true)
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
