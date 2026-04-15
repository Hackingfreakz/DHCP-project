# Mini DHCP System

## 1. Introduction

This project implements a lightweight **Dynamic Host Configuration Protocol (DHCP)** system using the C programming language. It demonstrates how multiple clients dynamically obtain IP addresses from a centralized server over a network.

The system mimics real-world DHCP behavior and incorporates key concepts such as:

* UDP socket programming
* Networking protocols
* Lease management
* System-level programming

---

## 2. System Overview

### Components

The system consists of two main components:

* **DHCP Server** – Manages IP allocation and lease tracking
* **DHCP Client** – Requests and utilizes IP configuration

### Architecture

```
Client → Server → IP Allocation → Lease Management
```

### DHCP Workflow

```
DISCOVER → OFFER → REQUEST → ACK
```

---

## 3. Server Design (server.c)

The server acts as the central controller for DHCP operations.

### Key Responsibilities

* Creates a UDP socket using IPv4
* Binds to port **1111**
* Listens for incoming client messages
* Processes DHCP message types:

  * DISCOVER
  * REQUEST
  * RENEW
  * RELEASE
* Allocates IP addresses from a pool
* Maintains a lease database
* Sends responses (**OFFER, ACK, NAK**)
* Logs all activities

### Message Handling

* **DISCOVER** → Sends OFFER
* **REQUEST** → Sends ACK
* **RENEW** → Extends lease
* **RELEASE** → Frees IP
* **Pool Exhausted** → Sends NAK

---

## 4. Client Design (client.c)

The client dynamically requests IP configuration from the server.

### Key Responsibilities

* Creates a UDP socket with broadcast support
* Sends DISCOVER message
* Receives OFFER from server
* Sends REQUEST for IP allocation
* Receives ACK with configuration

### Features

* Displays:

  * Assigned IP
  * Subnet Mask
  * Gateway
  * Lease Time

* Implements:

  * Lease renewal at half-time
  * Graceful exit using **Ctrl + C** (sends RELEASE message)

---

## 5. IP Pool & Lease Management (ip_pool.c)

This module manages IP allocation and lease tracking.

### Responsibilities

* Loads IP range from configuration file
* Maintains lease table with states:

  * FREE
  * OFFERED
  * ALLOCATED

### Handles

* IP allocation
* Lease confirmation
* Lease renewal
* Lease expiration
* IP reuse

### Key Functions

* `get_ip_from_lease()` → Assigns IP
* `confirm_lease()` → Confirms allocation
* `renew_lease()` → Extends lease
* `lease_free()` → Releases IP

---

## 6. Communication Layer (UDP)

The system uses UDP for communication.

### Features

* Socket Type: `SOCK_DGRAM`
* Protocol: IPv4 (`AF_INET`)
* Port: **1111**

### Communication Methods

* `sendto()` → Send packets
* `recvfrom()` → Receive packets

### Behavior

* Broadcast → Used for DISCOVER
* Unicast → Used for OFFER, ACK, NAK

---

## 7. Configuration Module (server.conf)

The server reads configuration from a file.

### Location

```
config/server.conf
```

### Parameters

```
POOL_START=10.10.10.1
POOL_END=10.10.10.10
SUBNET_MASK=255.255.255.0
GATEWAY=10.10.10.0
LEASE_TIME=60
```

### Functionality

* Defines IP pool range
* Sets lease duration
* Configures network parameters

---

## 8. Logging System (log.c)

The system logs all DHCP-related activities.

### Features

* Logs stored in: `logs/dhcp.log`
* Includes timestamp and log levels:

  * INFO
  * WARN
  * ERROR
* Supports log rotation when file size exceeds a limit
* Prints logs to console for debugging

### Log Format

```
[YYYY-MM-DD HH:MM:SS] LEVEL: Message
```

---

## 9. Build System (Makefile)

The project uses a Makefile for compilation.

### Commands

* Build:

  ```
  make
  ```

* Clean:

  ```
  make clean
  ```

### Targets

* server
* client

---

## 10. Execution Procedure

1. Compile the project:

   ```
   make
   ```

2. Start the server:

   ```
   ./server
   ```

3. Run the client:

   ```
   ./client <client_id>
   ```

4. Observe IP allocation and logs

5. Stop the client:

   ```
   Ctrl + C
   ```

---

## 11. Key Concepts Demonstrated

* UDP Socket Programming
* Client-Server Architecture
* Network Protocol Simulation (DHCP)
* Lease Management System
* File Handling and Logging
* Signal Handling (**SIGINT**)
* Configuration-based System Design

---

## 12. Conclusion

This project provides a practical implementation of a DHCP system in C, demonstrating how dynamic IP allocation works in real-world networks.

It integrates networking, system programming, and protocol design concepts, forming a strong foundation for understanding DHCP servers used in routers and modern network infrastructure
