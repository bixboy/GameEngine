#pragma once

namespace BixEngine::Gui::Widgets::Layout
{
    /** Ajoute un espacement vertical court (utilisé après les headers ou sections). */
    void SmallVerticalSpacing(float height = 4.0f) noexcept;

    /** Ajoute un espacement vertical marqué pour séparer des blocs de contenu. */
    void SectionSpacing(float height = 8.0f) noexcept;
}
