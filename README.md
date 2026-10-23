# Gishath

## Overview
Gishath is a UCI-compatible chess engine built in C++. Currently, the engine plays around 2900 elo when compared to other engines on [CCRL](https://computerchess.org.uk/).

## Features
Gishath uses a standard PVS search with various pruning and move ordering heuristics. For evaluation, Gishath uses a simple NNUE with one hidden layer.

## Installation
There is no provided executable currently, so you must build the project yourself. Once you have the executable, you can enter in UCI commands in the terminal. If you want to play against the engine yourself, you should probably use an existing chess GUI, such as [En Croissant](https://encroissant.org/).
