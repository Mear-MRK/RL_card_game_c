# Card Game RL Agent Training Project

## Overview

This project is designed for building and training reinforcement learning (RL) agents to play a card game. The project is structured to support various agent classes, RL models, game state management, and utility functions for deck handling, card operations, and more. The design of the project uses advanced C programming techniques to simulate polymorphism and inheritance, commonly found in object-oriented programming.

## Project Structure

The project is organized into several files, each serving a specific purpose. Below is a brief description of each file:

### Agent Files

- **agent**: Defines the main agent structure and functions for constructing, destructing, and managing agents. It includes function pointers for agent behavior such as calling trump, acting, and gaining rewards. The `agent_class` structure simulates polymorphism by allowing different agent classes to implement these behaviors differently. The `agent` structure uses a pointer to `agent_class` to achieve a form of inheritance, allowing the behavior of an agent to vary based on its class.

- **agent_cls_Interactive**: Declares the `agent_cls_Interactive` class for interactive agents, allowing user input to guide the agent's actions.

- **agent_cls_Rand**: Declares the `agent_cls_Rand` class for random agents, including a structure for random number generator functions. This class defines an agent that makes decisions randomly, which can be useful for testing or as a baseline comparison.

- **agent_cls_RL_nn_max_Q**: Declares the `agent_cls_RL_nn_max_Q` class for RL agents using a neural network to maximize Q values. This class integrates deep learning models to learn optimal policies.

- **agent_cls_RL_nn_policy**: Declares the `agent_cls_RL_nn_policy` class for RL agents using a neural network for policy-based actions. This class focuses on learning policies directly from state-action pairs.

- **agent_cls_Sound**: Declares the `agent_cls_Sound` class for sound-based agents, potentially utilizing audio cues as part of the agent's decision-making process.

- **agent_RL**: Defines structures and functions for managing RL models, including constructing, destructing, loading, and saving models. This file provides the backbone for integrating RL techniques with agent behaviors.

### Game Mechanics

- **card**: Defines card-related structures and functions, including card validation, comparison, and conversion to/from strings. This file establishes the fundamental components of the card game.

- **deck**: Defines structures and functions for managing a deck of cards, including shuffling and constructing/deconstructing the deck. This file provides the necessary tools for handling card distribution and randomization.

- **game**: Contains utility functions for game rules, such as checking legal actions, comparing cards, and evaluating the table. This file ensures the rules of the card game are consistently applied.

- **hand**: Defines the structure of a player's hand and functions for managing cards in hand. This file manages the cards held by each player, supporting operations like adding and removing cards.

- **state**: Defines the state structure, which includes the current play order, trump suit, hands, and table state. This file captures the game state at any point, which is crucial for decision-making by agents.

### Reinforcement Learning

- **intern_rep_buf**: Defines an internal representation buffer for storing state-action pairs and rewards. This file supports the internal workings of RL algorithms by maintaining a history of interactions.

- **RL_model**: Defines structures and functions for RL models, including model types and training properties. This file integrates machine learning models into the agent framework.

- **RL_replay_buffer**: Defines structures and functions for a replay buffer used in RL training. This file implements the experience replay mechanism, which is essential for stabilizing RL training.

- **RL_training**: Defines parameters and functions for training RL models using data from the replay buffer. This file provides the necessary tools to train RL models based on stored experiences.

### Utilities

- **config**: Defines configuration macros for data types used throughout the project. This file allows for easy adjustments of data types based on different precision requirements or platforms.

- **rnd_seed**: Declares functions for generating random seeds. This file ensures that random processes within the game can be reliably reproduced or randomized as needed.

- **stat_act_array**: Provides functions for converting game state and actions into arrays for use in RL models. This file bridges the gap between game state representations and neural network inputs.

### Testing

- **test**: Declares test functions for various components, including deck, card, hand, and random seed generation. This file ensures that the components of the project work as intended through unit tests.

## Key Concepts

### Simulating Polymorphism and Inheritance

In this project, we use function pointers and structures to simulate OOP concepts like polymorphism and inheritance in C:

- **Polymorphism**: This is achieved through function pointers within the `agent_class` structure. Different agent classes can implement their own versions of behaviors (like `call_trump`, `act`, and `gain`) by assigning different functions to these pointers. This allows for dynamic behavior at runtime based on the agent's class.

- **Inheritance**: The `agent` structure contains a pointer to an `agent_class`, allowing an agent to inherit the behavior defined by its class. This design mimics the inheritance mechanism found in OOP languages, where a subclass inherits methods and properties from its superclass.
