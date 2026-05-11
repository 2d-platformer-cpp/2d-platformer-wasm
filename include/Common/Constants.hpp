/// @file Constants.hpp
/// All game-wide compile-time constants: screen dimensions, physics values,
/// texture IDs, render layer indices, UI sizes, file paths, and more.

#pragma once

namespace Common {
// ============================================================================
// Window Settings
// ============================================================================
/// The logical (and default window) width in pixels.
inline constexpr int SCREEN_WIDTH = 1280;
/// The logical (and default window) height in pixels.
inline constexpr int SCREEN_HEIGHT = 720;
/// Smallest allowed window width; prevents unusably small windows.
inline constexpr int MINIMUM_SCREEN_WIDTH = 854;
/// Smallest allowed window height.
inline constexpr int MINIMUM_SCREEN_HEIGHT = 480;
/// Displayed in the window title bar.
inline constexpr const char *WINDOW_TITLE = "2D Platformer";

// ============================================================================
// Camera Settings
// ============================================================================
/// How many logical pixels are visible horizontally at once; equals
/// SCREEN_WIDTH.
inline constexpr int VIEWPORT_WIDTH = SCREEN_WIDTH;
/// Leftmost X the camera can scroll to.
inline constexpr float DEFAULT_CAMERA_MIN_X = 0.0f;
/// Rightmost X (viewport width is subtracted from level width in code).
inline constexpr float DEFAULT_CAMERA_MAX_X = 1280.0f;

// ============================================================================
// World Settings
// ============================================================================
/// Default level width when no level file is loaded or width is unspecified.
inline constexpr float DEFAULT_LEVEL_WIDTH = 2560.0f;

// ============================================================================
// Player Settings
// ============================================================================
/// Player entity hitbox / render width in logical pixels.
inline constexpr float PLAYER_WIDTH = 48.0f;
/// Player entity hitbox / render height in logical pixels.
inline constexpr float PLAYER_HEIGHT = 60.0f;
/// Base horizontal speed scalar set in PlayerControl when loading a level.
inline constexpr float PLAYER_SPEED = 1000.0f;
/// Default X position used when spawning the player (near left edge).
inline constexpr float PLAYER_SPAWN_X = 50.0f;
/// Vertical offset used when the level file does not specify a spawn Y.
inline constexpr float PLAYER_SPAWN_Y_OFFSET = 100.0f;

// ============================================================================
// Physics
// ============================================================================
/// Target frames per second; used to derive TIME_STEP.
inline constexpr float TARGET_FPS = 60.0f;
/// Duration of one simulation step in seconds (~16.67 ms).
inline constexpr float TIME_STEP = 1.0f / TARGET_FPS;
/// Downward acceleration applied every frame in px/s².
inline constexpr float GRAVITY = 1500.0f;
/// Maximum falling speed cap in px/s.
inline constexpr float TERMINAL_VELOCITY = 8000.0f;
/// Horizontal acceleration when a movement key is held, in px/s².
inline constexpr float PLAYER_ACCELERATION = 5000.0f;
/// Horizontal deceleration when no movement key is held, in px/s².
inline constexpr float FRICTION = 10000.0f;
/// Instant vertical velocity applied on jump, in px/s (negative = upward).
inline constexpr float JUMP_FORCE = -650.0f;
/// Maximum horizontal speed cap in px/s.
inline constexpr float PLAYER_MAX_SPEED = 500.0f;
/// Brief window after leaving a ledge during which jump is still allowed.
inline constexpr float GROUND_DEBOUNCE_TIME = 0.05f;
/// Alias for GROUND_DEBOUNCE_TIME; used for coyote-time logic.
inline constexpr float COYOTE_TIME = 0.05f;

// ============================================================================
// Texture IDs
// ============================================================================
/// Enum identifying every texture the renderer can load and draw.
enum class TextureID : int {
  TEX_PLAYER = 0,   ///< Player entity cyan fallback (not loaded as texture)
  TEX_PLATFORM = 1, ///< Platform solid surface (not loaded, green fallback)
  TEX_FLOOR = 2,    ///< Floor surface (not loaded, green fallback)
  TEX_ENEMY = 3,    ///< Enemy / hazard entity red fallback
  TEX_BACKGROUND_FAR = 4, ///< Far background layer -> Background_Far.png
  TEX_BACKGROUND_MID = 5, ///< Mid background layer -> Background_Mid_Start.png
  TEX_BACKGROUND_NEAR =
      6,                ///< Near background layer -> Background_Near_Start.png
  TEX_HAZARD_LAVA = 7,  ///< Lava hazard orange fallback
  TEX_HAZARD_SPIKE = 8, ///< Spike hazard grey fallback
  TEX_COLLECTIBLE = 9,  ///< Collectible item yellow fallback
  TEX_SKULL = 10,       ///< Skull icon on death screen -> skull.png
  TEX_MENU_BG = 11,     ///< Menu background -> menu_bg.png
  TEX_COUNT,            ///< Number of texture slots (not a valid texture)
  TEX_NONE = -1         ///< Sentinel meaning "no texture" / solid color
};

// ============================================================================
// Render Layers (Z-Index Ranges)
// ============================================================================
/// Far background: rendered first (behind everything).
inline constexpr int LAYER_BACKGROUND_FAR = 0;
/// Mid background.
inline constexpr int LAYER_BACKGROUND_MID = 4;
/// Near background.
inline constexpr int LAYER_BACKGROUND_NEAR = 7;
/// World geometry: platforms, floors, hazards.
inline constexpr int LAYER_WORLD = 10;
/// Dynamic entities: player, enemies.
inline constexpr int LAYER_ENTITIES = 50;
/// Visual effects: particles, screen shakes (reserved).
inline constexpr int LAYER_EFFECTS = 100;
/// UI elements: buttons, panels (non-scrolling).
inline constexpr int LAYER_UI = 150;
/// Full-screen overlays: transitions, death fade.
inline constexpr int LAYER_OVERLAY = 200;

// ============================================================================
// Tile / Grid
// ============================================================================
/// Default tile size in pixels (used for grid snapping, reserved).
inline constexpr int TILE_SIZE = 32;

// ============================================================================
// UI
// ============================================================================
/// Standard menu button width in logical pixels.
inline constexpr float MENU_BUTTON_WIDTH = 250.0f;
/// Standard menu button height in logical pixels.
inline constexpr float MENU_BUTTON_HEIGHT = 80.0f;
/// Pause / stats panel width.
inline constexpr float PAUSE_PANEL_WIDTH = 380.0f;
/// Pause / stats panel height.
inline constexpr float PAUSE_PANEL_HEIGHT = 400.0f;

/// Progress bar track width (top-right minimap bar).
inline constexpr float PROGRESS_BAR_WIDTH = 200.0f;
/// Progress bar track height.
inline constexpr float PROGRESS_BAR_HEIGHT = 10.0f;
/// Distance from screen edge for the progress bar.
inline constexpr float PROGRESS_BAR_MARGIN = 20.0f;

/// HUD element padding from the screen edge.
inline constexpr float HUD_MARGIN = 12.0f;
/// Vertical spacing between HUD text lines.
inline constexpr float HUD_LINE_HEIGHT = 30.0f;

// ============================================================================
// Level Editor
// ============================================================================
/// Speed at which selected entities move with WASD in the editor.
inline constexpr float LEVEL_EDITOR_MOVE_SPEED = 400.0f;
/// Speed at which selected entities resize with Shift+WASD.
inline constexpr float LEVEL_EDITOR_SCALE_SPEED = 200.0f;
/// Default X for new objects created in the editor.
inline constexpr float DEFAULT_OBJECT_X = 400.0f;
/// Default Y for new objects.
inline constexpr float DEFAULT_OBJECT_Y = 300.0f;
/// Default width for new StaticObject entities.
inline constexpr float DEFAULT_OBJECT_WIDTH = 200.0f;
/// Default height for new StaticObject entities.
inline constexpr float DEFAULT_OBJECT_HEIGHT = 100.0f;

// ============================================================================
// Level Transition
// ============================================================================
/// Duration of the fade-to-black when crossing a level exit.
inline constexpr float DEFAULT_TRANSITION_DURATION = 1.0f;
/// Longer fade for death respawn (skull + YOU DIED screen).
inline constexpr float DEATH_FADE_DURATION = 1.5f;
/// Alpha value at which the transition is fully opaque.
inline constexpr float FADE_MAX_ALPHA = 255.0f;

// ============================================================================
// File Extensions
// ============================================================================
/// Extension for per-user encrypted stat files.
inline constexpr const char *USER_FILE_EXT = ".user";
/// Extension for level data files.
inline constexpr const char *LEVEL_FILE_EXT = ".level";
/// Path to the encrypted user database file.
inline constexpr const char *USERS_FILE_PATH = "assets/users/users.user";
/// Directory containing user stat files and the database.
inline constexpr const char *USERS_DIR_PATH = "assets/users";
/// Directory where level files are stored.
inline constexpr const char *LEVELS_DIR = "assets/levels/";

// ============================================================================
// Default Collider
// ============================================================================
/// Default width for newly created colliders / entities.
inline constexpr float DEFAULT_COLLIDER_WIDTH = 32.0f;
/// Default height for newly created colliders / entities.
inline constexpr float DEFAULT_COLLIDER_HEIGHT = 32.0f;

// ============================================================================
// Transition spawn offsets
// ============================================================================
/// Distance from left edge to place the player after entering from an east
/// exit.
inline constexpr float SPAWN_EAST_OFFSET = 50.0f;
/// Distance from right edge (levelWidth - playerWidth - offset) after entering
/// from a west exit.
inline constexpr float SPAWN_WEST_OFFSET = 50.0f;

// ============================================================================
// Fixed Timestep
// ============================================================================
/// Maximum accumulated frame time (caps spiral-of-death if frame takes too
/// long).
inline constexpr float MAX_FRAME_TIME = 0.25f;
/// How often the FPS counter in the window title is updated.
inline constexpr float FPS_UPDATE_INTERVAL = 0.25f;

} // namespace Common
