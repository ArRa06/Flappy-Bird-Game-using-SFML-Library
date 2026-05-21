# Flappy Bird Clone (C++ & SFML)

A 2D arcade-style game developed using Object-Oriented Programming (OOP) concepts and the SFML graphics library. This project recreates the classic Flappy Bird experience, featuring smooth bird movement, dynamic obstacle generation, precise collision detection, and live score tracking in a visually engaging environment.

---

## 🎮 Features

* **Physics & Mechanics:** Smooth, gravity-based bird movement and jumping physics.
* **Procedural Generation:** Endless, randomized pipe generation for continuous gameplay.
* **Collision Detection:** Accurate hitboxes for the bird interacting with pipes and the environment.
* **Scoring System:** Live score tracking for successfully navigating through obstacles.

---

## 🛠️ Tech Stack

* **Language:** C++
* **Graphics & Windowing:** [SFML](https://www.sfml-dev.org/) (Simple and Fast Multimedia Library)
* **Architecture:** Object-Oriented Programming (OOP)

---

## 🚀 Setup & Installation

### Prerequisites
To compile and run this game, you will need:
1. A C++ compiler (such as GCC/g++ or MSVC).
2. The [SFML library](https://www.sfml-dev.org/download.php) installed and properly linked on your system.

### Compilation
If you are using `g++` (via Linux or MinGW on Windows), you can compile the game from the terminal using the following command:

```bash
g++ Flappy_Bird_Game.cpp -o flappy -lsfml-graphics -lsfml-window -lsfml-system
