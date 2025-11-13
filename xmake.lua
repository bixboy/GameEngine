-- ╔══════════════════════════════════════════════════════════════╗
-- ║ 🧱  XMake build script for BixEngine                         ║
-- ╚══════════════════════════════════════════════════════════════╝

set_xmakever("3.0.4")
add_rules("mode.debug", "mode.release")

set_languages("cxx20")
set_warnings("allextra")
set_optimize("faster")

-- ────────────────────────────────────────────────────────────────
-- 🔧 Module directory (for custom build tools)
-- ────────────────────────────────────────────────────────────────
add_moduledirs(path.join(os.projectdir(), "Tools/xmake"))

-- ────────────────────────────────────────────────────────────────
-- 🧩 SDL3 Configuration
-- ────────────────────────────────────────────────────────────────
option("sdl3_dir")
    set_showmenu(true)
    set_description("Chemin vers le répertoire include de SDL3")
    set_default(os.getenv("SDL3_DIR") or "")
option_end()

option("sdl3_lib_dir")
    set_showmenu(true)
    set_description("Chemin vers le répertoire lib de SDL3")
    set_default(os.getenv("SDL3_LIB_DIR") or "")
option_end()

local sdl3_inc = get_config("sdl3_dir") or ""
local sdl3_lib = get_config("sdl3_lib_dir") or ""

if (sdl3_inc == "" and sdl3_lib == "") then
    sdl3_inc = path.join(os.projectdir(), "ThirdParty/SDL3-3.2.22/include")
    sdl3_lib = path.join(os.projectdir(), "ThirdParty/SDL3-3.2.22/lib/x64")
elseif (sdl3_inc == "" or sdl3_lib == "") then
    print("❌ Veuillez définir *à la fois* sdl3_dir et sdl3_lib_dir pour utiliser SDL3 externe.")
end


-- ────────────────────────────────────────────────────────────────
-- 🧩 SDL3_image Configuration
-- ────────────────────────────────────────────────────────────────
option("sdl3_image_dir")
    set_showmenu(true)
    set_description("Chemin vers le répertoire include de SDL3_image")
    set_default(os.getenv("SDL3_IMAGE_DIR") or "")
option_end()

option("sdl3_image_lib_dir")
    set_showmenu(true)
    set_description("Chemin vers le répertoire lib de SDL3_image")
    set_default(os.getenv("SDL3_IMAGE_LIB_DIR") or "")
option_end()

local sdl3_image_inc = get_config("sdl3_image_dir") or ""
local sdl3_image_lib = get_config("sdl3_image_lib_dir") or ""

if (sdl3_image_inc == "" and sdl3_image_lib == "") then
    sdl3_image_inc = path.join(os.projectdir(), "ThirdParty/SDL3_image/include/SDL3_image")
    sdl3_image_lib = path.join(os.projectdir(), "ThirdParty/SDL3_image/lib/x64")
    
end

-- ────────────────────────────────────────────────────────────────
-- 🗂️ Generated headers directory
-- ────────────────────────────────────────────────────────────────
local plat = get_config("plat") or os.host()
local arch = get_config("arch") or os.arch()
local mode = get_config("mode") or "debug"

local generated_dir = path.join("Build", plat, arch, mode, "Intermediate", "GeneratedHeaders")

-- ────────────────────────────────────────────────────────────────
-- 📚 Include directories (shared between the engine and executables)
--   * Runtime/Include                  → public engine headers
--   * ThirdParty                       → header-only libraries (nlohmann/json, etc.)
--   * ThirdParty/ImGui (+ backends)    → ImGui sources
--   * ThirdParty/stb                   → stb_image & stb_image_write
--   * Generated headers                → reflection output (per platform/config)
--   * SDL3 / SDL3_image                → SDKs detected above (if available)
--
--  The table is filtered to avoid empty strings so Rider/MSBuild receive
--  a clean list with no duplicate or dangling separators.
-- ────────────────────────────────────────────────────────────────
local engine_public_includes = {
    "Runtime/Include",
    "ThirdParty",
    "ThirdParty/ImGui",
    "ThirdParty/ImGui/backends",
    "ThirdParty/stb",
    generated_dir
}

