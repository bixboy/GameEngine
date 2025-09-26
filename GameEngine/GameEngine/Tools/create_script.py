#!/usr/bin/env python3
"""Utility for generating paired header/source gameplay scripts.

This tool mimics part of the workflow provided by Unreal Engine when creating
new gameplay scripts.  Given a desired class name and the directory where the
script should appear, it will automatically generate the corresponding `.h`
file inside the Public tree and the `.cpp` file inside the Private tree.  The
behaviour is symmetrical: starting from a directory under either tree produces
the complementary file in the matching directory of the other tree.

Templates that describe the generated content are configurable via the
`script_templates.json` file that sits next to this script.  The configuration
controls the list of choices exposed to the user when no template is provided
explicitly.  Templates can also be filtered with `--allow` to mimic Unreal
Engine's "Add C++ Class" dialogue, which offers a curated set of base classes.

Example usages:
    # List available templates
    python Tools/create_script.py --list

    # Create MyActor inside Engine/Public/Game/Actors using the "Actor" template
    python Tools/create_script.py MyActor --template Actor \
        --location Engine/Public/Game/Actors

    # Same, but starting from a Private directory.  The header will be written
    # to the mirrored Public location automatically.
    python Tools/create_script.py MyActor --template Actor \
        --location Engine/Private/Game/Actors
"""

from __future__ import annotations

import argparse
import json
import os
import re
import sys
from pathlib import Path
from typing import Dict, Iterable, List, Optional, Tuple
import xml.etree.ElementTree as ET

# ---------------------------------------------------------------------------
# Configuration loading
# ---------------------------------------------------------------------------


def load_templates(config_path: Path) -> Dict[str, object]:
    try:
        with config_path.open("r", encoding="utf-8") as fh:
            data = json.load(fh)
    except FileNotFoundError as exc:  # pragma: no cover - configuration required
        raise SystemExit(
            f"Template configuration not found at '{config_path}'."
        ) from exc
    except json.JSONDecodeError as exc:  # pragma: no cover - handled at runtime
        raise SystemExit(
            f"Failed to parse template configuration '{config_path}': {exc}"
        ) from exc

    if "templates" not in data or not isinstance(data["templates"], list):
        raise SystemExit("Configuration file must contain a 'templates' list.")

    return data


# ---------------------------------------------------------------------------
# Utility helpers
# ---------------------------------------------------------------------------


def sanitise_identifier(value: str) -> str:
    """Return a string that is safe to use as a namespace component."""

    if not value:
        return value

    # Replace invalid characters with underscores and ensure we start with a
    # valid letter or underscore.
    value = re.sub(r"[^0-9A-Za-z_]", "_", value)
    if not re.match(r"[A-Za-z_]", value):
        value = f"_{value}"
    return value


def indent_lines(lines: Iterable[str], prefix: str) -> List[str]:
    result: List[str] = []
    for line in lines:
        if line:
            result.append(f"{prefix}{line}")
        else:
            result.append("")
    return result


def deduplicate_preserve_order(items: Iterable[str]) -> List[str]:
    seen: set[str] = set()
    ordered: List[str] = []
    for item in items:
        if item not in seen:
            seen.add(item)
            ordered.append(item)
    return ordered


# ---------------------------------------------------------------------------
# Template resolution and rendering
# ---------------------------------------------------------------------------


def filter_templates(
    templates: List[Dict[str, object]], allowed: Optional[Iterable[str]]
) -> List[Dict[str, object]]:
    if allowed is None:
        return templates

    allowed_set = {entry.lower() for entry in allowed}
    filtered: List[Dict[str, object]] = []
    for template in templates:
        aliases = [template.get("name", "")] + template.get("aliases", [])
        aliases = [alias for alias in aliases if isinstance(alias, str)]
        alias_matches = any(alias.lower() in allowed_set for alias in aliases)

        header_info = template.get("header", {})
        header_includes = header_info.get("includes", [])
        header_includes = [inc for inc in header_includes if isinstance(inc, str)]
        include_matches = any(include.lower() in allowed_set for include in header_includes)

        if alias_matches or include_matches:
            filtered.append(template)

    return filtered


def prompt_user_to_select_template(
    templates: List[Dict[str, object]]
) -> Dict[str, object]:
    if not sys.stdin.isatty():
        raise SystemExit(
            "No template provided and standard input is not interactive; "
            "use --template or --allow to specify a choice explicitly."
        )

    print("Available templates:")
    for idx, template in enumerate(templates, start=1):
        description = template.get("description", "")
        if description:
            print(f"  {idx}. {template['name']} - {description}")
        else:
            print(f"  {idx}. {template['name']}")

    while True:
        try:
            selection = input("Select template number: ").strip()
        except EOFError:  # pragma: no cover - user abort
            raise SystemExit("No template selected.")

        if not selection:
            continue
        if not selection.isdigit():
            print("Please enter a valid number.")
            continue

        index = int(selection) - 1
        if 0 <= index < len(templates):
            return templates[index]
        print("Selection out of range.")


