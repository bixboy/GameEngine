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
    raise("❌ Veuillez définir *à la fois* sdl3_dir et sdl3_lib_dir pour utiliser SDL3 externe.")
end

-- ────────────────────────────────────────────────────────────────
-- 🗂️ Generated headers directory
-- ────────────────────────────────────────────────────────────────
local plat = get_config("plat") or os.host()
local arch = get_config("arch") or os.arch()
local mode = get_config("mode") or "debug"

local generated_dir = path.join("Build", plat, arch, mode, "Intermediate", "GeneratedHeaders")

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
        local header_paths = 
        {
            "Runtime/Include/**.h",
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

        -- 🔍 Vérifie si un .generated.h existe et récupère le plus récent
        local newest_generated_time = 0
        local has_generated = false

        for _, gen in ipairs(os.files(path.join(generated_dir, "*.generated.h"))) do

            has_generated = true
            local mtime = os.mtime(gen)

            if mtime > newest_generated_time then
                newest_generated_time = mtime
            end
        end

        -- 🧠 Décision : régénérer ou pas ?
        local need_regen = false
        local missing_generated = {}

        -- 🧩 Vérifie si chaque .h a un équivalent .generated.h
        for _, pattern in ipairs(header_paths) do
            for _, header in ipairs(os.files(pattern)) do
                local name = path.basename(header)
                local gen_path = path.join(generated_dir, name .. ".generated.h")
                if not os.isfile(gen_path) then
                    table.insert(missing_generated, gen_path)
                end
            end
        end

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
            print("[GenerateHeaders] 🔄 Des headers ont été modifiés — régénération nécessaire.")
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

    add_files("Runtime/Source/**.cpp")
    add_includedirs("Runtime/Include", { public = true })

    -- 🧩 ImGui
    add_files("ThirdParty/ImGui/*.cpp")
    add_files("ThirdParty/ImGui/backends/imgui_impl_sdl3.cpp")
    add_files("ThirdParty/ImGui/backends/imgui_impl_sdlrenderer3.cpp")
    add_includedirs("ThirdParty/ImGui", "ThirdParty/ImGui/backends", { public = true })

    -- 🧩 SDL3 + Generated
    add_includedirs(sdl3_inc, generated_dir, { public = true })
    add_linkdirs(sdl3_lib)
    add_links("SDL3")
    

-- ╔══════════════════════════════════════════════════════════════╗
-- ║ 🚀 Target: BixRun (exécutable principal)                     ║
-- ╚══════════════════════════════════════════════════════════════╝
target("BixRun")
    set_kind("binary")
    set_default(true)
    add_files("BixRun/main.cpp")
    add_deps("BixEngine")

    add_includedirs("Runtime/Include", sdl3_inc, generated_dir)
    add_linkdirs(sdl3_lib)
    add_links("SDL3")
    
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
        local dll_path = path.join(sdl3_lib, "SDL3.dll")
        if os.isfile(dll_path) then
            os.cp(dll_path, exe_dir)
            print("[BixEngine] 📦 Copie de SDL3.dll → " .. exe_dir)
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
