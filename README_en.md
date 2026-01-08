<p align="right"><a href="./README.md">中文版本</a></p>

<h1 align='center'>Taiyi</h1>

<br>
<div align='center'>
  <a href='https://hub.docker.com/r/hongzhongx/taiyi/tags'><img src='https://img.shields.io/github/v/tag/hongzhongx/taiyi?style=flat-square'></a>
  <a href='https://github.com/hongzhongx/taiyi/blob/main/LICENSE'><img src='https://img.shields.io/github/license/hongzhongx/taiyi?style=flat-square'></a>
  <a href='https://github.com/hongzhongx/taiyi/graphs/contributors'><img src='https://img.shields.io/github/contributors-anon/hongzhongx/taiyi?style=flat-square'></a>
  <a href='https://github.com/hongzhongx/taiyi/commits/main'><img src='https://img.shields.io/github/commit-activity/y/hongzhongx/taiyi?style=flat-square'></a>
</div>
<p align="center">
👋 The project has just started. Please join our <a href="https://discord.gg/g4f84UEGCD" target="_blank">Discord</a> for early-stage communication.
</p>
<br>

# Introduction

This is an online virtual mini-game that simulates a cultivation (Xianxia) world, designed for fun-seekers to find their joy.

<br>
<div align='center'><a href='./doc/imgs/map.png'><img src='./doc/imgs/map.png' width=50%></a></div>
<div align='center'><i>In a MUD game, what would it be like if you could truly own and design your own room, or even operate your own town or sect?</i></div>

## Taiyi: The Underlying Network for Simulated Game Worlds

Simings (Siming: Master of Fate) connect with each other, gradually forming a causal network where various Heavenly Laws (Tiandao) operate. Under these rules, numerous virtual worlds evolve. One of them is the "Da Nuo World" (Great Exorcism World).

Whether you are a developer, operator, player, or NPC—whether human or AI Agent—once you connect to the Taiyi network, you are playing this game. Every participant has the equal right to build and interact with the virtual world. Furthermore, various rules (Heavenly Laws) are gradually created, improved, and evolved as the game progresses, and these rules can also be developed by any participant.

Developing and maintaining this game network can be done by any anonymous participant without the need for centralized facilities. Players can organize game design and development and fairly receive rewards even without knowing or trusting each other.

The Da Nuo World game utilizes blockchain concepts and technology. The game is not built on an abstract general-purpose blockchain; the game itself *is* a blockchain.

## Documentation