def build_namespace_parts(
    namespace_root: Optional[str], relative_parts: Iterable[str]
) -> List[str]:
    parts: List[str] = []
    if namespace_root:
        parts.append(namespace_root)
    parts.extend(sanitise_identifier(part) for part in relative_parts if part)
    return [part for part in parts if part]


def render_includes(raw_includes: Iterable[str], format_args: Dict[str, str]) -> List[str]:
    includes: List[str] = []
    for include in raw_includes:
        formatted = include.format(**format_args)
        if formatted.startswith('<') and formatted.endswith('>'):
            include_line = f"#include {formatted}"
        elif formatted.startswith('"') and formatted.endswith('"'):
            include_line = f"#include {formatted}"
        else:
            include_line = f"#include \"{formatted}\""
        includes.append(include_line)

    return deduplicate_preserve_order(includes)


def render_section(
    section: Dict[str, object],
    namespace_parts: List[str],
    format_args: Dict[str, str]
) -> List[str]:
    includes = section.get("includes", [])
    body = section.get("body", [])

    if not isinstance(includes, list) or not isinstance(body, list):
        raise SystemExit("Template sections must define 'includes' and 'body' lists.")

    rendered_includes = render_includes(includes, format_args)

    rendered_body = [line.format(**format_args) for line in body]
    if namespace_parts:
        namespace = "::".join(namespace_parts)
        namespace_open = f"namespace {namespace}"
        namespace_close = f"}} // namespace {namespace}"
        rendered_body = (
            [namespace_open, "{"]
            + indent_lines(rendered_body, "    ")
            + [namespace_close]
        )

    if rendered_body and rendered_includes:
        return rendered_includes + [""] + rendered_body
    return rendered_includes + rendered_body


def validate_class_name(name: str) -> None:
    if not name:
        raise SystemExit("Class name must not be empty.")
    if not re.match(r"^[A-Za-z_][A-Za-z0-9_]*$", name):
        raise SystemExit(
            "Class name must be a valid C++ identifier (letters, digits, underscore, "
            "not starting with a digit)."
        )


# ---------------------------------------------------------------------------
# Project file management
# ---------------------------------------------------------------------------


def ensure_item_group(
    root: ET.Element, namespace: str, item_tag: str
) -> ET.Element:
    ns = {"msbuild": namespace}
    for group in root.findall("msbuild:ItemGroup", ns):
        if group.find(f"msbuild:{item_tag}", ns) is not None:
            return group
    return ET.SubElement(root, f"{{{namespace}}}ItemGroup")


def add_file_to_project(
    project_path: Path,
    item_tag: str,
    relative_path: str,
) -> None:
    tree = ET.parse(project_path)
    root = tree.getroot()
    namespace_match = re.match(r"\{([^}]*)\}", root.tag)
    namespace = namespace_match.group(1) if namespace_match else ""

    ET.register_namespace("", namespace)
    group = ensure_item_group(root, namespace, item_tag)

    existing_paths = {elem.get("Include") for elem in group.findall(f"{{{namespace}}}{item_tag}")}
    if relative_path not in existing_paths:
        new_elem = ET.SubElement(group, f"{{{namespace}}}{item_tag}")
        new_elem.set("Include", relative_path)

    ET.indent(tree, space="  ")
    tree.write(project_path, encoding="utf-8", xml_declaration=True)


def add_file_to_filters(
    filters_path: Path,
    item_tag: str,
    relative_path: str,
    filter_name: str,
) -> None:
    tree = ET.parse(filters_path)
    root = tree.getroot()
    namespace_match = re.match(r"\{([^}]*)\}", root.tag)
    namespace = namespace_match.group(1) if namespace_match else ""

    ET.register_namespace("", namespace)
    ns = {"msbuild": namespace}

    target_group: Optional[ET.Element] = None
    for group in root.findall("msbuild:ItemGroup", ns):
        if group.find(f"msbuild:{item_tag}", ns) is not None:
            target_group = group
            break

    if target_group is None:
        target_group = ET.SubElement(root, f"{{{namespace}}}ItemGroup")

    existing_paths = {elem.get("Include") for elem in target_group.findall(f"{{{namespace}}}{item_tag}")}
    if relative_path in existing_paths:
        ET.indent(tree, space="  ")
        tree.write(filters_path, encoding="utf-8", xml_declaration=True)
        return

    new_elem = ET.SubElement(target_group, f"{{{namespace}}}{item_tag}")
    new_elem.set("Include", relative_path)
    filter_elem = ET.SubElement(new_elem, f"{{{namespace}}}Filter")
    filter_elem.text = filter_name

    ET.indent(tree, space="  ")
    tree.write(filters_path, encoding="utf-8", xml_declaration=True)


