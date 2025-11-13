local config = import("core.project.config")
local M = {}

local function get_config_or_default(name, default)
    if config and config.get then
        local value = config.get(name)
        if value ~= nil then
            return value
        end
    end
    return default
end

function M.generate_headers(force, generated_dir)
    cprint("${bright cyan}──────────────────────────────────────────────")
    cprint("${bright cyan}[Reflection] Démarrage de la génération des headers...")
    cprint("${bright cyan}──────────────────────────────────────────────")

    local plat = get_config_or_default("plat", os.host())
    local arch = get_config_or_default("arch", os.arch())
    local mode = get_config_or_default("mode", "debug")

    local content_path = path.join(os.projectdir(), "Build", plat, arch, mode, "Content")
    local generated_abs = path.join(os.projectdir(), generated_dir)

    cprint(string.format("${dim blue}→ Dossier de sortie : %s", generated_abs))

    if force then
        cprint("${bright yellow}[*] Suppression complète des fichiers générés (--force)")
        if os.isdir(generated_abs) then
            for _, f in ipairs(os.files(path.join(generated_abs, "*.generated.h"))) do
                cprint(string.format("${dim red}   ✖ Suppression : %s", f))
                os.rm(f)
            end
        end
    end

    if not os.isdir(generated_abs) then
        cprint(string.format("${bright yellow}[*] Création du dossier manquant : %s", generated_abs))
        os.mkdir(generated_abs)
    else
        cprint("${dim green}[✓] Dossier de génération déjà présent.")
    end

    local gen_names = {}
    cprint("${bright yellow}[*] Recherche des includes .generated.h dans les headers...")

    local header_patterns = {
        "src/**/public/**.h"
    }

    if os.isdir(content_path) then
        table.insert(header_patterns, path.join(content_path, "**.h"))
    end

    for _, pattern in ipairs(header_patterns) do
        for _, header in ipairs(os.files(pattern)) do
            local content = io.readfile(header)
            if content then
                local inc = content:match([[#include%s+"([%w_]+%.generated%.h)"]])
                if inc then
                    gen_names[inc] = true
                    cprint(string.format("${dim white}   + Trouvé : %s (dans %s)", inc, header))
                end
            end
        end
    end

    local count = 0
    for _ in pairs(gen_names) do
        count = count + 1
    end

    if count == 0 then
        cprint("${bright red}[!] Aucun include .generated.h trouvé — rien à générer.")
        return
    else
        cprint(string.format("${bright green}[✓] %d includes trouvés, génération en cours...", count))
    end

    cprint("${bright yellow}[*] Exécution de BixHeaderTool pour générer les headers réels...")

    local tool_target = "BixHeaderTool"
    local mode_dir = get_config_or_default("mode", "debug")

    local tool_exe = path.join(os.projectdir(), "build", mode_dir, tool_target .. ".exe")
    if not os.isfile(tool_exe) then
        tool_exe = path.join(os.projectdir(), "Build", os.host(), os.arch(), mode_dir, tool_target .. ".exe")
    end

    cprint(string.format("${dim blue}→ Résolution du binaire : %s", tool_exe))

    if not os.isfile(tool_exe) then
        cprint("${bright red}[!] Erreur : BixHeaderTool introuvable, impossible de générer les headers.")
        return
    end

    local header_roots = {
        path.join(os.projectdir(), "src")
    }

    if os.isdir(content_path) then
        table.insert(header_roots, content_path)
    else
        table.insert(header_roots, path.join(os.projectdir(), "src"))
    end

    table.insert(header_roots, generated_abs)

    local ok = os.execv(tool_exe, header_roots)
    if ok ~= 0 then
        cprint("${bright red}[!] Échec de l'exécution de BixHeaderTool :" .. tostring(ok))
        return
    end

    cprint("${bright green}[✓] BixHeaderTool exécuté avec succès.")

    cprint("${bright yellow}[*] Nettoyage des fichiers vides...")
    for _, f in ipairs(os.files(path.join(generated_abs, "*.generated.h"))) do
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
