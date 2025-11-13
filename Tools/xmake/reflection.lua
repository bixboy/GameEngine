import("core.project.config")

local M = {}

local function collect_header_roots()
    local roots = {}
    local visited = {}

    for _, dir in ipairs(os.dirs(path.join(os.projectdir(), "src/**"))) do
        if os.isdir(dir) and not visited[dir] then
            visited[dir] = true
            table.insert(roots, dir)
        end
    end

    local plat = get_config("plat") or os.host()
    local arch = get_config("arch") or os.arch()
    local mode = get_config("mode") or "debug"
    local content_dir = path.join(os.projectdir(), "Build", plat, arch, mode, "Content")
    if os.isdir(content_dir) and not visited[content_dir] then
        visited[content_dir] = true
        table.insert(roots, content_dir)
    end

    return roots
end

local function needs_generation(roots)
    for _, root in ipairs(roots) do
        for _, header in ipairs(os.files(path.join(root, "**.h"))) do
            local content = io.readfile(header)
            if content and content:find('#include%s+"[%w_]+%.generated%.h"') then
                return true
            end
        end
    end
    return false
end

function M.generate_headers(force, generated_dir)
    local output_dir = path.join(os.projectdir(), generated_dir)

    cprint("${bright cyan}──────────────────────────────────────────────")
    cprint("${bright cyan}[Reflection] Démarrage de la génération des headers...")
    cprint("${bright cyan}──────────────────────────────────────────────")
    cprint(string.format("${dim blue}→ Dossier de sortie : %s", output_dir))

    if force and os.isdir(output_dir) then
        cprint("${bright yellow}[*] Suppression complète des fichiers générés (--force)")
        for _, f in ipairs(os.files(path.join(output_dir, "*.generated.h"))) do
            cprint(string.format("${dim red}   ✖ Suppression : %s", f))
            os.rm(f)
        end
    end

    if not os.isdir(output_dir) then
        cprint(string.format("${bright yellow}[*] Création du dossier manquant : %s", output_dir))
        os.mkdir(output_dir)
    else
        cprint("${dim green}[✓] Dossier de génération déjà présent.")
    end

    local header_roots = collect_header_roots()
    if not needs_generation(header_roots) then
        cprint("${bright red}[!] Aucun include .generated.h trouvé — rien à générer.")
        return
    end

    cprint("${bright yellow}[*] Exécution de BixHeaderTool pour générer les headers réels...")

    local tool_target = "BixHeaderTool"
    local mode = get_config("mode") or "debug"
    local tool_exe = path.join(os.projectdir(), "build", mode, tool_target .. ".exe")
    if not os.isfile(tool_exe) then
        tool_exe = path.join(os.projectdir(), "Build", os.host(), os.arch(), mode, tool_target .. ".exe")
    end

    cprint(string.format("${dim blue}→ Résolution du binaire : %s", tool_exe))

    if not os.isfile(tool_exe) then
        cprint("${bright red}[!] Erreur : BixHeaderTool introuvable, impossible de générer les headers.")
        return
    end

    if #header_roots == 0 then
        cprint("${bright red}[!] Aucun dossier d'includes valide à analyser.")
        return
    end

    local args = {}
    for _, dir in ipairs(header_roots) do
        table.insert(args, dir)
    end
    table.insert(args, output_dir)

    local code, _, err = os.execv(tool_exe, args)
    if code ~= 0 then
        cprint("${bright red}[!] Échec de l'exécution de BixHeaderTool :")
        if err then
            print(err)
        end
        return
    end

    cprint("${bright green}[✓] BixHeaderTool exécuté avec succès.")

    cprint("${bright yellow}[*] Nettoyage des fichiers vides...")
    for _, f in ipairs(os.files(path.join(output_dir, "*.generated.h"))) do
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
