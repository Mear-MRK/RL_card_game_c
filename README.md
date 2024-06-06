# RL Agent Training for Card Game

## Overview

This project implements a Reinforcement Learning (RL) agent in C, trained to play a trick-taking card game. The project is structured to support various agent classes, RL models, game state management, and utility functions for deck handling, card operations, and more. The design of the project uses advanced C programming techniques to simulate polymorphism and inheritance, commonly found in object-oriented programming.

### Features and Algorithms

* **Reinforcement Learning (RL):** The project utilizes two core RL algorithms:
    * **Q-Learning with Neural Network Approximation (Max-Q):** The agent learns to estimate the expected future rewards (Q-values) for different actions in a given state. These Q-values are then used to select the best action. Neural networks are employed to approximate these Q-values, enabling the agent to handle complex game states.
    * **Policy Gradient with Neural Network:** Instead of directly estimating Q-values, this algorithm learns a policy that maps states to actions. The policy is represented by a neural network, and the agent learns by adjusting the network's parameters to maximize the expected cumulative reward.

* **Agent Types:**
    * **Interactive:** A human-controlled agent, allowing users to play the game and provide feedback for debugging and comparison with AI agents.
    * **Random:** A baseline agent that selects actions randomly, demonstrating the importance of learning in achieving good performance.
    * **Sound:** A rule-based agent that follows a set of heuristics to play the game, providing a comparison point for the RL agent's performance.
    * **RL:** The reinforcement learning agents that utilize the Max-Q and Policy Gradient algorithms, respectively.

### Technical Implementation: Inheritance and Polymorphism in C

C, being a procedural language, does not directly support object-oriented features like inheritance and polymorphism. However, this project creatively emulates these concepts to create a flexible and modular agent framework.

The core idea revolves around using structs and function pointers:

* **`agent_class`:** This struct acts as a blueprint for different agent types (classes). It holds function pointers for essential agent behaviors like construction, action selection, learning, and so on.
* **`agent`:** This struct represents an actual agent instance. It contains a pointer to an `agent_class` that defines its behavior. By changing the `agent_class` pointer, you change the agent's type and its corresponding actions.
* **Function Pointers:** The function pointers in `agent_class` act like virtual methods in object-oriented languages. Each agent implementation provides its own concrete functions for these pointers.
* **Dynamic Dispatch:** When a method is called on an `agent` object, the actual function executed is determined at runtime based on the `agent_class` it points to. This achieves polymorphism, where the same method call leads to different behaviors for different agent types.

### Code Structure

The codebase is organized into several files:

* `agent.h`:  Defines the `agent` and `agent_class` structs and provides the core agent interface.
* `agent_*.h`, `agent_*.c`: Each pair of header and source files implements a specific agent type, filling in the function pointers in `agent_class` with its own logic.
* `game.h`, `card.h`, `deck.h`, etc.: Define data structures and logic for the card game, including cards, decks, hands, and game rules.
* `RL_*.h`, `RL_*.c`: Implement the reinforcement learning algorithms, neural networks, and training procedures.
* `main.c`: Contains the main program loop for running experiments, training agents, and evaluating their performance.
