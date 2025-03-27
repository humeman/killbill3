# COM S 3270 Project
## Dungeon Game
This repository contains the code for the semester-long assignment in COM S 3270 at Iowa State,
which will eventually be a complete dungeon game written in C++.

## Run
A Makefile is provided which compiles each individual assignment into a binary.
For example: `make assignment1_01` will create `assignment1_01`

## Submit
A submission target is provided in the Makefile to automatically create and test the submission tarball
for an assignment.
For example: `make submit assignment=1_01`

## Time Travel
Tags are created for each completed assignment. Run `git checkout 1_01` (for example) to view the repo as it was when a particular assignment was completed.

## Assignments
* **Assignment 1.01**: Random dungeon generator

    This program creates a randomly-generated dungeon with several rooms and corridors, and prints it out to the terminal.
* **Assignment 1.02**: Dungeon load/save

    This program enables loading/saving dungeons in an RLG327 file format. It also creates a PC character and adds debugging flags.
* **Assignment 1.03**: Pathfinding

    This program adds pathfinding capabilities to the dungeon, generating two distance
    maps (for tunneling and non-tunneling monsters) to the PC.
* **Assignment 1.04**: Monsters

    This program adds various random monsters to the dungeon and allows them to move,
    with the dungeon refreshing continually until none remain.
* **Assignment 1.05**: Game UI

    This program turns the game into an actual game with an ncurses interface.
    The PC can now move, and colors, a monster list, quit button, and staircases are added. 
* **Assignment 1.06**: C++ and Fog of War

    The program has been ported to C++, and the player now has fog of war as it travels around the map
    (it can only see tiles near where it has already traveled).