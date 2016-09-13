/*
 *  GameLogic.cpp
 *
 *  Created on: 2014/11/09
 *      Author: Dimitri Kourkoulis
 *     License: BSD 3-Clause License (see LICENSE file)
 */


#define MAX_Z -1.0f
#define MIN_Z -24.0f

#define GROUND_Y -1.0f
#define FULL_ROTATION 6.28f // More or less 360 degrees in radians

#define BUG_ROTATION_SPEED 0.12f
#define BUG_DIVE_TILT 0.8f
#define BUG_SPEED 0.08f
#define BUG_DIVE_DURATION 30
#define BUG_START_DIVE_DISTANCE 0.6f
#define BUG_FLIGHT_HEIGHT 1.4f

#define GOAT_ROTATION_SPEED 0.1f
#define GOAT_SPEED 0.05f

#include <memory>
#include <MathFunctions.hpp>
#include <Exception.hpp>
#include <MathFunctions.hpp>
#include <cmath>
#include "GameLogic.hpp"


using namespace small3d;

namespace AvoidTheBug3D {

  GameLogic::GameLogic() {
    initLogger();

    renderer = shared_ptr<Renderer>(new Renderer());

    renderer->init(854, 480, false, "Avoid the Bug 3D");

    crusoeText48 = shared_ptr<Text>(new Text(renderer));

    unique_ptr<Image> startScreenTexture(
      new Image("resources/images/startScreen.png"));
    renderer->generateTexture("startScreen", startScreenTexture->getData(), startScreenTexture->getWidth(), startScreenTexture->getHeight());

    unique_ptr<Image> groundTexture(
      new Image("resources/images/grass.png"));
    renderer->generateTexture("ground", groundTexture->getData(), groundTexture->getWidth(), groundTexture->getHeight());

    unique_ptr<Image> skyTexture(
      new Image("resources/images/sky.png"));
    renderer->generateTexture("sky", skyTexture->getData(), skyTexture->getWidth(), skyTexture->getHeight());

    goat = shared_ptr<SceneObject>(
      new SceneObject("goat",
        "resources/models/Goat/goatAnim",
        19, "resources/models/Goat/Goat.png",
        "resources/models/GoatBB/GoatBB.obj"));

    bug = shared_ptr<SceneObject>(
      new SceneObject("bug",
        "resources/models/Bug/bugAnim",
        9));
    bug->setColour(0.2f, 0.2f, 0.2f, 1.0f);
    bug->setFrameDelay(2);

    bugVerticalSpeed = ROUND_2_DECIMAL(BUG_FLIGHT_HEIGHT / BUG_DIVE_DURATION);

    tree = shared_ptr<SceneObject>(
      new SceneObject("tree",
        "resources/models/Tree/tree.obj",
        1, "resources/models/Tree/tree.png",
        "resources/models/TreeBB/TreeBB.obj"));

    tree->setOffset(2.6f, GROUND_Y, -8.0f);
    tree->setRotation(0.0f, -0.5f, 0.0f);

    gameState = START_SCREEN;
	
    sound = shared_ptr<Sound>(new Sound());
       sound->load("resources/sounds/bah.ogg", "bah");

    startTicks = 0;
    seconds = 0;

    lightModifier = -0.01f;
  }

  GameLogic::~GameLogic() {

  }

  void GameLogic::initGame()
  {
    goat->setOffset(-1.2f, GROUND_Y, -4.0f);
    bug->setOffset(0.5f, GROUND_Y + BUG_FLIGHT_HEIGHT, -18.0f);

    bug->startAnimating();

    bugState = FLYING_STRAIGHT;
    bugPreviousState = FLYING_STRAIGHT;
    bugFramesInCurrentState = 1;

    startTicks = SDL_GetTicks();

  }