local function push_if_dir(list, value)
    if value and value ~= "" then
        table.insert(list, value)
    end
end

push_if_dir(engine_public_includes, sdl3_inc)
push_if_dir(engine_public_includes, sdl3_image_inc)

-- ────────────────────────────────────────────────────────────────
-- 🧩 Source modules (public/private layout discovery)
-- ────────────────────────────────────────────────────────────────
local function discover_modules(root)
    local modules = {}
    for _, dir in ipairs(os.dirs(path.join(root, "*"))) do
        local name = path.basename(dir)
        local private_dir = path.join(dir, "private")
        local public_dir = path.join(dir, "public")

        if os.isdir(private_dir) or os.isdir(public_dir) then
            table.insert(modules, {
                name = name,
                root = dir,
                private_dir = private_dir,
                public_dir = public_dir
            })
        end
    end

    table.sort(modules, function(a, b) return a.name < b.name end)
    return modules
end

local all_modules = discover_modules("src")
local engine_modules = {}
local main_module = nil

for _, module in ipairs(all_modules) do
    if module.name:lower() == "main" then
        main_module = module
    else
        table.insert(engine_modules, module)
    end
end

-- ╔══════════════════════════════════════════════════════════════╗
-- ║ 🔨 Target: BixHeaderTool                                     ║
-- ╚══════════════════════════════════════════════════════════════╝
target("BixHeaderTool")
    set_kind("binary")
    set_default(false)
    set_plat(os.host())
    set_arch(os.arch())
    set_policy("build.fence", true)

    before_build(function (target)
        import("core.project.config")
        local mode = config.get("mode") or "debug"
        target:set("targetdir", path.join("Build", os.host(), os.arch(), mode))
        print(string.format("[BixHeaderTool] 🎯 Mode actif détecté : %s", mode))
        print(string.format("[BixHeaderTool] 📦 Sortie : %s", target:targetdir()))
    end)

    add_files("Tools/BixHeaderTool/**.cpp")
    add_includedirs("Tools/BixHeaderTool", { public = true })

-- ╔══════════════════════════════════════════════════════════════╗
-- ║ 🧩 Target: GenerateHeaders (pré-build header generation)      ║
-- ╚══════════════════════════════════════════════════════════════╝
target("GenerateHeaders")
    set_kind("phony")
    add_deps("BixHeaderTool")

    before_build(function(target)
        import("core.project.config")
        import("Tools.xmake.reflection")
    
        local mode = config.get("mode") or "debug"
        print(string.format("[GenerateHeaders] 🚀 Mode de génération détecté : %s", mode))
    
        if not os.isdir(generated_dir) then
            os.mkdir(generated_dir)
        end
    
        -- 🔍 Vérifie si un .h a changé depuis la dernière génération
        local newest_header_time = 0
        local header_paths = {
            "src/**.h",
            path.join("Build", os.host(), os.arch(), mode, "Content", "**.h")
        }
    
        for _, pattern in ipairs(header_paths) do
            for _, header in ipairs(os.files(pattern)) do
                local mtime = os.mtime(header)
                if mtime > newest_header_time then
                    newest_header_time = mtime
                end
            end
        end
    
        -- 🔍 Vérifie les .generated.h existants
        local newest_generated_time = 0
        local has_generated = false
        for _, gen in ipairs(os.files(path.join(generated_dir, "*.generated.h"))) do
            has_generated = true
            local mtime = os.mtime(gen)
            if mtime > newest_generated_time then
                newest_generated_time = mtime
            end
        end
    
        -- 🧩 Recherche uniquement les headers qui contiennent un include .generated.h
        local headers_with_generated = {}
        for _, pattern in ipairs(header_paths) do
            for _, header in ipairs(os.files(pattern)) do
                local content = io.readfile(header)
                if content and content:find('#include%s+"[%w_]+%.generated%.h"') then
                    table.insert(headers_with_generated, header)
                end
            end
        end
    
        -- 🧠 Vérifie uniquement pour ces headers si un .generated.h manque
        local missing_generated = {}
        for _, header in ipairs(headers_with_generated) do
            local stem = path.basename(header):gsub("%.h$", "")
            local gen_path = path.join(generated_dir, stem .. ".generated.h")
            if not os.isfile(gen_path) then
                table.insert(missing_generated, gen_path)
            end
        end
    
        -- 🧠 Décision
        local need_regen = false
        if not has_generated then
            print("[GenerateHeaders] ⚠️ Aucun fichier généré trouvé — première génération requise.")
            need_regen = true
        elseif #missing_generated > 0 then
            print("[GenerateHeaders] ⚠️ Fichiers générés manquants détectés :")
            for _, f in ipairs(missing_generated) do
                print("   - " .. f)
            end
            need_regen = true
        elseif newest_header_time > newest_generated_time then
            print("[GenerateHeaders] 🔄 Des headers contenant du reflection ont été modifiés — régénération nécessaire.")
            need_regen = true
        else
            print("[GenerateHeaders] ⏩ Aucun changement détecté — génération sautée.")
        end
    
        -- 🚀 Génération seulement si besoin
        if need_regen then
            local reflection = import("Tools.xmake.reflection")
            reflection.generate_headers(false, generated_dir)
        end
    end)

    
