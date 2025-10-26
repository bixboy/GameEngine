-- XMake build script for BixEngine

set_xmakever("3.0.4")
add_rules("mode.debug", "mode.release")

add_moduledirs(path.join(os.projectdir(), "tools/xmake"))

set_languages("cxx20")
set_warnings("allextra")
set_optimize("faster")

-- SDL3 configuration
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

if (sdl3_inc == nil or sdl3_inc == "") and (sdl3_lib == nil or sdl3_lib == "") then

    sdl3_inc = path.join(os.projectdir(), "third_party/SDL3-3.2.22/include")
    sdl3_lib = path.join(os.projectdir(), "third_party/SDL3-3.2.22/lib/x64")

elseif (sdl3_inc == nil or sdl3_inc == "") or (sdl3_lib == nil or sdl3_lib == "") then

    raise("Veuillez définir *à la fois* sdl3_dir et sdl3_lib_dir pour utiliser SDL3 externe.")
end

-- Dossier de génération dynamique
local plat = get_config("plat") or os.host()
local arch = get_config("arch") or os.arch()
local mode = get_config("mode") or "debug"

local generated_dir = path.join(
    "build",
    plat,
    arch,
    mode,
    "Intermediate",
    "GeneratedHeaders"
)

-- ✅ Target: BixHeaderTool
target("BixHeaderTool")
    set_kind("binary")
    set_default(false)
    set_plat(os.host())
    set_arch(os.arch())
    set_policy("build.fence", true)

    add_includedirs("tools/BixHeaderTool", { public = true })
    add_files("tools/BixHeaderTool/**.cpp")

-- ✅ Target: BixEngine
target("BixEngine")
    set_kind("static")
    set_default(false)
    add_deps("BixHeaderTool")

    add_files("engine/src/**.cpp")

    add_includedirs("engine/include", { public = true })

    -- 🔹 Fichiers ImGui
    add_files("third_party/ImGui/*.cpp")
    add_files("third_party/ImGui/backends/imgui_impl_sdl3.cpp")
    add_files("third_party/ImGui/backends/imgui_impl_sdlrenderer3.cpp")
    add_includedirs("third_party/ImGui", { public = true })
    add_includedirs("third_party/ImGui/backends", { public = true })


    add_includedirs(path.join(os.projectdir(), "third_party/SDL3-3.2.22/include"), { public = true })
    add_includedirs(path.join(os.projectdir(), generated_dir), { public = true })
    add_linkdirs(sdl3_lib)
    add_links("SDL3")

    -- ✅ Auto-génération AVANT build
    on_load(function(target)
        import("core.project.config")
        local reflection = import("tools.xmake.reflection")
        local generated_dir = path.join(
            "build", os.host(), os.arch(),
            config.get("mode") or "debug",
            "Intermediate", "GeneratedHeaders"
        )

        print("[BixEngine] Vérification des headers générés…")
        if not os.isdir(generated_dir)
            or #os.files(path.join(generated_dir, "*.generated.h")) == 0 then
            print("[BixEngine] → Génération initiale des headers manquants…")
            os.exec("xmake build BixHeaderTool")
            reflection.generate_headers(true, generated_dir)
        else
            print("[BixEngine] Headers déjà présents, aucune génération requise.")
        end
    end)


------------------------------------------------------------
-- ✅ Target: BixRun (exécutable principal)
------------------------------------------------------------
target("BixRun")
    set_kind("binary")
    set_default(true)

    -- Fichier d'entrée (main)
    add_files("apps/bix_run/main.cpp")
    add_deps("BixEngine")

    -- Include + libs
    add_includedirs("engine/include")
    add_includedirs(path.join(os.projectdir(), "third_party/SDL3-3.2.22/include"))
    add_includedirs(path.join(os.projectdir(), generated_dir))
    add_linkdirs(sdl3_lib)
    add_links("SDL3")

    -- Copie automatique de SDL3.dll
    after_build(function(target)
        local exe_dir = path.directory(target:targetfile())
        local dll_path = path.join(sdl3_lib, "SDL3.dll")
        if os.isfile(dll_path) then
            os.cp(dll_path, exe_dir)
        end
    end)

-- ✅ Task: regen
task("regen")
    set_category("action")

    on_run(function()
        import("core.base.option")
        local reflection = import("tools.xmake.reflection")
        
        os.exec("xmake build BixHeaderTool")
        reflection.generate_headers(option.get("force") or false, generated_dir)
    end)

    set_menu {
        usage = "xmake regen [options]",
        description = "Regénérer les fichiers d'en-tête générés (reflection)",
        options = {
            { nil, "force", "k", nil, "Supprimer tous les headers générés avant la régénération" }
        }
    }

-- ✅ Task: fullbuild
task("fullbuild")
    set_category("action")

    on_run(function()
        import("core.base.option")
        import("core.project.task")
        task.run("regen", { force = option.get("force") or false })
        os.exec("xmake build")
    end)

    set_menu {
        usage = "xmake fullbuild [options]",
        description = "Régénérer les headers puis construire entièrement BixEngine",
        options = {
            { nil, "force", "k", nil, "Forcer la régénération complète avant la compilation" }
        }
    }