  void GameLogic::moveGoat(const KeyInput &keyInput)
  {
    shared_ptr<glm::vec3> goatRotation = goat->getRotation();
    shared_ptr<glm::vec3> goatOffset = goat->getOffset();

    goat->stopAnimating();

    if (keyInput.left) {
      goatRotation->y -= GOAT_ROTATION_SPEED;

      while (goat->collidesWithSceneObject(tree)) {
        goatRotation->y += GOAT_ROTATION_SPEED;
      }

      if (goatRotation->y < -FULL_ROTATION)
        goatRotation->y = 0.0f;
      goat->startAnimating();

    }
    else if (keyInput.right) {
      goatRotation->y += GOAT_ROTATION_SPEED;

      while (goat->collidesWithSceneObject(tree)) {
        goatRotation->y -= GOAT_ROTATION_SPEED;
      }


      if (goatRotation->y > FULL_ROTATION)
        goatRotation->y = 0.0f;
      goat->startAnimating();
    }

    if (keyInput.up) {

      goatOffset->x -= cos(goatRotation->y) * GOAT_SPEED;
      goatOffset->z -= sin(goatRotation->y) * GOAT_SPEED;

      while (goat->collidesWithSceneObject(tree)) {
        goatOffset->x += cos(goatRotation->y) * GOAT_SPEED;
        goatOffset->z += sin(goatRotation->y) * GOAT_SPEED;
      }

      goat->startAnimating();

    }
    else if (keyInput.down) {
      goatOffset->x += cos(goatRotation->y) * GOAT_SPEED;
      goatOffset->z += sin(goatRotation->y) * GOAT_SPEED;

      while (goat->collidesWithSceneObject(tree)) {
        goatOffset->x -= cos(goatRotation->y) * GOAT_SPEED;
        goatOffset->z -= sin(goatRotation->y) * GOAT_SPEED;
      }

      goat->startAnimating();
    }

    if (goatOffset->z < MIN_Z + 1.0f)
      goatOffset->z = MIN_Z + 1.0f;
    if (goatOffset->z > MAX_Z - 1.0f)
      goatOffset->z = MAX_Z - 1.0f;

    if (goatOffset->x < goatOffset->z)
      goatOffset->x = goatOffset->z;
    if (goatOffset->x > -(goatOffset->z))
      goatOffset->x = -(goatOffset->z);

    goat->animate();

    goat->setRotation(0.0f, goatRotation->y, 0.0f);

    // Uncomment to see the goat's view of the world
    /* renderer->cameraPosition = *goatOffset;
    renderer->cameraPosition.y += 1.0f;
    renderer->cameraRotation = *goatRotation;
    renderer->cameraRotation.y -= 1.57f; */

  }

  void GameLogic::moveBug()
  {
    shared_ptr<glm::vec3> bugRotation = bug->getRotation();
    shared_ptr<glm::vec3> bugOffset = bug->getOffset();

    float xDistance = bug->getOffset()->x - goat->getOffset()->x;
    float zDistance = bug->getOffset()->z - goat->getOffset()->z;
    float distance = ROUND_2_DECIMAL(sqrt(xDistance * xDistance + zDistance*zDistance));

    float goatRelX = ROUND_2_DECIMAL(xDistance / distance);
    float goatRelZ = ROUND_2_DECIMAL(zDistance / distance);

    float bugDirectionX = cos(bug->getRotation()->y);
    float bugDirectionZ = sin(bug->getRotation()->y);

    float dotPosDir = goatRelX * bugDirectionX + goatRelZ * bugDirectionZ; // dot product

    // Bug state: decide
    if (bugState == bugPreviousState) {
      ++bugFramesInCurrentState;
    }
    else
    {
      bugFramesInCurrentState = 1;
    }

    bugPreviousState = bugState;

    if (bugState == DIVING_DOWN)
    {
      if (goat->collidesWithPoint(bug->getOffset()->x, bug->getOffset()->y, bug->getOffset()->z))
      {
		sound->play("bah");

        seconds = (SDL_GetTicks() - startTicks) / 1000;
        gameState = START_SCREEN;
      }

      if (bugFramesInCurrentState > BUG_DIVE_DURATION / 2)
      {
        bugState = DIVING_UP;
      }
    }
    else if (bugState == DIVING_UP)
    {
      if (goat->collidesWithPoint(bug->getOffset()->x, bug->getOffset()->y, bug->getOffset()->z))
      {
        gameState = START_SCREEN;
      }

      if (bugFramesInCurrentState > BUG_DIVE_DURATION / 2) {
        bugState = FLYING_STRAIGHT;
        bugOffset->y = GROUND_Y + BUG_FLIGHT_HEIGHT; // Correction of possible rounding errors
      }
    }
    else
    {

      if (distance > BUG_START_DIVE_DISTANCE)
      {
        if (dotPosDir < 0.98f)
        {
          bugState = TURNING;
        }
        else
        {
          bugState = FLYING_STRAIGHT;
        }
      }
      else
      {
        bugState = DIVING_DOWN;
      }

    }

    // Bug state: represent

    bugRotation->z = 0;

    if (bugState == TURNING)
    {

      bugRotation->y -= BUG_ROTATION_SPEED;

    }
    else if (bugState == DIVING_DOWN)
    {
      bugRotation->z = -BUG_DIVE_TILT;
      bugOffset->y -= bugVerticalSpeed;
    }
    else if (bugState == DIVING_UP)
    {
      bugRotation->z = BUG_DIVE_TILT;
      bugOffset->y += bugVerticalSpeed;
    }

    if (bugRotation->y < -FULL_ROTATION)
      bugRotation->y = 0.0f;


    bugOffset->x -= cos(bugRotation->y) * BUG_SPEED;
    bugOffset->z -= sin(bugRotation->y) * BUG_SPEED;

    if (bugOffset->z < MIN_Z)
      bugOffset->z = MIN_Z;
    if (bugOffset->z > MAX_Z)
      bugOffset->z = MAX_Z;

    if (bugOffset->x < bugOffset->z)
      bugOffset->x = bugOffset->z;
    if (bugOffset->x > -(bugOffset->z))
      bugOffset->x = -(bugOffset->z);

    // Uncomment to see the bug's view of the world
    /* renderer->cameraPosition = *bugOffset;
       renderer->cameraRotation = *bugRotation;
       renderer->cameraRotation.y -= 1.57f;*/

    bug->animate();
  }

