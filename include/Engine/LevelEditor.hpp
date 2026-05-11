/// @file LevelEditor.hpp
/// In-game level editor for placing, manipulating, and configuring entities.
/// Activated via the admin dashboard; uses keyboard + mouse for all operations.

#pragma once

#include "Common/Types.hpp"
#include "Engine/Camera.hpp"
#include "Gameplay/EntityManager.hpp"
#include <optional>
#include <memory>
#include <string>
#include <array>

namespace Engine {

/// In-game level editor for placing, manipulating, and configuring entities.
/// Activated via the admin dashboard; uses keyboard + mouse for all operations.
class LevelEditor {
public:
    /// Signals "save the current level and exit to the admin dashboard".
    struct Suggestion {
        std::string levelName;  ///< Name/path to save the level as.
    };

    /// @param entityManager  Reference to the entity manager to edit.
    /// @param camera          Reference to the camera (editor scrolls it with arrows).
    LevelEditor(Gameplay::EntityManager &entityManager, Camera &camera);

    /// Per-frame editor logic: mouse selection, object manipulation, hotkey processing.
    /// @param deltaTime  Time since last frame in seconds.
    /// @param input      Current frame input snapshot.
    void update(float deltaTime, const Common::InputState &input);

    /// Draws selection highlight and the editor HUD overlay.
    /// @param commands       Render command list to append to.
    /// @param textCommands   Text command list for HUD text.
    void render(std::vector<Common::RenderCommand> &commands,
                std::vector<Common::TextCommand> &textCommands);

    /// Whether editor mode is currently active.
    bool isActive() const { return m_active; }
    /// Enables or disables editor mode.
    void setActive(bool active) { m_active = active; }
    /// True if the user pressed Enter indicating they want to save and exit.
    bool hasPendingSubmission() const { return m_pendingSubmission.has_value(); }

private:
    Gameplay::EntityManager &m_entityManager;  ///< The entity manager being edited.
    Camera &m_camera;                           ///< Camera; editor can scroll it.
    std::weak_ptr<Gameplay::Entity> m_selectedEntity;  ///< Currently selected entity.
    bool m_active = false;                      ///< Editor mode toggle.
    bool m_prevMouseDown = false;               ///< Mouse debounce tracking.

    /// Rising-edge debouncing for editor hotkeys (not movement keys).
    enum PrevKey : int {
        PK_C, PK_V, PK_B, PK_T, PK_Y, PK_U, PK_R,
        PK_BACKSPACE,
        PK_NUM1, PK_NUM2, PK_NUM3, PK_NUM4, PK_NUM5,
        PK_NUM6, PK_NUM7, PK_NUM8, PK_NUM9,
        PK_ENTER, PK_L,
        PK_LEFT_BRACKET, PK_RIGHT_BRACKET,
        PK_COUNT
    };
    std::array<bool, PK_COUNT> m_prevPressed = {};  ///< Previous-frame hotkey states.

    std::optional<Suggestion> m_pendingSubmission;  ///< Set when Enter is pressed.

    // --- Editor helper methods ---
    void handleObjectManipulation(float deltaTime, const Common::InputState &input);  ///< WASD move, Shift+WASD resize, arrow camera scroll, C/V/B creation.
    void handlePropertyEditing(const Common::InputState &input);   ///< T/Y/U/R texture/collider/exit/z-index cycles, number-key texture presets.
    void handleLevelConfigEditing(const Common::InputState &input);///< [/] width, L clamp toggle.
    void selectEntityAt(float worldX, float worldY);                ///< Pick topmost entity at world coordinates.
    void createDefaultObject();                                     ///< C: create a StaticObject (platform).
    void createPlayer();                                            ///< V: create a Player spawn point.
    void createBackgroundLayer();                                   ///< B: create a full-screen BackgroundLayer.
    void deleteSelectedObject();                                    ///< Backspace: destroy selected entity.
    void cycleTexture();                                            ///< T: cycle through all TextureIDs.
    void cycleColliderType();                                       ///< Y: toggle none/solid/trigger/hazard.
    void toggleExit();                                              ///< U: toggle east/west exit properties.
    void cycleZIndex();                                             ///< R: cycle through layer presets.
    void setTextureDirect(Common::TextureID textureID);             ///< Number keys: assign specific texture.

    void buildHUDText(std::vector<Common::TextCommand> &textCommands);          ///< Compose the editor overlay text.
    std::string textureName(int id) const;                                      ///< Human-readable name for a TextureID.
    std::string colliderTypeName(const std::shared_ptr<Gameplay::Entity> &entity) const;  ///< "none", "solid", "trigger", "hazard".
    std::string entityTypeName(const std::shared_ptr<Gameplay::Entity> &entity) const;     ///< "Player", "BackgroundLayer", "StaticObject".
    std::string layerName(int z) const;                                             ///< Human-readable name for a z-index value.
};

} // namespace Engine