# ---------------------------------------------------------------------------
# File creation logic
# ---------------------------------------------------------------------------


def determine_locations(
    project_root: Path,
    class_name: str,
    location: Path,
) -> Tuple[Path, Path, Path, Path, List[str]]:
    engine_root = project_root / "Engine"
    public_root = engine_root / "Public"
    private_root = engine_root / "Private"

    location = location.resolve()
    try:
        relative_to_public = location.relative_to(public_root)
        in_public = True
        relative_parts = list(relative_to_public.parts)
    except ValueError:
        in_public = False
        relative_to_public = None

    try:
        relative_to_private = location.relative_to(private_root)
        in_private = True
        private_parts = list(relative_to_private.parts)
    except ValueError:
        in_private = False
        private_parts = []

    if in_public:
        header_dir = location
        source_dir = private_root / relative_to_public
    elif in_private:
        source_dir = location
        header_dir = public_root / relative_to_private
        relative_parts = private_parts
    else:
        raise SystemExit(
            "Location must reside under either Engine/Public or Engine/Private."
        )

    header_path = header_dir / f"{class_name}.h"
    source_path = source_dir / f"{class_name}.cpp"

    namespace_parts = relative_parts

    return header_dir, source_dir, header_path, source_path, namespace_parts


def ensure_directories(*directories: Path) -> None:
    for directory in directories:
        directory.mkdir(parents=True, exist_ok=True)


def write_file(path: Path, lines: List[str], force: bool) -> None:
    if path.exists() and not force:
        raise SystemExit(f"File '{path}' already exists. Use --force to overwrite.")

    content = "\n".join(lines).rstrip() + "\n"
    path.write_text(content, encoding="utf-8")


# ---------------------------------------------------------------------------
# CLI entry point
# ---------------------------------------------------------------------------


def parse_arguments() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Generate paired script files.")
    parser.add_argument("class_name", nargs="?", help="Name of the class to generate.")
    parser.add_argument(
        "--template",
        "-t",
        dest="template_name",
        help="Template to use (see --list).",
    )
    parser.add_argument(
        "--location",
        "-l",
        dest="location",
        help="Directory under Engine/Public or Engine/Private where the script should be created.",
    )
    parser.add_argument(
        "--allow",
        nargs="+",
        help="Restrict templates to the provided names or header includes.",
    )
    parser.add_argument(
        "--list",
        action="store_true",
        help="List available templates and exit.",
    )
    parser.add_argument(
        "--force",
        action="store_true",
        help="Overwrite files if they already exist.",
    )
    parser.add_argument(
        "--no-project",
        action="store_true",
        help="Do not update the Visual Studio project files.",
    )
    return parser.parse_args()


def resolve_location_path(project_root: Path, location: Optional[str]) -> Path:
    if not location:
        if not sys.stdin.isatty():
            raise SystemExit(
                "Location not provided and input is not interactive; please "
                "supply --location."
            )
        location = input(
            "Enter target directory (under Engine/Public or Engine/Private): "
        ).strip()
    if not location:
        raise SystemExit("Location must not be empty.")

    location_path = Path(location)
    if not location_path.is_absolute():
        location_path = (project_root / location_path).resolve()
    return location_path