* Design Concept: [Taiyi: A Peer-to-Peer Network for Decentralized Autonomous Worlds](./doc/whitepaper_en.md)
* Da Nuo World Construction and Practice: [https://github.com/hongzhongx/taiyi-contracts](https://github.com/hongzhongx/taiyi-contracts)
* Smart Game Scripts (SGS): [SGS APIs Help](./doc/sgs_api.md)
* Improvement Proposals: [Taiyi Improvement Proposals — TIPs](https://github.com/hongzhongx/TIPs)

## Communication

* Early-stage Discord: [https://discord.gg/g4f84UEGCD](https://discord.gg/g4f84UEGCD). Ideally, the community will be hosted by the Taiyi network itself; see the [ZuoWangDao Project](https://) for details.
* GitHub Forums for Related Projects: Open a topic in the [TIPs Forum](https://github.com/hongzhongx/TIPs/discussions), [Taiyi Network Project Forum](https://github.com/hongzhongx/taiyi/discussions/), and [Contract Application Practice Forum](https://github.com/hongzhongx/taiyi-contracts/discussions) to discuss various improvement suggestions.

## Features

* No cost for game interaction (Recoverable Vitality = Free-to-play model)
* Fast event confirmation (One Breath = 3 seconds)
* Delayed security (Qi = Longevity/YANG converted over time)
* Hierarchical permissions (Multi-level private keys)
* Natively implemented built-in game asset allocation
* Heavenly Laws: Smart Game Scripts (SGS)
* Natively implemented DAO management mechanism (ZuoWangDao)
* Continuously enriched user-created game content (Heavenly Law execution, incentives via Longevity or Qi generation)

## Why use Lua as the Virtual Machine for Smart Game Scripts? Instead of EVM, WASM, V8, Docker, etc.?

* The Siming network uses blockchain technology and natively implements basic game logic and assets. Remember that the Taiyi network itself is part of the game; it has already deployed native game assets, so users do not need to use smart contracts to define game assets. Smart contracts on the Taiyi network are Smart Game Scripts (SGS), used for the logic and configuration of the game application itself rather than for general-purpose application development. This is very similar to early MUD game programming languages.
* Lua itself is very lightweight and integrates well with C++ applications. Welcome to all programmers who have used Lua for game development!
* What we need is a configuration-style smart contract, not a general-purpose smart contract. We want to easily configure the resources and logic of the Da Nuo World game on this blockchain-style network, rather than building various general blockchain applications. In short, the Taiyi network itself is a game that happens to use encrypted blockchain technology.
* For more information about the Taiyi Lua VM, please refer to the [documentation](libraries/lua/README.md).

## Metric Details

* The basic currency is the Longevity Pill, abbreviated as Longevity (Yang), with the symbol YANG.
* The consensus mechanism of the Siming Causal Network adopts the Delegated Proof of Stake (DPOS) algorithm; a Siming's power requires the strength of their followers.
* The total supply of Longevity (YANG) undergoes continuous inflation. Within twenty years of real-world time, the inflation rate linearly decreases from 10% APR to 1% APR.
  * 90% of the new supply is used to reward game logic development, game content production, and operations (mostly rewarded in the form of Qi).
  * 10% of the new supply is used to reward the Simings who maintain the Heavenly Causal Network (i.e., block production nodes).
* One day in the Da Nuo World lasts approximately 1 hour in real-world time; thus, one real-world year is twenty-four years in the Da Nuo World.

# Roadmap/Plan

See [Taiyi Network Roadmap and Plan](doc/roadmap.md) for details.

# Installation Instructions

Joining the Taiyi network is relatively simple. You can either choose a Docker image, manually build a Docker environment, or compile directly from source code. All steps are documented for different operating systems, with Ubuntu 22.04 being the most recommended and simplest way.

## How to Get Started Quickly

To quickly join the Taiyi network, we provide pre-compiled Docker images. For more instructions, please refer to the [Quick Start Guide](doc/quickstart.md).

## Building the Project

It is **strongly** recommended to use the pre-compiled Docker images or use Docker to compile the Taiyi system. These processes are explained in the [Quick Start Guide](doc/quickstart.md).

However, if you need to compile the system from source code, there is also a [Build Guide](doc/building.md) explaining the methods for Linux (Ubuntu LTS) and MacOS.

## Starting a P2P Node via Docker

Start a P2P node (currently requires 2GB RAM):

    docker run \
    -d -p 2025:2025 -p 8090:8090 --name taiyin-default \
    zuowangdaox/taiyi

    docker logs -f taiyin-default  # follow along

## Starting a Full Node via Docker

Start a node that provides *all* queryable data (e.g., to support a content website frontend; currently requires 14GB of RAM, and this amount is continuously growing):

    docker run \
    --env USE_WAY_TOO_MUCH_RAM=1 --env USE_FULL_WEB_NODE=1 \
    -d -p 2025:2025 -p 8090:8090 --name taiyin-full \
    zuowangdaox/taiyi

    docker logs -f taiyin-full

## Starting a Testnet Full Node via Docker

Start a testnet node that provides *all* queryable data (e.g., to support a content website frontend; currently requires 4GB of RAM, and this amount is continuously growing):

    docker run \
    --env IS_TESTNET=1 --env USE_FULL_WEB_NODE=1 \
    --env REQUIRED_PARTICIPATION=0 --env TAIYI_SEED_NODES="47.109.49.30:2025" \
    -d -p 2025:2025 -p 8090:8090 --name taiyin-full \
    zuowangdaox/taiyi

    docker logs -f taiyin-full

## Using Xuanpin Gate (xuanpin)

To interact with the node service program `taiyin`, we provide a basic client program called `xuanpin`. This client program comes with its own documentation, which can be viewed via the `help` command. The node that `xuanpin` connects to must have the `account_by_key_api` and `baiyujing_api` plugins enabled and be configured to accept WebSocket connections via the `webserver-ws-endpoint` parameter.

## Testing

To build the test project, please refer to the document [doc/devs/testing.md](doc/devs/testing.md). For instructions regarding test cases, refer to [tests/README.md](./tests/README.md).

# Configuring Taiyi Node

## Configuration File Instructions

Starting the `taiyin` program for the first time automatically generates a default data directory and configuration files, which are stored in the `siming_node_data_dir` directory by default. Since no seed nodes are specified in the default configuration, the `taiyin` program will do nothing, and you can only force quit (kill) the `taiyin` process. If you need to modify the configuration, here are two configuration examples for Docker images for reference ([Witness Node](contrib/config-for-docker.ini) and [Full Node](contrib/fullnode.config.ini)). The default configuration includes all options, some of which change based on the Docker configuration (some options used in images can be set via the command line).

## Seed Nodes

Here is a list of some seed nodes you can use to start joining the network: [doc/seednodes.txt](doc/seednodes.txt).

This file has been packaged into the Docker image. When running `docker run`, the container environment variable `TAIYI_SEED_NODES` can be set to space-separated seed node information (including port numbers) to override this.

## Environment Variables

There are several environment variable settings to run Taiyi nodes in different ways:

* `USE_WAY_TOO_MUCH_RAM` - If set to true, the Taiyi system will start as a 'Full Node'.
* `USE_FULL_WEB_NODE` - If set to true, the default configuration file will enable full API access and start some related plugins.
* `USE_NGINX_FRONTEND` - If set to true, an NGINX reverse proxy will be enabled on the outer layer of the Taiyi node. This proxy will handle incoming WebSocket Taiyi requests first. It also starts a custom health check at the path '/health', which lists how far your node is behind the latest block in the current network. If the gap is within 60 seconds, the health check returns a '200' code.
* `USE_MULTICORE_READONLY` - If set to true, the Taiyi system will start in multi-person read mode, providing better performance on multi-core systems. All read requests will be handled by multiple read-only nodes, while all write requests are automatically forwarded to a single 'write' node. NGINX load balances requests to read-only nodes, with each CPU core handling an average of 4 requests. This design is currently experimental and has some issues with certain API calls, which will be addressed in future development.
* `HOME` - Set the path where you want the Taiyi system to store data files (including block data, state data, configuration files, etc.). By default, this path is `/var/lib/taiyi`, and this path must exist within the Docker container. If you need to use a different loading location (such as a RAM disk or another disk drive), you can set this variable to point to a mapped volume on your Docker container.
* `IS_TESTNET` - If set to true, use the node program adapted for the current test network.
* `REQUIRED_PARTICIPATION` - Node participation rate. In some testnet environments, the participation rate is not high, so it can be set to 0 in testnet environments.

## System Requirements

For a full-featured web-enabled Taiyi node, at least 110GB of disk space is currently required, while the block data itself occupies just over 27GB. It is strongly recommended to run the Taiyi system on a fast disk system such as an SSD, or simply put the state files into a RAM disk. You can use the `--state-storage-dir=/path` option on the command line to set these locations. For a full-featured web-service node, its state data is at least 16GB. A seed node (P2P mode) generally consumes as little as 4GB of RAM and 24GB of state data file space. Basically, current single-core CPUs can meet the performance requirements. Note that the Taiyi system is continuously growing; these numbers are based on my assumed actual measurements as of November 2025. However, you may find that running a full node generally requires more disk space. We will continue to optimize the disk space used by the Taiyi system in the future.

On Linux systems, initial synchronization or replay of Taiyi nodes can use the following Virtual Memory settings. Of course, this is usually not necessary.

```
echo    75 | sudo tee /proc/sys/vm/dirty_background_ratio
echo  1000 | sudo tee /proc/sys/vm/dirty_expire_centisecs
echo    80 | sudo tee /proc/sys/vm/dirty_ratio
echo 30000 | sudo tee /proc/sys/vm/dirty_writeback_centisecs
```

# Responsibilities and Rights

The Da Nuo World game has no official project team. This game network (the Taiyi network) was initiated by ZuoWangDao 🀄️ purely for the enjoyment of fun-seekers. Any enthusiasts can freely participate, distribute, or leave. ZuoWangDao is responsible for all consequences caused by this project.

The code for this project is entirely open-source. ZuoWangDao 🀄️ waives all rights, assumes no responsibility, and makes no commitment to provide any support.

<br>
<div align='center'><b><big><i>Hear the horn from beyond the heavens, unveil the curtain of your cultivation journey.</i></big></b></div>