-- ╔══════════════════════════════════════════════════════════════╗
-- ║ ⚙️ Target: BixEngine (core static library)                   ║
-- ╚══════════════════════════════════════════════════════════════╝
target("BixEngine")
    set_kind("static")
    set_default(false)
    add_deps("BixHeaderTool", "GenerateHeaders")

    add_includedirs(engine_public_includes, { public = true })
    add_includedirs("src", { public = true })

    for _, module in ipairs(engine_modules) do
        local module_group = module.name

        if os.isdir(module.public_dir) then
            add_includedirs(module.public_dir, { public = true })
            add_headerfiles(path.join(module.public_dir, "**.h"), {
                public = true,
                group = module_group .. "/Public/Headers",
                prefixdir = path.join(module.name, "Public")
            })
            add_files(path.join(module.public_dir, "**.cpp"), {
                group = module_group .. "/Public/Sources"
            })
        end

        if os.isdir(module.private_dir) then
            add_includedirs(module.private_dir)
            add_headerfiles(path.join(module.private_dir, "**.h"), {
                group = module_group .. "/Private/Headers",
                prefixdir = path.join(module.name, "Private")
            })
            add_files(path.join(module.private_dir, "**.cpp"), {
                group = module_group .. "/Private/Sources"
            })
        end
    end

    -- 🧩 ImGui
    add_files("ThirdParty/ImGui/*.cpp")
    add_files("ThirdParty/ImGui/backends/imgui_impl_sdl3.cpp")
    add_files("ThirdParty/ImGui/backends/imgui_impl_sdlrenderer3.cpp")

    add_linkdirs(sdl3_lib, sdl3_image_lib)
    add_links("SDL3", "SDL3_image")
    

-- ╔══════════════════════════════════════════════════════════════╗
-- ║ 🚀 Target: BixRun (exécutable principal)                     ║
-- ╚══════════════════════════════════════════════════════════════╝
target("BixRun")
    set_kind("binary")
    set_default(true)
    add_deps("BixEngine")

    add_includedirs(engine_public_includes)
    add_linkdirs(sdl3_lib, sdl3_image_lib)
    add_links("SDL3", "SDL3_image")

    if main_module then
        local module_group = main_module.name

        if os.isdir(main_module.public_dir) then
            add_includedirs(main_module.public_dir)
            add_headerfiles(path.join(main_module.public_dir, "**.h"), {
                group = module_group .. "/Public/Headers",
                prefixdir = path.join(main_module.name, "Public")
            })
            add_files(path.join(main_module.public_dir, "**.cpp"), {
                group = module_group .. "/Public/Sources"
            })
        end

        if os.isdir(main_module.private_dir) then
            add_includedirs(main_module.private_dir)
            add_headerfiles(path.join(main_module.private_dir, "**.h"), {
                group = module_group .. "/Private/Headers",
                prefixdir = path.join(main_module.name, "Private")
            })
            add_files(path.join(main_module.private_dir, "**.cpp"), {
                group = module_group .. "/Private/Sources"
            })
        end
    end

    local content_dir = path.join("Build", plat, arch, mode, "Content")
    if os.isdir(content_dir) then

        add_files(path.join(content_dir, "**.cpp"))
        add_includedirs(content_dir, { public = true })
        
        print("[BixEngine] 🧩 Inclusion des scripts utilisateur depuis : " .. content_dir)
    else
        print("[BixEngine] ⚠️ Aucun dossier Content trouvé à : " .. content_dir)
    end

    after_build(function(target)
        local exe_dir = path.directory(target:targetfile())
        local dlls =
        {
            path.join(sdl3_lib, "SDL3.dll"),
            path.join(sdl3_image_lib, "SDL3_image.dll"),
        }
    
        for _, dll_path in ipairs(dlls) do
            if os.isfile(dll_path) then
                os.cp(dll_path, exe_dir)
                print("[BixEngine] 📦 Copie de " .. path.filename(dll_path) .. " → " .. exe_dir)
            else
                print("[BixEngine] ⚠️ DLL manquante : " .. dll_path)
            end
        end
    end)