def main() -> None:
    args = parse_arguments()
    script_path = Path(__file__).resolve()
    project_root = script_path.parent.parent
    config_path = script_path.with_name("script_templates.json")

    config = load_templates(config_path)
    templates_raw = config.get("templates", [])
    templates = [t for t in templates_raw if isinstance(t, dict)]

    if len(sys.argv) == 1:
        # 🚀 Pas d'arguments → lancer en mode GUI
        run_gui(project_root, templates, config)
        return

    if args.list:
        filtered = filter_templates(templates, args.allow)
        if not filtered:
            print("No templates match the provided filters." )
            return
        for template in filtered:
            description = template.get("description", "")
            if description:
                print(f"{template['name']}: {description}")
            else:
                print(template["name"])
        return

    class_name = args.class_name
    if not class_name:
        if not sys.stdin.isatty():
            raise SystemExit(
                "Class name not provided and input is not interactive; supply "
                "the name as a positional argument."
            )
        class_name = input("Enter class name: ").strip()

    validate_class_name(class_name)

    location_path = resolve_location_path(project_root, args.location)

    filtered_templates = filter_templates(templates, args.allow)
    if not filtered_templates:
        raise SystemExit("No templates available after applying filters.")

    template: Dict[str, object]
    if args.template_name:
        matches = [
            t for t in filtered_templates
            if str(t.get("name", "")).lower() == args.template_name.lower()
        ]
        if not matches:
            available_names = ", ".join(t.get("name", "") for t in filtered_templates)
            raise SystemExit(
                f"Template '{args.template_name}' not found. Available: {available_names}"
            )
        template = matches[0]
    else:
        template = prompt_user_to_select_template(filtered_templates)

    header_dir, source_dir, header_path, source_path, relative_namespace_parts = (
        determine_locations(project_root, class_name, location_path)
    )

    ensure_directories(header_dir, source_dir)

    namespace_root = config.get("root_namespace", "")
    namespace_parts = build_namespace_parts(namespace_root, relative_namespace_parts)

    relative_header = header_path.relative_to(project_root / "Engine" / "Public")
    header_include = str(relative_header).replace(os.sep, "/")

    format_args = {
        "class_name": class_name,
        "header_include": header_include,
    }

    header_section = template.get("header", {})
    source_section = template.get("source", {})

    header_lines = render_section(header_section, namespace_parts, format_args)
    if header_lines:
        header_lines = ["#pragma once", ""] + header_lines
    else:
        header_lines = ["#pragma once"]
    source_lines = render_section(source_section, namespace_parts, format_args)

    write_file(header_path, header_lines, args.force)
    write_file(source_path, source_lines, args.force)

    if not args.no_project:
        project_file = project_root / "GameEngine.vcxproj"
        filters_file = project_root / "GameEngine.vcxproj.filters"

        if not project_file.exists() or not filters_file.exists():
            print(
                "Warning: project files not found; skipping Visual Studio project update.",
                file=sys.stderr,
            )
        else:
            header_project_path = str(header_path.relative_to(project_root)).replace("/", "\\")
            source_project_path = str(source_path.relative_to(project_root)).replace("/", "\\")

            add_file_to_project(project_file, "ClInclude", header_project_path)
            add_file_to_project(project_file, "ClCompile", source_project_path)
            add_file_to_filters(filters_file, "ClInclude", header_project_path, "Header Files")
            add_file_to_filters(filters_file, "ClCompile", source_project_path, "Source Files")

    print(f"Created {header_path}")
    print(f"Created {source_path}")


# ---------------------------------------------------------------------------
# GUI mode (Unreal-style "Add C++ Class")
# ---------------------------------------------------------------------------

def run_gui(project_root: Path, templates: List[Dict[str, object]], config: Dict[str, object]):
    import tkinter as tk
    from tkinter import ttk, messagebox

    root = tk.Tk()
    root.title("Add C++ Class (Custom Engine)")
    root.geometry("400x250")

    # Template selection
    tk.Label(root, text="Template:").pack(anchor="w", padx=10, pady=5)
    template_var = tk.StringVar()
    template_names = [tpl["name"] for tpl in templates]
    combo = ttk.Combobox(root, textvariable=template_var, values=template_names, state="readonly")
    combo.current(0)
    combo.pack(fill="x", padx=10)

    # Class name input
    tk.Label(root, text="Class Name:").pack(anchor="w", padx=10, pady=5)
    class_entry = tk.Entry(root)
    class_entry.pack(fill="x", padx=10)

    # Location input
    tk.Label(root, text="Location (under Engine/Public or Engine/Private):").pack(anchor="w", padx=10, pady=5)
    loc_entry = tk.Entry(root)
    loc_entry.insert(0, "Engine/Public/Game")
    loc_entry.pack(fill="x", padx=10)

    def on_create():
        class_name = class_entry.get().strip()
        location = loc_entry.get().strip()
        template_name = template_var.get()

        if not class_name or not location:
            messagebox.showerror("Error", "Class name and location are required.")
            return

        validate_class_name(class_name)
        location_path = resolve_location_path(project_root, location)

        template = next(t for t in templates if t["name"] == template_name)

        header_dir, source_dir, header_path, source_path, ns_parts = determine_locations(
            project_root, class_name, location_path
        )
        ensure_directories(header_dir, source_dir)

        relative_header = header_path.relative_to(project_root / "Engine" / "Public")
        header_include = str(relative_header).replace(os.sep, "/")

        format_args = {
            "class_name": class_name,
            "header_include": header_include,
        }

        header_lines = render_section(template.get("header", {}), ns_parts, format_args)
        source_lines = render_section(template.get("source", {}), ns_parts, format_args)

        if header_lines:
            header_lines = ["#pragma once", ""] + header_lines
        else:
            header_lines = ["#pragma once"]

        write_file(header_path, header_lines, force=True)
        write_file(source_path, source_lines, force=True)

        messagebox.showinfo("Success", f"Created:\n{header_path}\n{source_path}")
        root.destroy()

    # Button
    btn = tk.Button(root, text="Create Class", command=on_create)
    btn.pack(pady=15)

    root.mainloop()

if __name__ == "__main__":
    main()