  void GameLogic::processGame(const KeyInput &keyInput)
  {
    moveBug();
    moveGoat(keyInput);
  }

  void GameLogic::processStartScreen(const KeyInput &keyInput)
  {
    if (keyInput.enter) {
      initGame();
      gameState = PLAYING;
    }
  }

  void GameLogic::process(const KeyInput &keyInput)
  {
    switch (gameState) {
    case START_SCREEN:
      processStartScreen(keyInput);
      break;
    case PLAYING:
      processGame(keyInput);
      break;
    default:
      throw Exception("Urecognised game state");
      break;
    }
  }

  void GameLogic::render()
  {
    renderer->clearScreen();

    //Uncomment for groovy nightfall effect :)
    /*renderer->lightIntensity += lightModifier;

      if (renderer->lightIntensity < 0)
      {
      renderer->lightIntensity = 0.0f;
      lightModifier = 0.01f;
      }

      if (renderer->lightIntensity > 1.0f)
      {
      renderer->lightIntensity = 1.0f;
      lightModifier = -0.01f;
      }*/

    if (gameState == START_SCREEN) {
      float startScreenVerts[16] =
      {
        -1.0f, -1.0f, 1.0f, 1.0f,
        1.0f, -1.0f, 1.0f, 1.0f,
        1.0f, 1.0f, 1.0f, 1.0f,
        -1.0f, 1.0f, 1.0f, 1.0f
      };

      renderer->renderImage(&startScreenVerts[0], "startScreen");

      if (seconds != 0)
      {
        SDL_Color textColour = { 255, 100, 0, 255 };
        crusoeText48->renderText("Goat not bitten for " + intToStr(seconds) + " seconds",
          textColour, -0.95f, -0.6f, 0.0f, -0.8f);
      }

    }
    else
    {

      float skyVerts[16] =
      {
        -1.0f, -1.0f, 1.0f, 1.0f,
        1.0f, -1.0f, 1.0f, 1.0f,
        1.0f, 1.0f, 1.0f, 1.0f,
        -1.0f, 1.0f, 1.0f, 1.0f
      };

      renderer->renderImage(&skyVerts[0], "sky");

      // Draw the background

      float groundVerts[16] =
      {
        -25.0f, GROUND_Y, MAX_Z, 1.0f,
        25.0f, GROUND_Y, MAX_Z, 1.0f,
        25.0f, GROUND_Y,  MIN_Z, 1.0f,
        -25.0f, GROUND_Y, MIN_Z, 1.0f
      };

      renderer->renderImage(&groundVerts[0], "ground", true, glm::vec3(0.0f, 0.0f, 0.0f));

      renderer->renderSceneObject(goat);
      renderer->renderSceneObject(bug);
      renderer->renderSceneObject(tree);

    }
    renderer->swapBuffers();
  }

}
