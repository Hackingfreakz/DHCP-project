Mini DHCP System

1. Introduction

This project implements a lightweight Dynamic Host Configuration Protocol (DHCP) system using C programming. It demonstrates how multiple clients dynamically obtain IP addresses from a centralized server over a network.

The system is designed to mimic real-world DHCP behavior and integrates core concepts of UDP socket programming, networking protocols, lease management, and system-level programming.

---

2. System Overview

The system consists of two main components:

DHCP Server – Manages IP allocation and leases
DHCP Client – Requests and uses IP configuration

Architecture

Client → Server → IP Allocation → Lease Management

DHCP Workflow

DISCOVER → OFFER → REQUEST → ACK

---

3. Server Design (server.c)

The server acts as the central DHCP controller.

Key Responsibilities:

Creates a UDP socket using IPv4
Binds to port 1111
Listens for incoming client messages
Processes DHCP message types such as DISCOVER, REQUEST, RENEW, and RELEASE
Allocates IP addresses from a configured pool
Maintains lease database
Sends responses such as OFFER, ACK, and NAK
Logs all activities

Message Handling:

DISCOVER → Sends OFFER
REQUEST → Sends ACK
RENEW → Extends lease
RELEASE → Frees IP
Pool Exhausted → Sends NAK

---

4. Client Design (client.c)

The client requests IP configuration dynamically from the server.

Key Responsibilities:

Creates UDP socket with broadcast support
Sends DISCOVER message
Receives OFFER from server
Sends REQUEST for IP allocation
Receives ACK with configuration details

Additional Features:

Displays assigned IP address, subnet mask, gateway, and lease time
Performs lease renewal at half of lease duration
Handles graceful exit using Ctrl+C by sending RELEASE message

---

5. IP Pool and Lease Management (ip_lease.c)

This module handles IP allocation and tracking of leases.

Responsibilities:

Loads IP range from configuration file
Maintains lease table with states FREE, OFFERED, and ALLOCATED
Allocates IP addresses dynamically
Tracks lease expiry using timestamps
Reuses expired IP addresses
Prevents duplicate allocation
Supports lease renewal and release

Key Functions:

get_ip_from_lease assigns an IP
confirm_lease confirms allocation
renew_lease extends lease
lease_free releases IP

---

6. Communication Layer (UDP)

The system uses UDP for communication between client and server.

Features:

Socket type SOCK_DGRAM
Protocol IPv4 using AF_INET
Port number 1111

Communication Methods:

sendto is used to send packets
recvfrom is used to receive packets

Behavior:

Broadcast is used for DISCOVER
Unicast is used for OFFER, ACK, and NAK

---

7. Configuration Module (server.conf)

The server reads configuration parameters from a file.

Location

config/server.conf

Example Configuration

POOL_START=10.10.10.1
POOL_END=10.10.10.10
SUBNET_MASK=255.255.255.0
GATEWAY=10.10.10.0
LEASE_TIME=60

Functionality

Defines IP pool range
Sets lease duration
Configures network parameters

---

8. Logging System (logger.c)

The system logs all DHCP activities for monitoring and debugging.

Features

Logs are stored in logs/dhcp.log
Each log includes timestamp, log level, and message
Supports log levels INFO, WARN, and ERROR
Implements log rotation when file exceeds size limit
Prints logs to console

Log Format

[YYYY-MM-DD HH:MM:SS] LEVEL: Message

---

9. Build System (Makefile)

The Makefile compiles all components of the project.

Build

make

Clean

make clean

Targets

server
client

---

10. Execution Procedure

11. Compile the project

make

2. Start the server

./server

3. Run client

./client client1

4. Observe IP allocation and logs

5. Stop client using Ctrl+C

---

11. Key Concepts Demonstrated

UDP socket programming
Client-server architecture
DHCP protocol simulation
Lease management system
File handling and logging
Signal handling using SIGINT
Configuration-based system design

---

12. Conclusion

This project provides a practical implementation of a DHCP system in C. It demonstrates how dynamic IP allocation works in real networks and integrates networking, system programming, and protocol design concepts.

---
