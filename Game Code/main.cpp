#include <iostream>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <stb/stb_image.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "shaderClass.h"
#include "VAO.h"
#include "VBO.h"
#include "EBO.h"
#include "Texture.h"
#include "Character.h"
#include "Player.h"
#include "Enemy.h"
#include "WaveManager.h"
#include "HUD.h"
#include "Audio.h"
#include "Menu.h"

#include "Constants.h"

// Controls the current screen/game mode
enum class GameState {
    MENU,
    SETTINGS,
    CREDITS,
    PLAYING,
    PAUSED,
    GAME_OVER,
    VICTORY
};

// Initialized as 1.0f and updated using monitor resolution
float ASPECT_RATIO = 1.0f;

// Character quad vertices (position + texture coordinates)
GLfloat vertices[] =
{
    -0.5f,  0.5f, 0.0f,   0.0f, 1.0f,
    -0.5f, -0.5f, 0.0f,   0.0f, 0.0f,
     0.5f, -0.5f, 0.0f,   1.0f, 0.0f,
     0.5f,  0.5f, 0.0f,   1.0f, 1.0f
};

// Quad indices
GLuint indices[] =
{
    0, 1, 2,
    0, 2, 3
};

int main()
{
    // ----------------------------
    // GLFW Initialization
    // ----------------------------

    glfwInit();

    // Configure OpenGL version and profile
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // Obtain primary monitor information
    GLFWmonitor* monitor = glfwGetPrimaryMonitor();
    const GLFWvidmode* mode = glfwGetVideoMode(monitor);

    // Calculate aspect ratio and projection matrix
    ASPECT_RATIO =
        (float)mode->width / (float)mode->height;

    glm::mat4 projection =
        glm::ortho(
            -ASPECT_RATIO,
            ASPECT_RATIO,
            -1.0f, 1.0f,
            -1.0f, 1.0f);

    // Create fullscreen game window
    GLFWwindow* window =
        glfwCreateWindow(
            mode->width,
            mode->height,
            "Game",
            monitor,
            NULL);

    // Abort if window creation fails
    if (window == NULL)
    {
        std::cout << "Failed to create GLFW window"
            << std::endl;

        glfwTerminate();
        return -1;
    }

    // Make window the current OpenGL context
    glfwMakeContextCurrent(window);

    // Enable vertical synchronization
    glfwSwapInterval(1);

    // ----------------------------
    // OpenGL Initialization
    // ----------------------------

    if (!gladLoadGLLoader(
        (GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD\n";
        return -1;
    }

    // Enable alpha blending for sprites
    glEnable(GL_BLEND);
    glBlendFunc(
        GL_SRC_ALPHA,
        GL_ONE_MINUS_SRC_ALPHA);

    // Set viewport to monitor resolution
    glViewport(
        0,
        0,
        mode->width,
        mode->height);

    // ----------------------------
    // Audio and HUD Setup
    // ----------------------------

    Audio audio;

    // Start menu music immediately
    audio.PlayMusic("Audio/menu.mp3");

    HUD hud;

    // ----------------------------
    // Character Rendering Setup
    // ----------------------------

    Shader shaderProgram(
        "Resources/Character Shaders/default.vert",
        "Resources/Character Shaders/default.frag");

    // Create character VAO
    VAO VAO1;
    VAO1.Bind();

    // Create vertex and index buffers
    VBO VBO1(vertices, sizeof(vertices));
    EBO EBO1(indices, sizeof(indices));

    // Link vertex attributes
    VAO1.LinkAttrib(
        VBO1,
        0,
        3,
        GL_FLOAT,
        5 * sizeof(float),
        (void*)0);

    VAO1.LinkAttrib(
        VBO1,
        1,
        2,
        GL_FLOAT,
        5 * sizeof(float),
        (void*)(3 * sizeof(float)));

    // Unbind buffers after setup
    VAO1.Unbind();
    VBO1.Unbind();
    EBO1.Unbind();

    // ----------------------------
    // Background Rendering Setup
    // ----------------------------

    GLfloat bgVertices[] =
    {
        -ASPECT_RATIO, -1.0f, 0.0f, 0.0f, 0.0f,
         ASPECT_RATIO, -1.0f, 0.0f, 1.0f, 0.0f,
         ASPECT_RATIO,  1.0f, 0.0f, 1.0f, 1.0f,
        -ASPECT_RATIO,  1.0f, 0.0f, 0.0f, 1.0f
    };

    GLuint bgIndices[] =
    {
        0, 1, 2,
        0, 2, 3
    };

    // Create background VAO/VBO/EBO
    VAO bgVAO;
    bgVAO.Bind();

    VBO bgVBO(bgVertices, sizeof(bgVertices));
    EBO bgEBO(bgIndices, sizeof(bgIndices));

    bgVAO.LinkAttrib(
        bgVBO,
        0,
        3,
        GL_FLOAT,
        5 * sizeof(float),
        (void*)0);

    bgVAO.LinkAttrib(
        bgVBO,
        1,
        2,
        GL_FLOAT,
        5 * sizeof(float),
        (void*)(3 * sizeof(float)));

    bgVAO.Unbind();
    bgVBO.Unbind();
    bgEBO.Unbind();

    // ----------------------------
    // Shader Uniform Locations
    // ----------------------------

    GLuint uniModel =
        glGetUniformLocation(
            shaderProgram.ID,
            "model");

    GLuint uniOffset =
        glGetUniformLocation(
            shaderProgram.ID,
            "uOffset");

    GLuint uniNumFrames =
        glGetUniformLocation(
            shaderProgram.ID,
            "numFrames");

    GLuint uniProjection =
        glGetUniformLocation(
            shaderProgram.ID,
            "projection");

    // Flip textures vertically during loading
    stbi_set_flip_vertically_on_load(true);

    int gameFrame = 0;

    // ----------------------------
    // Player Setup
    // ----------------------------

    Player player(
        glm::vec3(
            -0.8f,
            GROUND_LEVEL,
            0.0f));

    // Load all player animation spritesheets
    player.LoadTexture(AnimState::IDLE,
        "Resources/Textures/Player/Idle.png");

    player.LoadTexture(AnimState::WALK,
        "Resources/Textures/Player/Walk.png");

    player.LoadTexture(AnimState::JUMP,
        "Resources/Textures/Player/Jump.png");

    player.LoadTexture(AnimState::ATTACK,
        "Resources/Textures/Player/Attack.png");

    player.LoadTexture(AnimState::BLOCK,
        "Resources/Textures/Player/Block.png");

    player.LoadTexture(AnimState::HIT,
        "Resources/Textures/Player/Hit.png");

    player.LoadTexture(AnimState::KO,
        "Resources/Textures/Player/KO.png");

    // ----------------------------
    // HUD Setup
    // ----------------------------

    hud.LoadDigitTexture(
        "Resources/Textures/UI/digits.png");

    hud.LoadLabelTextures();

    // ----------------------------
    // Background Texture
    // ----------------------------

    Texture background(
        "Resources/Textures/UI/background.png",
        GL_TEXTURE_2D,
        GL_TEXTURE0,
        GL_RGBA,
        GL_UNSIGNED_BYTE);

    background.texUnit(
        shaderProgram,
        "sprite",
        0);

    // ----------------------------
    // Main Menu Setup
    // ----------------------------

    Menu menu;

    menu.LoadButtonTextures(
        "Resources/Textures/Menu/play.png",
        "Resources/Textures/Menu/settings.png",
        "Resources/Textures/Menu/credits.png",
        "Resources/Textures/Menu/quit.png");

    // ----------------------------
    // Wave System Setup
    // ----------------------------

    WaveManager waveManager(10);

    // Spawn first wave immediately
    waveManager.SpawnWave(0);

    // Used when transitioning between waves
    int framesSinceWaveComplete = 0;

    const int FRAMES_BETWEEN_WAVES = 120;

    // ----------------------------
    // Frame Timing
    // ----------------------------

    double lastTime = glfwGetTime();

    const double targetFrameTime =
        1.0 / FPS;

    int waveFramesRemaining =
        2 * 60 * FPS;

    // ----------------------------
    // Game State Variables
    // ----------------------------

    GameState gameState = GameState::MENU;

    int score = 0;

    GameState previousState =
        GameState::MENU;

    bool lastEscState = false;

    float masterVol = 1.0f;
    float musicVol = 1.0f;
    float sfxVol = 1.0f;

    int koTimer = 0;
    const int KO_DISPLAY_FRAMES = 60;

	// Main while loop
    while (!glfwWindowShouldClose(window)) 
    {
        double now = glfwGetTime();
        if (now - lastTime < targetFrameTime) continue;
        lastTime = now;

        gameFrame++;

        glClearColor(0.07f, 0.07f, 0.07f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        if (gameState == GameState::PLAYING)
        {
            if (!player.IsAlive())
            {
                koTimer++;
                if (koTimer >= KO_DISPLAY_FRAMES) 
                {
                    gameState = GameState::GAME_OVER;
                    audio.StopMusic();
                }
            }
            else if (waveFramesRemaining <= 0 && !waveManager.IsWaveComplete())
            {
                gameState = GameState::GAME_OVER;
                audio.StopMusic();
            }
            else if (waveManager.IsGameComplete())
            {
                gameState = GameState::VICTORY;
                audio.StopMusic();
            }
        }

        if (gameState == GameState::MENU)
        {
            // Draw background
            shaderProgram.Activate();
            glUniformMatrix4fv(uniProjection, 1, GL_FALSE, glm::value_ptr(projection));
            glm::mat4 bgModel = glm::mat4(1.0f);
            glUniformMatrix4fv(uniModel, 1, GL_FALSE, glm::value_ptr(bgModel));
            glUniform1f(uniOffset, 0.0f);
            glUniform1i(uniNumFrames, 1);
            background.Bind();
            bgVAO.Bind();
            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
            bgVAO.Unbind();

            // Bind character VAO before drawing menu buttons
            VAO1.Bind();
            menu.Draw(window, shaderProgram, uniModel, uniOffset, uniNumFrames, uniProjection, projection);
            VAO1.Unbind();

            // Handle button clicks
            MenuButton clicked = menu.Update(window);
            if (clicked == MenuButton::PLAY)
            {
                gameState = GameState::PLAYING;
                audio.PlayMusic("Audio/gameplay.mp3");
            }
            else if (clicked == MenuButton::SETTINGS)
            {
                previousState = GameState::MENU;
                gameState = GameState::SETTINGS;
            }
            else if (clicked == MenuButton::CREDITS)
                gameState = GameState::CREDITS;
            else if (clicked == MenuButton::QUIT)
                glfwSetWindowShouldClose(window, true);
        }

        if (gameState == GameState::PLAYING)
        {
            // Activating shader program
            shaderProgram.Activate();
            // Getting uniform for projection
            glUniformMatrix4fv(uniProjection, 1, GL_FALSE, glm::value_ptr(projection));

            glm::mat4 bgModel = glm::mat4(1.0f); // identity - no transform needed
            glUniformMatrix4fv(uniModel, 1, GL_FALSE, glm::value_ptr(bgModel));
            glUniform1f(uniOffset, 0.0f);
            glUniform1i(uniNumFrames, 1);
            background.Bind();
            bgVAO.Bind();
            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
            bgVAO.Unbind();

            VAO1.Bind();
            
            if (player.IsAlive())
                // Update player data
                player.Update(window, gameFrame, audio);
            else
                player.UpdateAnimation();
            // Draw player
            player.Draw(shaderProgram, uniModel, uniOffset, uniNumFrames);

            // Wave transitions
            if (waveManager.IsWaveComplete())
            {
                framesSinceWaveComplete++;
                if (framesSinceWaveComplete >= FRAMES_BETWEEN_WAVES &&
                    !waveManager.IsGameComplete())
                {
                    waveManager.SpawnWave(waveManager.GetCurrentWave() + 1);
                    framesSinceWaveComplete = 0;
                    waveFramesRemaining = 2 * 60 * FPS;
                }
            }
            else
            {
                framesSinceWaveComplete = 0;
            }

            if (player.IsAlive()) 
            {
                // Player position at the start of each wave is changed to position at the end of last wave
                waveManager.SetPlayerPos(player.position);
                // Update current wave info
                waveManager.Update(window, gameFrame, audio);
            }
            // Draw enemies based on current wave info
            waveManager.Draw(shaderProgram, uniModel, uniOffset, uniNumFrames);

            VAO1.Unbind();

            // Combat checks
            if (player.IsAlive())
            {
                for (auto enemy : waveManager.GetEnemies())
                {
                    if (!player.hasHit && player.GetHitbox().Intersects(enemy->GetHurtbox()))
                    {
                        enemy->TakeDamage(5);
                        player.hasHit = true;
                        if (!enemy->IsAlive())
                            audio.PlaySFX("Audio/ko.wav");
                        else
                            audio.PlaySFX("Audio/attack.wav");
                        if (!enemy->IsAlive())
                            score += 100;
                    }
                }
            }

            bool anyEnemyInFront = false;
            for (auto enemy : waveManager.GetEnemies())
            {
                // Condition checks if any enemy is facing the player, which is needed to check if block is valid or not
                if ((player.facingRight && enemy->position.x > player.position.x) ||
                    (!player.facingRight && enemy->position.x < player.position.x))
                {
                    anyEnemyInFront = true;
                    break;
                }
            }
            player.blockValid = anyEnemyInFront;

            if (player.IsAlive())
            {
                for (auto enemy : waveManager.GetEnemies())
                {
                    /* Player doesn't take damage when enemy is in HIT state, during landing (invincibilityTimer != 0), 
                       during jumping and when enemy hitbox doesn't overlap with player hurt box */
                    if (!enemy->hasHit
                        && player.GetInvincibilityTimer() == 0
                        && player.state != AnimState::JUMP
                        && enemy->GetHitbox().Intersects(player.GetHurtbox()))
                    {
                        AnimState stateBefore = player.state;
                        player.TakeDamage(4);
                        enemy->hasHit = true;

                        if (player.state == AnimState::HIT)
                            audio.PlaySFX("Audio/hit.wav");
                        else if (stateBefore == AnimState::BLOCK && player.state == AnimState::BLOCK)
                            audio.PlaySFX("Audio/block.wav");
                    }
                }
            }

            // Overlap prevention
            for (auto enemy : waveManager.GetEnemies())
            {
                AABB playerBox = player.GetHurtbox();
                AABB enemyBox = enemy->GetHurtbox();

                if (playerBox.Intersects(enemyBox))
                {
                    float overlapLeft = (playerBox.x + playerBox.width) - enemyBox.x;
                    float overlapRight = (enemyBox.x + enemyBox.width) - playerBox.x;

                    // If player is slightly to the left of the enemy, then player is set to the left of enemy when their boxes intersect
                    if (overlapLeft < overlapRight)
                        player.position.x -= overlapLeft;
                    // If player is slightly to the right of the enemy, then player is set to the right of enemy when their boxes intersect
                    else
                        player.position.x += overlapRight;

                    // Makes sure characters can't move out of window borders
                    player.position.x = glm::clamp(player.position.x, -ASPECT_RATIO + 0.1f, ASPECT_RATIO - 0.1f);
                    enemy->position.x = glm::clamp(enemy->position.x, -ASPECT_RATIO + 0.1f, ASPECT_RATIO - 0.1f);
                }
            }

            // Enemy-to-enemy overlap
            auto& enemyList = waveManager.GetEnemies();
            for (int i = 0; i < (int)enemyList.size(); i++)
            {
                for (int j = i + 1; j < (int)enemyList.size(); j++)
                {
                    AABB boxA = enemyList[i]->GetHurtbox();
                    AABB boxB = enemyList[j]->GetHurtbox();

                    if (boxA.Intersects(boxB))
                    {
                        float overlapLeft = (boxA.x + boxA.width) - boxB.x;
                        float overlapRight = (boxB.x + boxB.width) - boxA.x;

                        // Shifts player over to side of less overlap
                        if (overlapLeft < overlapRight)
                        {
                            enemyList[i]->position.x -= overlapLeft / 2.0f;
                            enemyList[j]->position.x += overlapLeft / 2.0f;
                        }
                        else
                        {
                            enemyList[i]->position.x += overlapRight / 2.0f;
                            enemyList[j]->position.x -= overlapRight / 2.0f;
                        }
                    }
                }
            }

            if (!waveManager.IsWaveComplete())
                waveFramesRemaining--;

            // Draws health bar of player and wave time remaining
            hud.Draw(player.health, 100, waveFramesRemaining,
                waveManager.GetCurrentWave() + 1,  // +1 so player sees 1-10 not 0-9
                waveManager.GetTotalWaves(),
                score);
        }

        else if (gameState == GameState::SETTINGS)
        {
            // Draw background
            shaderProgram.Activate();
            glUniformMatrix4fv(uniProjection, 1, GL_FALSE, glm::value_ptr(projection));
            glm::mat4 bgModel = glm::mat4(1.0f);
            glUniformMatrix4fv(uniModel, 1, GL_FALSE, glm::value_ptr(bgModel));
            glUniform1f(uniOffset, 0.0f);
            glUniform1i(uniNumFrames, 1);
            background.Bind();
            bgVAO.Bind();
            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
            bgVAO.Unbind();

            hud.DrawSettings(window, masterVol, musicVol, sfxVol);

            HUD::SettingsButton settingsClicked = hud.UpdateSettings(window, masterVol, musicVol, sfxVol);

            // Apply volume changes
            audio.SetMasterVolume(masterVol);
            audio.SetMusicVolume(musicVol);
            audio.SetSFXVolume(sfxVol);

            if (settingsClicked == HUD::SettingsButton::BACK)
                gameState = previousState;
        }

        else if (gameState == GameState::CREDITS)
        {
            // Draw background
            shaderProgram.Activate();
            glUniformMatrix4fv(uniProjection, 1, GL_FALSE, glm::value_ptr(projection));
            glm::mat4 bgModel = glm::mat4(1.0f);
            glUniformMatrix4fv(uniModel, 1, GL_FALSE, glm::value_ptr(bgModel));
            glUniform1f(uniOffset, 0.0f);
            glUniform1i(uniNumFrames, 1);
            background.Bind();
            bgVAO.Bind();
            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
            bgVAO.Unbind();

            hud.DrawCredits();

            // Click anywhere to return to menu
            static bool lastClickState = false;
            bool currentClick = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;
            bool freshClick = currentClick && !lastClickState;
            lastClickState = currentClick;

            if (freshClick)
                gameState = GameState::MENU;
        }

        else if (gameState == GameState::PAUSED)
        {
            // Keep drawing background
            shaderProgram.Activate();
            glUniformMatrix4fv(uniProjection, 1, GL_FALSE, glm::value_ptr(projection));
            glm::mat4 bgModel = glm::mat4(1.0f);
            glUniformMatrix4fv(uniModel, 1, GL_FALSE, glm::value_ptr(bgModel));
            glUniform1f(uniOffset, 0.0f);
            glUniform1i(uniNumFrames, 1);
            background.Bind();
            bgVAO.Bind();
            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
            bgVAO.Unbind();

            hud.DrawPaused(window);

            HUD::PauseButton pauseClicked = hud.UpdatePaused(window);
            if (pauseClicked == HUD::PauseButton::RESUME)
            {
                gameState = GameState::PLAYING;
                audio.StopPauseMusic();
                audio.ResumeMusic();
            }
            else if (pauseClicked == HUD::PauseButton::SETTINGS)
            {
                previousState = GameState::PAUSED;
                gameState = GameState::SETTINGS;
            }
            else if (pauseClicked == HUD::PauseButton::QUIT)
                glfwSetWindowShouldClose(window, true);
        }

        else if (gameState == GameState::GAME_OVER)
        {
            hud.DrawGameOver();

            // Click anywhere to return to menu
            static bool lastClickState = false;
            bool currentClick = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;
            bool freshClick = currentClick && !lastClickState;
            lastClickState = currentClick;

            if (freshClick)
            {
                gameState = GameState::MENU;
                audio.PlayMusic("Audio/menu.mp3");

                // Reset game state
                player = Player(glm::vec3(-0.8f, GROUND_LEVEL, 0.0f));
                player.LoadTexture(AnimState::IDLE, "Resources/Textures/Player/Idle.png");
                player.LoadTexture(AnimState::WALK, "Resources/Textures/Player/Walk.png");
                player.LoadTexture(AnimState::JUMP, "Resources/Textures/Player/Jump.png");
                player.LoadTexture(AnimState::ATTACK, "Resources/Textures/Player/Attack.png");
                player.LoadTexture(AnimState::BLOCK, "Resources/Textures/Player/Block.png");
                player.LoadTexture(AnimState::HIT, "Resources/Textures/Player/Hit.png");
                player.LoadTexture(AnimState::KO, "Resources/Textures/Player/KO.png");
                waveManager.SpawnWave(0);
                score = 0;
                waveFramesRemaining = 2 * 60 * FPS;
            }
        }

        else if (gameState == GameState::VICTORY)
        {
            hud.DrawVictory();

            static bool lastClickState = false;
            bool currentClick = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;
            bool freshClick = currentClick && !lastClickState;
            lastClickState = currentClick;

            if (freshClick)
            {
                gameState = GameState::MENU;
                audio.PlayMusic("Audio/menu.mp3");

                // Reset game state
                player = Player(glm::vec3(-0.8f, GROUND_LEVEL, 0.0f));
                player.LoadTexture(AnimState::IDLE, "Resources/Textures/Player/Idle.png");
                player.LoadTexture(AnimState::WALK, "Resources/Textures/Player/Walk.png");
                player.LoadTexture(AnimState::JUMP, "Resources/Textures/Player/Jump.png");
                player.LoadTexture(AnimState::ATTACK, "Resources/Textures/Player/Attack.png");
                player.LoadTexture(AnimState::BLOCK, "Resources/Textures/Player/Block.png");
                player.LoadTexture(AnimState::HIT, "Resources/Textures/Player/Hit.png");
                player.LoadTexture(AnimState::KO, "Resources/Textures/Player/KO.png");
                waveManager.SpawnWave(0);
                score = 0;
                waveFramesRemaining = 2 * 60 * FPS;
            }
        }

        // Swaps front buffer with created back buffer
        glfwSwapBuffers(window);
        // Takes care of window events like stretching and minimizing
        glfwPollEvents();

        // Pressing Esc exits the game or pauses it depending on current game state
        bool currentEscState = glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS;
        bool freshEsc = currentEscState && !lastEscState;
        lastEscState = currentEscState;

        if (freshEsc)        
        {
            if (gameState == GameState::PLAYING)
            {
                gameState = GameState::PAUSED;
                audio.PauseMusic();
                audio.PlayPauseMusic("Audio/pause.mp3");
            }
            else if (gameState == GameState::PAUSED)
            {
                gameState = GameState::PLAYING;
                audio.StopPauseMusic();
                audio.ResumeMusic();
            }
            else if (gameState == GameState::CREDITS)
                gameState = GameState::MENU;
            else if (gameState == GameState::GAME_OVER || gameState == GameState::VICTORY)
                gameState = GameState::MENU;
        }
    }

	// Delete all the objects created
	VAO1.Delete();
	VBO1.Delete();
	EBO1.Delete();
    bgVAO.Delete();
    bgVBO.Delete();
    bgEBO.Delete();
    background.Delete();
	shaderProgram.Delete();
	// Delete window before ending the program
	glfwDestroyWindow(window);
	// Terminate GLFW before ending the program
	glfwTerminate();
	return 0;
}