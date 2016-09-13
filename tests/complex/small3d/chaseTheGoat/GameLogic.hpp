/*
 *  GameLogic.hpp
 *
 *  Created on: 2014/11/09
 *      Author: Dimitri Kourkoulis
 *     License: BSD 3-Clause License (see LICENSE file)
 */

#pragma once

#include <memory>
#include <Logger.hpp>
#include <SceneObject.hpp>
#include <Renderer.hpp>
#include <Text.hpp>
#include <Sound.hpp>

#include "KeyInput.hpp"

using namespace small3d;

namespace AvoidTheBug3D {

  /**
   * @class	GameLogic
   *
   * @brief	The main body of the sample game.
   *
   */

  class GameLogic {

  private:

    shared_ptr<SceneObject> goat;
    shared_ptr<SceneObject> bug;
    shared_ptr<SceneObject> tree;

    shared_ptr<Renderer> renderer;

    shared_ptr<Text> crusoeText48;
	
	shared_ptr<Sound> sound;

    enum GameState {START_SCREEN, PLAYING};
    GameState gameState;

    enum BugState {FLYING_STRAIGHT, TURNING, DIVING_DOWN, DIVING_UP};
    BugState bugState, bugPreviousState;
    int bugFramesInCurrentState;
    float bugVerticalSpeed;

    unsigned int startTicks;
    int seconds;

    void initGame();
    void processGame(const KeyInput &keyInput);
    void processStartScreen(const KeyInput &keyInput);

    void moveGoat(const KeyInput &keyInput);
    void moveBug();
		
  public:

    /**
     * Constructor
     */
    GameLogic();

    /**
     * Destructor
     */
    ~GameLogic();

    /**
     * Process conditions and set up the next frame, also taking into consideration
     * the input from the keyboard
     * 
     * @param	keyInput	The keyboard input
     */
    void process(const KeyInput &keyInput);

    /**
     * @fn	void GameLogic::render();
     *
     * @brief	Renders the current state of the game on the screen.
     *
     */

    void render();

    float lightModifier; 
  };

} /* namespace AvoidTheBug3D */

