## Milestone — Arena Editor
Available in **Debug** and **Development** modes. At any moment during a match, press **F1** to open/hide the Arena Editor, built with Dear ImGui.
When opened, the match pauses (cars and ball lose control) and the camera enters **free-fly mode**: holding the right mouse button lets you mouse-look and fly with W/A/S/D, like in a game engine, to freely position the camera.

**Scope note:** the arena is a bounded box, not an open world — this editor is about *layout and dimensions*, not terrain sculpting. It stays separate from the physics **tuning panels** (Milestone 12): the editor changes *where things are*; the tuning panels change *how they feel*.

**Tools:**
1. **Arena shape:** inputs for floor length & width, wall height, ceiling height, corner radius/chamfer, and goal size (mouth width, height, depth). Editing regenerates the procedural arena mesh and its Jolt static colliders live. A "Reset to default" button restores the standard arena.
2. **Boost pads:** the placeable-object tool. A list with thumbnails for **Big pad** (full boost) and **Small pad** (partial boost). Select one, see a ghost preview snapped to the floor as you move the mouse, and click to place. Holding **Ctrl** and clicking an existing pad removes it. Per-pad fields: refill amount and cooldown.
3. **Spawns:** kickoff spawn markers for **Team Blue** and **Team Orange**, plus the **ball spawn**. Same place/preview/Ctrl-remove interaction. Shown as icons because cars and ball move during play (see Persistence).
4. **(Optional) Props:** low-poly decoration (light rigs, banners, stand blocks) with thumbnails, purely visual, no collision — same interaction as boost pads.

**Mirror symmetry (arena-specific — replaces the RPG mob-distribution feature):**
- A **Mirror** toggle: anything placed on one half of the arena is automatically mirrored to the other half, so the layout stays fair for both teams. Applies to boost pads and (mirrored per team) spawns.
- A **Generate standard layout** button (with confirmation): auto-places the classic layout — 6 big pads (corners + midline) plus the small pads on a symmetric grid — and the standard kickoff spawns for both teams. This is the RL analog of "distribute across the map": instead of scattering mob types by distance, it lays down one canonical, mirror-symmetric arena.
- **Clear all pads** / **Clear all spawns** buttons.

**Persistence:** on leaving edit mode, changes are saved to the arena asset and persist across recompilations and across build modes. Because the match is live while editing, cars and ball will have drifted — so the editor shows **spawn icons** (kickoff positions + ball spawn) and a **Reset positions** button that snaps everything back to its spawn. Only the spawns and the layout are saved, never the live positions.

**Context menu (right-click) on the boost-pad / spawn / prop lists:**
- **Remove all:** removes every instance of that type from the arena.
- **Procedural scatter:** two sliders — radius around where the editor camera is looking, and quantity — plus a button to randomly place that many instances within the radius (respecting Mirror symmetry if enabled).
→ verify: opening F1 pauses play and frees the camera; editing arena dimensions rebuilds mesh + colliders live; pads and spawns place/remove/mirror correctly; "Generate standard layout" yields a symmetric arena; changes survive a rebuild and a build-mode switch.