-- ╔══════════════════════════════════════════════════════════════╗
-- ║ 🧰 Task: regen (regenerate reflection headers)                ║
-- ╚══════════════════════════════════════════════════════════════╝
task("regen")
    set_category("action")
    on_run(function()
        import("Tools.xmake.reflection")
        print("[BixEngine] ♻️ Regénération manuelle des headers forcée...")
        local reflection = import("Tools.xmake.reflection")
        reflection.generate_headers(true, generated_dir)
    end)

    set_menu {
        usage = "xmake regen [options]",
        description = "Regénère les fichiers d'en-tête générés (reflection)",
        options = {
            { nil, "force", "k", nil, "Forcer la régénération complète avant la compilation" }
        }
    }

-- ╔══════════════════════════════════════════════════════════════╗
-- ║ 🏗️ Task: fullbuild (regen + full rebuild)                    ║
-- ╚══════════════════════════════════════════════════════════════╝
task("fullbuild")
    set_category("action")
    on_run(function()
        import("core.project.task")
        import("core.base.option")

        task.run("regen", { force = option.get("force") or false })
        os.exec("xmake build")
    end)

    set_menu {
        usage = "xmake fullbuild [options]",
        description = "Régénère les headers puis reconstruit entièrement BixEngine",
        options = {
            { nil, "force", "k", nil, "Forcer la régénération complète avant la compilation" }
        }
    }

-- ╔══════════════════════════════════════════════════════════════╗
-- ║ 🧹 Task: cleanbuild (clean + full rebuild + run)              ║
-- ╚══════════════════════════════════════════════════════════════╝
task("cleanbuild")
    set_category("action")

    on_run(function()
        import("core.project.task")
        import("core.project.config")

        os.exec(string.format("xmake f -m %s", mode))
        
        print("[BixEngine] 🧹 Nettoyage complet du projet...")

        os.exec("xmake clean -a")
        os.tryrm(".xmake")
        os.tryrm("Build")
        os.tryrm("Intermediate")
        os.tryrm("bin")
        os.tryrm("Binaries")
        os.tryrm("logs")
        os.tryrm("crashlogs")
        os.tryrm("SDL3.dll")

        print("[BixEngine] ✅ Nettoyage terminé.")
        print("[BixEngine] ⚙️  Regénération des headers...")
        
        os.exec(string.format("xmake f -m %s", mode))

         print(string.format("[BixEngine] 🔨 Reconstruction de BixHeaderTool (mode: %s)...", mode))
        os.exec("xmake build BixHeaderTool")

        print("[BixEngine] ⚙️  Regénération des headers...")
        task.run("regen", { force = true })

        print("[BixEngine] 🏗️ Reconstruction complète du moteur...")
        os.exec("xmake build BixRun")

        print("[BixEngine] 🚀 Lancement du moteur...")
        os.exec("xmake run BixRun")
    end)

    set_menu {
        usage = "xmake cleanbuild",
        description = "Nettoie, regénère et reconstruit entièrement BixEngine."
    }