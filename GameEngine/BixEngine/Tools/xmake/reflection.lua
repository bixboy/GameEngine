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

    scan_headers("Runtime/Include/**.h")
    scan_headers("Samples/**.h")

    local count = 0
    for _ in pairs(gen_names) do count = count + 1 end

    if count == 0 then
        cprint("${bright red}[!] Aucun include .generated.h trouvé — rien à générer.")
        return
    else
        cprint(string.format("${bright green}[✓] %d includes trouvés, génération en cours...", count))
    end

    -- 4️⃣ Création des fichiers stub manquants
    cprint("${bright yellow}[*] Vérification / création des fichiers stub manquants...")
    for gen_file, _ in pairs(gen_names) do

        local gen_path = path.join(generated_dir, gen_file)
        if not os.isfile(gen_path) then

            io.writefile(gen_path, "")
            cprint(string.format("${dim green}   + Stub créé : %s", gen_path))
        else
            cprint(string.format("${dim blue}   = Déjà existant : %s", gen_path))
        end
    end

    -- 5️⃣ Exécution du BixHeaderTool
    cprint("${bright yellow}[*] Exécution de BixHeaderTool pour générer les headers réels...")

    local builddir = os.isdir(path.join(os.projectdir(), "Build")) and "Build" or "build"
    local mode = config.get("mode") or "release"
    local tool_exe = path.join(os.projectdir(), builddir, os.host(), os.arch(), mode, "BixHeaderTool.exe")

    if is_plat("windows") then
        tool_exe = tool_exe .. ".exe"
    end

    cprint(string.format("${dim blue}→ Résolution du binaire : %s", tool_exe))
    if not os.isfile(tool_exe) then

        cprint("${bright red}[!] Erreur : BixHeaderTool introuvable au chemin ci-dessus.")
        return
    end

    cprint(string.format("${dim blue}→ Commande : %s Runtime/Include Samples %s", tool_exe, generated_dir))
    
    local args = {
        path.join(os.projectdir(), "Runtime/Include"),
        path.join(os.projectdir(), "Samples"),
        path.join(os.projectdir(), generated_dir)
    }

    cprint(string.format("${dim blue}→ Lancement de : %s %s", tool_exe, table.concat(args, " ")))

    local ok, err = os.iorunv(tool_exe, args)  -- capture la sortie complète
    if not ok then
        cprint("${bright red}[!] Erreur pendant l'exécution du BixHeaderTool :")
        print(err or "Aucune sortie capturée.")
        return
    end

    print(ok)

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