# Ping Tool

A lightweight, custom implementation of the classic `ping` command written in C. This tool sends ICMP echo requests to network hosts and measures response times, providing network connectivity testing and latency measurement.

## Features

- **ICMP Echo Requests**: Sends standard ICMP echo packets to target hosts
- **DNS Resolution**: Supports both hostnames and IP addresses
- **Real-time Statistics**: Displays response times, packet loss, and RTT statistics
- **Signal Handling**: Graceful shutdown with Ctrl+C, showing comprehensive statistics
- **Timeout Handling**: Configurable timeout for unresponsive hosts
- **Cross-platform**: Works on Linux and other Unix-like systems

## Requirements

- Linux or Unix-like operating system (Or WSL)
- GCC compiler
- Root privileges (required for raw socket creation)

## Installation

1. Clone the repository:
```bash
git clone https://github.com/RETCY-unix/My-first-Ping-tool.git
cd My-first-Ping-tool
```

2. Compile the program:
```bash
gcc -o ping ping.c
```

## Usage

```bash
sudo ./ping <hostname/IP>
```

### Examples

```bash
# Ping a website
sudo ./ping google.com

# Ping an IP address
sudo ./ping 8.8.8.8

# Ping localhost
sudo ./ping localhost
```

### Sample Output

```
PING google.com (142.250.191.14): 56 data bytes
64 bytes from 142.250.191.14: icmp_seq=1 ttl=118 time=12.3 ms
64 bytes from 142.250.191.14: icmp_seq=2 ttl=118 time=11.8 ms
64 bytes from 142.250.191.14: icmp_seq=3 ttl=118 time=13.1 ms
^C
--- google.com ping statistics ---
3 packets transmitted, 3 received, 0% packet loss
rtt min/avg/max = 11.8/12.4/13.1 ms
```

## Technical Details

### Key Components

- **ICMP Packet Structure**: Custom implementation of ICMP echo request/reply packets
- **Checksum Calculation**: RFC-compliant checksum calculation for packet integrity
- **DNS Resolution**: Built-in hostname to IP address resolution
- **Raw Sockets**: Direct network interface access for ICMP packet transmission
- **Signal Handling**: Clean shutdown and statistics display on interruption

### Configuration

The tool includes several configurable parameters:

- `PACKET_SIZE`: Size of ping packets (default: 64 bytes)
- `MAX_WAIT_TIME`: Socket timeout in seconds (default: 5 seconds)
- `PING_SLEEP_RATE`: Interval between pings in microseconds (default: 1 second)

### Packet Flow

1. **DNS Lookup**: Resolves hostname to IP address if necessary
2. **Socket Creation**: Creates raw ICMP socket (requires root privileges)
3. **Packet Construction**: Builds ICMP echo request with checksum
4. **Transmission**: Sends packet and records timestamp
5. **Reception**: Receives reply and calculates round-trip time
6. **Statistics**: Tracks min/avg/max response times and packet loss

## Permissions

This tool requires root privileges to create raw sockets. This is a security requirement for low-level network operations.

```bash
# Run with sudo
sudo ./ping example.com

# Or set capabilities (alternative to running as root)
sudo setcap cap_net_raw+ep ./ping
./ping example.com
```

## Error Handling

The tool handles various error conditions:

- **Unknown hosts**: DNS resolution failures
- **Network unreachable**: No route to destination
- **Permission denied**: Insufficient privileges for raw socket creation
- **Timeout**: No response within configured timeout period

## Statistics

Upon termination (Ctrl+C), the tool displays:

- **Packets transmitted**: Total number of ping packets sent
- **Packets received**: Number of successful replies received
- **Packet loss percentage**: Reliability metric
- **RTT statistics**: Minimum, average, and maximum response times

## Limitations

- Requires root privileges for raw socket access
- IPv4 only (no IPv6 support)
- Basic ICMP implementation (no advanced ping features)
- Unix/Linux specific (uses Linux-specific headers)

## Contributing

1. Fork the repository
2. Create a feature branch (`git checkout -b feature/amazing-feature`)
3. Commit your changes (`git commit -m 'Add amazing feature'`)
4. Push to the branch (`git push origin feature/amazing-feature`)
5. Open a Pull Request

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

## Acknowledgments

- Based on the standard ping utility behavior
- Implements RFC 792 (Internet Control Message Protocol)
- Inspired by the original ping tool by Mike Muuss

## Security Note

This tool creates raw sockets and requires elevated privileges. Use responsibly and only on networks you own or have permission to test.
