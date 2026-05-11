/// @file LevelEditor.cpp
/// Implementation of the LevelEditor class: update/render logic, object
/// manipulation (move/resize/create/delete), property editing (texture,
/// collider, exit, z-index, texture presets), level config editing (width,
/// wall clamping), selection by mouse click, and HUD composition.

#include "Engine/LevelEditor.hpp"
#include "Gameplay/Entity.hpp"
#include "Common/Constants.hpp"
#include <cstdio>
#include <algorithm>
#include <limits>

namespace Engine {

LevelEditor::LevelEditor(Gameplay::EntityManager &entityManager, Camera &camera)
    /// Constructs the LevelEditor with references to the entity manager and camera.
    /// @param entityManager  The entity manager to edit.
    /// @param camera          The camera (editor scrolls it with arrows).
    : m_entityManager(entityManager), m_camera(camera) {}

void LevelEditor::update(float deltaTime, const Common::InputState &input) {
    /// Per-frame editor logic: mouse click selects entity at world coordinates,
    /// then delegates to object manipulation, property editing, and level
    /// config editing. Hitting Enter sets the pending submission flag.
    /// @param deltaTime  Time since last frame in seconds.
    /// @param input      Current frame input snapshot.
    if (!m_active) return;

    if (input.mouseLeftDown && !m_prevMouseDown) {
        selectEntityAt(input.mouseX + static_cast<int>(m_camera.getOffsetX()),
                       input.mouseY);
    }
    m_prevMouseDown = input.mouseLeftDown;

    handleObjectManipulation(deltaTime, input);
    handlePropertyEditing(input);
    handleLevelConfigEditing(input);

    bool justEnter = input.enter && !m_prevPressed[PK_ENTER];
    m_prevPressed[PK_ENTER] = input.enter;
    if (justEnter) {
        Suggestion s;
        s.levelName = "untitled";
        m_pendingSubmission = s;
    }
}

void LevelEditor::render(std::vector<Common::RenderCommand> &commands,
                          std::vector<Common::TextCommand> &textCommands) {
    /// Draws a red selection highlight around the currently selected entity
    /// (2px larger on each side) and builds the editor HUD text overlay.
    /// @param commands       Render command list to append to.
    /// @param textCommands   Text command list for HUD text.
    auto selected = m_selectedEntity.lock();
    if (selected) {
        commands.push_back({
            selected->transform.x - 2, selected->transform.y - 2,
            selected->transform.width + 4, selected->transform.height + 4,
            Common::TextureID::TEX_NONE, 1.0f, 255, 0, 0, 255
        });
    }

    buildHUDText(textCommands);
}

void LevelEditor::handleObjectManipulation(float deltaTime, const Common::InputState &input) {
    /// Processes C/V/B creation hotkeys, Backspace deletion, WASD movement,
    /// Shift+WASD resize, and arrow-key camera scrolling.
    /// @param deltaTime  Time step for speed calculation.
    /// @param input      Current frame input snapshot.
    bool justC = input.c && !m_prevPressed[PK_C];
    bool justV = input.v && !m_prevPressed[PK_V];
    bool justB = input.b && !m_prevPressed[PK_B];

    m_prevPressed[PK_C] = input.c;
    m_prevPressed[PK_V] = input.v;
    m_prevPressed[PK_B] = input.b;

    if (justC) createDefaultObject();
    if (justV) createPlayer();
    if (justB) createBackgroundLayer();

    auto selected = m_selectedEntity.lock();
    if (!selected) return;

    bool justBackspace = input.backspace && !m_prevPressed[PK_BACKSPACE];
    m_prevPressed[PK_BACKSPACE] = input.backspace;
    if (justBackspace) { deleteSelectedObject(); return; }

    float moveSpeed = Common::LEVEL_EDITOR_MOVE_SPEED * deltaTime;
    float scaleSpeed = Common::LEVEL_EDITOR_SCALE_SPEED * deltaTime;

    if (input.shift) {
        if (input.left)  { selected->transform.width  = std::max(1.0f, selected->transform.width  - scaleSpeed); if (selected->collider) selected->collider->width  = selected->transform.width; }
        if (input.right) { selected->transform.width  += scaleSpeed; if (selected->collider) selected->collider->width  = selected->transform.width; }
        if (input.up)    { selected->transform.height = std::max(1.0f, selected->transform.height - scaleSpeed); if (selected->collider) selected->collider->height = selected->transform.height; }
        if (input.down)  { selected->transform.height += scaleSpeed; if (selected->collider) selected->collider->height = selected->transform.height; }
    } else {
        if (input.left)  selected->transform.x -= moveSpeed;
        if (input.right) selected->transform.x += moveSpeed;
        if (input.up)    selected->transform.y -= moveSpeed;
        if (input.down)  selected->transform.y += moveSpeed;
    }

    if (input.arrowLeft)  m_camera.setOffsetX(m_camera.getOffsetX() - moveSpeed * 2);
    if (input.arrowRight) m_camera.setOffsetX(m_camera.getOffsetX() + moveSpeed * 2);
}

void LevelEditor::handlePropertyEditing(const Common::InputState &input) {
    /// Processes T/Y/U/R hotkeys for cycling texture, collider type, exit
    /// properties, and z-index, plus number-key texture presets (1-9).
    /// @param input  Current frame input snapshot.
    auto selected = m_selectedEntity.lock();
    if (!selected) return;

    bool justT = input.t && !m_prevPressed[PK_T];
    bool justY = input.y && !m_prevPressed[PK_Y];
    bool justU = input.u && !m_prevPressed[PK_U];
    bool justR = input.r && !m_prevPressed[PK_R];

    m_prevPressed[PK_T] = input.t;
    m_prevPressed[PK_Y] = input.y;
    m_prevPressed[PK_U] = input.u;
    m_prevPressed[PK_R] = input.r;

    if (justT) cycleTexture();
    if (justY) cycleColliderType();
    if (justU) toggleExit();
    if (justR) cycleZIndex();

    bool justNum1 = input.num1 && !m_prevPressed[PK_NUM1];
    bool justNum2 = input.num2 && !m_prevPressed[PK_NUM2];
    bool justNum3 = input.num3 && !m_prevPressed[PK_NUM3];
    bool justNum4 = input.num4 && !m_prevPressed[PK_NUM4];
    bool justNum5 = input.num5 && !m_prevPressed[PK_NUM5];
    bool justNum6 = input.num6 && !m_prevPressed[PK_NUM6];
    bool justNum7 = input.num7 && !m_prevPressed[PK_NUM7];
    bool justNum8 = input.num8 && !m_prevPressed[PK_NUM8];
    bool justNum9 = input.num9 && !m_prevPressed[PK_NUM9];

    m_prevPressed[PK_NUM1] = input.num1;
    m_prevPressed[PK_NUM2] = input.num2;
    m_prevPressed[PK_NUM3] = input.num3;
    m_prevPressed[PK_NUM4] = input.num4;
    m_prevPressed[PK_NUM5] = input.num5;
    m_prevPressed[PK_NUM6] = input.num6;
    m_prevPressed[PK_NUM7] = input.num7;
    m_prevPressed[PK_NUM8] = input.num8;
    m_prevPressed[PK_NUM9] = input.num9;

    if (justNum1) setTextureDirect(Common::TextureID::TEX_PLATFORM);
    if (justNum2) setTextureDirect(Common::TextureID::TEX_FLOOR);
    if (justNum3) setTextureDirect(Common::TextureID::TEX_HAZARD_LAVA);
    if (justNum4) setTextureDirect(Common::TextureID::TEX_HAZARD_SPIKE);
    if (justNum5) setTextureDirect(Common::TextureID::TEX_BACKGROUND_FAR);
    if (justNum6) setTextureDirect(Common::TextureID::TEX_BACKGROUND_MID);
    if (justNum7) setTextureDirect(Common::TextureID::TEX_BACKGROUND_NEAR);
    if (justNum8) setTextureDirect(Common::TextureID::TEX_PLAYER);
    if (justNum9) setTextureDirect(Common::TextureID::TEX_ENEMY);
}

void LevelEditor::handleLevelConfigEditing(const Common::InputState &input) {
    /// Processes [ and ] for decreasing/increasing level width by 256px,
    /// and L (with optional Shift) for toggling left/right wall clamping.
    /// @param input  Current frame input snapshot.
    bool justLeftBracket = input.leftBracket && !m_prevPressed[PK_LEFT_BRACKET];
    m_prevPressed[PK_LEFT_BRACKET] = input.leftBracket;
    if (justLeftBracket) {
        float newWidth = m_entityManager.levelConfig.levelWidth - 256.0f;
        if (newWidth >= Common::VIEWPORT_WIDTH) {
            m_entityManager.levelConfig.levelWidth = newWidth;
            m_camera.setBounds(0.0f, newWidth - Common::VIEWPORT_WIDTH);
        }
    }
    bool justRightBracket = input.rightBracket && !m_prevPressed[PK_RIGHT_BRACKET];
    m_prevPressed[PK_RIGHT_BRACKET] = input.rightBracket;
    if (justRightBracket) {
        float newWidth = m_entityManager.levelConfig.levelWidth + 256.0f;
        m_entityManager.levelConfig.levelWidth = newWidth;
        m_camera.setBounds(0.0f, newWidth - Common::VIEWPORT_WIDTH);
        if (m_camera.getOffsetX() > newWidth - Common::VIEWPORT_WIDTH)
            m_camera.setOffsetX(newWidth - Common::VIEWPORT_WIDTH);
    }
    bool justL = input.l && !m_prevPressed[PK_L];
    m_prevPressed[PK_L] = input.l;
    if (justL) {
        if (input.shift)
            m_entityManager.levelConfig.isRightWallClamped = !m_entityManager.levelConfig.isRightWallClamped;
        else
            m_entityManager.levelConfig.isLeftWallClamped = !m_entityManager.levelConfig.isLeftWallClamped;
    }
}

void LevelEditor::selectEntityAt(float worldX, float worldY) {
    /// Picks the topmost (highest zIndex) non-destroyed entity at the given
    /// world coordinates. Resets selection if nothing is found.
    /// Prints the selection info to stdout.
    /// @param worldX, worldY  World-space coordinates from mouse click.
    m_selectedEntity.reset();
    std::shared_ptr<Gameplay::Entity> best;
    int bestZ = std::numeric_limits<int>::min();

    for (const auto &entity : m_entityManager.getEntities()) {
        if (entity->isDestroyed) continue;
        const auto &t = entity->transform;
        if (worldX >= t.x && worldX <= t.x + t.width &&
            worldY >= t.y && worldY <= t.y + t.height) {
            if (t.zIndex > bestZ) {
                bestZ = t.zIndex;
                best = entity;
            }
        }
    }

    if (best) {
        m_selectedEntity = best;
        printf("Selected: %s at (%.0f, %.0f)  z=%d (%s)\n",
               entityTypeName(best).c_str(), best->transform.x, best->transform.y,
               best->transform.zIndex, layerName(best->transform.zIndex).c_str());
    }
}

void LevelEditor::createDefaultObject() {
    /// Creates a new StaticObject with default dimensions, TEX_PLATFORM,
    /// and a solid collider. Adds it to the entity manager and selects it.
    auto obj = std::make_shared<Gameplay::StaticObject>();
    obj->transform = {Common::DEFAULT_OBJECT_X, Common::DEFAULT_OBJECT_Y,
                      Common::DEFAULT_OBJECT_WIDTH, Common::DEFAULT_OBJECT_HEIGHT,
                      Common::LAYER_WORLD};
    obj->sprite = {Common::TextureID::TEX_PLATFORM};
    obj->collider = {Common::DEFAULT_OBJECT_WIDTH, Common::DEFAULT_OBJECT_HEIGHT,
                     0, 0, false, true};
    m_entityManager.addEntity(obj);
    m_selectedEntity = obj;
    printf("Created StaticObject (TEX_PLATFORM, solid)\n");
}

void LevelEditor::createPlayer() {
    /// Creates a new Player spawn point with default components and selects it.
    /// Positioned at DEFAULT_OBJECT_X/Y with PLAYER_WIDTH/HEIGHT.
    auto player = std::make_shared<Gameplay::Player>();
    player->transform = {Common::DEFAULT_OBJECT_X, Common::DEFAULT_OBJECT_Y,
                         Common::PLAYER_WIDTH, Common::PLAYER_HEIGHT,
                         Common::LAYER_ENTITIES};
    player->sprite = {Common::TextureID::TEX_PLAYER};
    player->playerControl = {Common::PLAYER_SPEED};
    player->physics = Gameplay::Physics();
    player->physics->maxSpeed = Common::PLAYER_SPEED;
    player->collider = {Common::PLAYER_WIDTH, Common::PLAYER_HEIGHT,
                        0, 0, false, true};
    m_entityManager.addEntity(player);
    m_selectedEntity = player;
    printf("Created Player spawn\n");
}

void LevelEditor::createBackgroundLayer() {
    /// Creates a full-screen BackgroundLayer with FAR texture and parallax
    /// factor 0.1. Adds it to the entity manager and selects it.
    auto bg = std::make_shared<Gameplay::BackgroundLayer>();
    bg->transform = {0.0f, 0.0f,
                     static_cast<float>(Common::SCREEN_WIDTH),
                     static_cast<float>(Common::SCREEN_HEIGHT),
                     Common::LAYER_BACKGROUND_FAR};
    bg->sprite = {Common::TextureID::TEX_BACKGROUND_FAR};
    bg->parallax = {0.1f};
    m_entityManager.addEntity(bg);
    m_selectedEntity = bg;
    printf("Created BackgroundLayer (TEX_BACKGROUND_FAR, parallax=0.1)\n");
}

void LevelEditor::deleteSelectedObject() {
    /// Marks the currently selected entity as destroyed (garbage-collected
    /// on next EntityManager::update) and clears the selection.
    auto selected = m_selectedEntity.lock();
    if (!selected) return;
    selected->isDestroyed = true;
    m_selectedEntity.reset();
    printf("Deleted entity\n");
}

void LevelEditor::cycleTexture() {
    /// Cycles the selected entity's texture through all TextureID values.
    auto selected = m_selectedEntity.lock();
    if (!selected || !selected->sprite) return;

    int current = static_cast<int>(selected->sprite->textureID);
    int maxTex = static_cast<int>(Common::TextureID::TEX_COUNT);
    int next = (current + 1) % maxTex;

    selected->sprite->textureID = static_cast<Common::TextureID>(next);
    printf("Texture: %s\n", textureName(next).c_str());
}

void LevelEditor::cycleColliderType() {
    /// Cycles the selected entity's collider through: none → solid → trigger
    /// → hazard → none. Creates a solid collider if none exists.
    auto selected = m_selectedEntity.lock();
    if (!selected) return;

    if (!selected->collider) {
        selected->collider = {selected->transform.width, selected->transform.height,
                              0, 0, false, true};
        printf("Collider: solid\n");
        return;
    }

    if (selected->collider->isSolid) {
        selected->collider->isSolid = false;
        selected->collider->isTrigger = true;
        selected->collider->isHazard = false;
        printf("Collider: trigger\n");
    } else if (selected->collider->isTrigger && !selected->collider->isHazard) {
        selected->collider->isSolid = false;
        selected->collider->isTrigger = true;
        selected->collider->isHazard = true;
        printf("Collider: hazard\n");
    } else if (selected->collider->isHazard) {
        selected->collider.reset();
        printf("Collider: none\n");
    } else {
        selected->collider = {selected->transform.width, selected->transform.height,
                              0, 0, false, true};
        printf("Collider: solid\n");
    }
}

void LevelEditor::toggleExit() {
    /// Toggles exit properties on the selected StaticObject through states:
    /// east only → east+west → west only → removed → east only.
    /// Ensures the collider becomes a trigger when any exit is active.
    auto selected = m_selectedEntity.lock();
    if (!selected) return;

    /// Lambda: ensures the selected entity has a trigger collider.
    auto ensureTrigger = [&]() {
        if (!selected->collider)
            selected->collider = {selected->transform.width, selected->transform.height,
                                  0, 0, true, false};
        else {
            selected->collider->isSolid = false;
            selected->collider->isTrigger = true;
            selected->collider->isHazard = false;
        }
    };

    if (selected->exitEast && !selected->exitWest) {
        selected->exitWest = {Gameplay::CardinalDirection::West, "untitled", Common::DEFAULT_TRANSITION_DURATION};
        ensureTrigger();
        printf("Exit: east + west\n");
    } else if (selected->exitEast && selected->exitWest) {
        selected->exitEast.reset();
        ensureTrigger();
        printf("Exit: west only\n");
    } else if (selected->exitWest && !selected->exitEast) {
        selected->exitWest.reset();
        if (!selected->exitEast)
            selected->collider.reset();
        printf("Exit removed\n");
    } else {
        selected->exitEast = {Gameplay::CardinalDirection::East, "untitled", Common::DEFAULT_TRANSITION_DURATION};
        ensureTrigger();
        printf("Exit: east only\n");
    }
}

void LevelEditor::cycleZIndex() {
    /// Cycles the selected entity's z-index through the predefined layer
    /// presets (BG_FAR → BG_MID → BG_NEAR → WORLD → ENTITIES → BG_FAR).
    /// Ensures BackgroundLayer entities always have a parallax component.
    auto selected = m_selectedEntity.lock();
    if (!selected) return;

    static const int layers[] = {
        Common::LAYER_BACKGROUND_FAR,
        Common::LAYER_BACKGROUND_MID,
        Common::LAYER_BACKGROUND_NEAR,
        Common::LAYER_WORLD,
        Common::LAYER_ENTITIES
    };
    static constexpr int numLayers = sizeof(layers) / sizeof(layers[0]);

    int current = selected->transform.zIndex;
    int nextIdx = 0;
    for (int i = 0; i < numLayers; i++) {
        if (layers[i] == current) {
            nextIdx = (i + 1) % numLayers;
            break;
        }
    }
    selected->transform.zIndex = layers[nextIdx];

    if (auto bg = std::dynamic_pointer_cast<Gameplay::BackgroundLayer>(selected)) {
        if (!bg->parallax) bg->parallax = {0.1f};
    }

    printf("zIndex: %d (%s) — controls draw order: lower = behind, higher = in front\n",
        selected->transform.zIndex, layerName(selected->transform.zIndex).c_str());
}

void LevelEditor::setTextureDirect(Common::TextureID textureID) {
    /// Directly assigns a texture to the selected entity.
    /// Called by number-key texture presets (1-9).
    /// @param textureID  The TextureID to assign.
    auto selected = m_selectedEntity.lock();
    if (!selected) return;
    selected->sprite = {textureID};
    printf("Texture: %s\n", textureName(static_cast<int>(textureID)).c_str());
}

void LevelEditor::buildHUDText(std::vector<Common::TextCommand> &textCommands) {
    /// Composes the editor overlay text: hotkey legend at the bottom,
    /// current level dimensions and wall-clamp state, and detailed info about
    /// the selected entity (type, position, size, texture, collider, z-index,
    /// exit properties, parallax factor).
    /// @param textCommands  Text command list to extend.
    auto selected = m_selectedEntity.lock();
    char buf[256];
    float rm = 10.0f;

    float y = 705.0f;
    textCommands.push_back({"C:place wall  V:player  B:bg  T:tex  Y:col  U:exit  R:z", rm, y, 150, 255, 150, 255, false, "ui", true});
    y -= 20.0f;
    textCommands.push_back({"WASD:move  Shift+WASD:resize  Arrows:camera  L:clampL  Shift+L:clampR  [:]-width", rm, y, 150, 255, 150, 255, false, "ui", true});

    y -= 25.0f;
    std::snprintf(buf, sizeof(buf),
        "Level: %.0fpx  L-clamp:%s  R-clamp:%s",
        m_entityManager.levelConfig.levelWidth,
        m_entityManager.levelConfig.isLeftWallClamped ? "ON" : "OFF",
        m_entityManager.levelConfig.isRightWallClamped ? "ON" : "OFF");
    textCommands.push_back({buf, rm, y, 200, 200, 100, 255, false, "ui", true});

    y -= 20.0f;
    if (selected) {
            std::snprintf(buf, sizeof(buf),
                "[%s] pos:(%.0f, %.0f) %.0fx%.0f  %s  %s  z=%d (%s)",
                entityTypeName(selected).c_str(),
                selected->transform.x, selected->transform.y,
                selected->transform.width, selected->transform.height,
                textureName(static_cast<int>(selected->sprite ? selected->sprite->textureID : Common::TextureID::TEX_NONE)).c_str(),
                colliderTypeName(selected).c_str(),
                selected->transform.zIndex,
                layerName(selected->transform.zIndex).c_str());
        textCommands.push_back({buf, rm, y, 255, 255, 255, 255, false, "ui", true});
        y -= 20.0f;

        if (selected->exitEast) {
            std::snprintf(buf, sizeof(buf),
                "Exit EAST: %s (%.1fs)",
                selected->exitEast->nextLevelPath.c_str(),
                selected->exitEast->transitionDuration);
            textCommands.push_back({buf, rm, y, 100, 255, 100, 255, false, "ui", true});
            y -= 20.0f;
        }
        if (selected->exitWest) {
            std::snprintf(buf, sizeof(buf),
                "Exit WEST: %s (%.1fs)",
                selected->exitWest->nextLevelPath.c_str(),
                selected->exitWest->transitionDuration);
            textCommands.push_back({buf, rm, y, 100, 255, 100, 255, false, "ui", true});
            y -= 20.0f;
        }

        if (auto bg = std::dynamic_pointer_cast<Gameplay::BackgroundLayer>(selected)) {
            if (bg->parallax) {
                std::snprintf(buf, sizeof(buf), "Parallax: %.2f", bg->parallax->factor);
                textCommands.push_back({buf, rm, y, 100, 255, 100, 255, false, "ui", true});
            }
        }
    } else {
        textCommands.push_back({"No entity selected", rm, y, 200, 200, 200, 255, false, "ui", true});
    }
}

std::string LevelEditor::textureName(int id) const {
    /// Returns a human-readable name for a given TextureID integer.
    /// @param id  The integer value of a TextureID.
    /// @return  Short string like "PLAYER", "PLATFORM", "BG_FAR", etc.
    switch (static_cast<Common::TextureID>(id)) {
        case Common::TextureID::TEX_PLAYER:         return "PLAYER";
        case Common::TextureID::TEX_PLATFORM:           return "PLATFORM";
        case Common::TextureID::TEX_FLOOR:          return "FLOOR";
        case Common::TextureID::TEX_ENEMY:          return "ENEMY";
        case Common::TextureID::TEX_BACKGROUND_FAR:  return "BG_FAR";
        case Common::TextureID::TEX_BACKGROUND_MID:  return "BG_MID";
        case Common::TextureID::TEX_BACKGROUND_NEAR: return "BG_NEAR";
        case Common::TextureID::TEX_HAZARD_LAVA:    return "LAVA";
        case Common::TextureID::TEX_HAZARD_SPIKE:   return "SPIKE";
        case Common::TextureID::TEX_COLLECTIBLE:    return "COLLECTIBLE";
        case Common::TextureID::TEX_SKULL:          return "SKULL";
        case Common::TextureID::TEX_MENU_BG:        return "MENU_BG";
        default: return "NONE";
    }
}

std::string LevelEditor::colliderTypeName(const std::shared_ptr<Gameplay::Entity> &entity) const {
    /// Returns a short string describing the collider type of an entity.
    /// @param entity  The entity to inspect.
    /// @return "none", "solid", "trigger", or "hazard".
    if (!entity || !entity->collider) return "none";
    if (entity->collider->isHazard) return "hazard";
    if (entity->collider->isSolid) return "solid";
    if (entity->collider->isTrigger) return "trigger";
    return "none";
}

std::string LevelEditor::entityTypeName(const std::shared_ptr<Gameplay::Entity> &entity) const {
    /// Returns the concrete type name of an entity via dynamic_cast.
    /// @param entity  The entity to classify.
    /// @return "Player", "BackgroundLayer", or "StaticObject".
    if (!entity) return "None";
    if (std::dynamic_pointer_cast<Gameplay::Player>(entity)) return "Player";
    if (std::dynamic_pointer_cast<Gameplay::BackgroundLayer>(entity)) return "BackgroundLayer";
    return "StaticObject";
}

std::string LevelEditor::layerName(int z) const {
    /// Returns a human-readable name for a z-index value.
    /// @param z  The z-index value.
    /// @return Short name like "BG_FAR", "WORLD", "ENTITIES", or "CUSTOM".
    if (z == Common::LAYER_BACKGROUND_FAR)  return "BG_FAR";
    if (z == Common::LAYER_BACKGROUND_MID)  return "BG_MID";
    if (z == Common::LAYER_BACKGROUND_NEAR) return "BG_NEAR";
    if (z == Common::LAYER_WORLD)           return "WORLD";
    if (z == Common::LAYER_ENTITIES)        return "ENTITIES";
    if (z == Common::LAYER_EFFECTS)         return "EFFECTS";
    if (z == Common::LAYER_UI)              return "UI";
    if (z == Common::LAYER_OVERLAY)         return "OVERLAY";
    return "CUSTOM";
}

} // namespace Engine
