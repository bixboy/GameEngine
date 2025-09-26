# Script Creation Utilities

The `create_script.py` helper replicates parts of Unreal Engine's workflow for
adding new gameplay scripts.  It ensures that when a new class is generated, the
header lives inside a module's **Public** directory while the source file is
created in the mirrored **Private** directory beneath `Engine/Source`.  The
behaviour is symmetrical, so invoking the tool from either side keeps the pair
in sync automatically.

```bash
python Tools/create_script.py --list
python Tools/create_script.py MyActor --template Actor --location Engine/Source/Game/Public/Game
```

Templates are defined in `script_templates.json`.  Each entry controls the
include directives and boilerplate for both files.  Passing `--allow` when
running the script restricts the menu to a curated subset, similar to Unreal
Engine's "Add C++ Class" dialogue.

By default the tool updates the Visual Studio project and filter files so the
new sources appear immediately inside the IDE.  Use `--no-project` to skip this
step when experimenting.

Launching the script without arguments opens a graphical interface that mirrors
Unreal Engine's workflow.  From there you can pick the target module, choose
between the Public and Private trees, and specify the relative sub-path.  The
generated files and project metadata are updated automatically, making the
workflow seamless in both Visual Studio and JetBrains Rider.
