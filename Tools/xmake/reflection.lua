import("core.project.config")
local M = {}

function generate_headers(force, generated_dir)

    cprint("${bright cyan}──────────────────────────────────────────────")
    cprint("${bright cyan}[Reflection] Démarrage de la génération des headers...")
    cprint("${bright cyan}──────────────────────────────────────────────")

    cprint(string.format("${dim blue}→ Dossier de sortie : %s", generated_dir))

    -- 1️⃣ Suppression des anciens fichiers si --force
    if force then
        cprint("${bright yellow}[*] Suppression complète des fichiers générés (--force)")
        if os.isdir(generated_dir) then
            for _, f in ipairs(os.files(path.join(generated_dir, "*.generated.h"))) do
                cprint(string.format("${dim red}   ✖ Suppression : %s", f))
                os.rm(f)
            end
        end
    end

    -- 2️⃣ Création du dossier s'il n'existe pas
    if not os.isdir(generated_dir) then
        cprint(string.format("${bright yellow}[*] Création du dossier manquant : %s", generated_dir))
        os.mkdir(generated_dir)
    else
        cprint("${dim green}[✓] Dossier de génération déjà présent.")
    end

    -- 3️⃣ Scan des headers pour trouver les includes .generated.h
    local gen_names = {}
    cprint("${bright yellow}[*] Recherche des includes .generated.h dans les headers...")

    local function scan_headers(path_pattern)
        for _, header in ipairs(os.files(path_pattern)) do
            local content = io.readfile(header)
            if content then
                local inc = content:match([[#include%s+"(%w+%.generated%.h)"]])
                if inc then
                    gen_names[inc] = true
                    cprint(string.format("${dim white}   + Trouvé : %s (dans %s)", inc, header))
                end
            end
        end
    end

    -- 🔍 Chemin dynamique vers le dossier Content
    local plat = get_config("plat") or os.host()
    local arch = get_config("arch") or os.arch()
    local mode = get_config("mode") or "debug"
    local content_path = path.join(os.projectdir(), "Build", plat, arch, mode, "Content")
    
    -- Scans
    scan_headers("src/**.h")
    
    if os.isdir(content_path) then
        local content_pattern = path.join(content_path, "**.h")
        scan_headers(content_pattern)
    end

    local count = 0
    for _ in pairs(gen_names) do count = count + 1 end

    if count == 0 then
        cprint("${bright red}[!] Aucun include .generated.h trouvé — rien à générer.")
        return
    else
        cprint(string.format("${bright green}[✓] %d includes trouvés, génération en cours...", count))
    end

    -- 4️⃣ Création des fichiers stub manquants
    cprint("${bright yellow}[*] Exécution de BixHeaderTool pour générer les headers réels...")
    
    -- Recherche automatique du binaire
    local tool_target = "BixHeaderTool"
    local mode = get_config("mode") or "debug"
    
    -- XMake place les binaires ici par défaut :
    local tool_exe = path.join(os.projectdir(), "build", mode, tool_target .. ".exe")
    
    if not os.isfile(tool_exe) then
        -- Fallback : version alternative si tu utilises ton dossier Build personnalisé
        tool_exe = path.join(os.projectdir(), "Build", os.host(), os.arch(), mode, tool_target .. ".exe")
    end
    
    cprint(string.format("${dim blue}→ Résolution du binaire : %s", tool_exe))
    
    if not os.isfile(tool_exe) then
        cprint("${bright red}[!] Erreur : BixHeaderTool introuvable, impossible de générer les headers.")
        return
    end
    
    -- Arguments
    local args = {
        path.join(os.projectdir(), "src"),
        path.join(os.projectdir(), "Build", os.host(), os.arch(), mode, "Content"),
        path.join(os.projectdir(), generated_dir)
    }
    
    -- Exécution
    local ok, out, err = os.execv(tool_exe, args)
    if ok ~= 0 then
        cprint("${bright red}[!] Échec de l'exécution de BixHeaderTool :")
        print(err or "Erreur inconnue")
        return
    end
    
    cprint("${bright green}[✓] BixHeaderTool exécuté avec succès.")


    -- 6️⃣ Nettoyage des fichiers vides
    cprint("${bright yellow}[*] Nettoyage des fichiers vides...")
    for _, f in ipairs(os.files(path.join(generated_dir, "*.generated.h"))) do

        local size = os.filesize(f)
        if size == 0 then
            cprint(string.format("${dim red}   ✖ Suppression (vide) : %s", f))
            os.rm(f)
        else
            cprint(string.format("${dim green}   ✓ Valide : %s (%d bytes)", f, size))
        end
    end

    cprint("${bright green}[+] Fichiers .generated.h mis à jour avec succès.")
    cprint("${bright cyan}──────────────────────────────────────────────\n")
end

return M