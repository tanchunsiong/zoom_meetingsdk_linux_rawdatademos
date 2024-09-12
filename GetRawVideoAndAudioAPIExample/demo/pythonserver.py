import socket
import signal
import sys
from datetime import datetime  # Import the datetime module

# Define the server host and port
HOST = '127.0.0.1'  # localhost
PORT = 8080         # Port to listen on (non-privileged ports are > 1023)
BUFFER_SIZE = 640   # Size of the buffer to read data in chunks

server_socket = None

def handle_exit_signal(signal, frame):
    """Handle Ctrl+C and close the socket gracefully."""
    if server_socket:
        print("\nCtrl+C detected! Closing the server socket...", flush=True)
        server_socket.close()  # Release the server socket
    sys.exit(0)  # Exit the program

def start_udp_server():
    global server_socket

    # Create a UDP socket
    server_socket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

    # Bind the socket to the host and port
    server_socket.bind((HOST, PORT))

    print(f"UDP server listening on {HOST}:{PORT}...", flush=True)

    while True:
        try:
            # Receive data from the client (with the address)
            data, client_address = server_socket.recvfrom(BUFFER_SIZE)

            if data:
                # Get the current timestamp
                timestamp = datetime.now().strftime('%Y-%m-%d %H:%M:%S')

                # Print the received data along with the timestamp
                print(f"[{timestamp}] Received {len(data)} bytes of data from {client_address}", flush=True)

                # Optionally, send a response back to the client
                response = b"Data received"
                server_socket.sendto(response, client_address)

        except socket.error as e:
            print(f"Error receiving data: {e}", flush=True)

if __name__ == '__main__':
    # Catch the Ctrl+C signal and call the `handle_exit_signal` function
    signal.signal(signal.SIGINT, handle_exit_signal)

    try:
        # Start the UDP server and keep it running
        start_udp_server()
    except KeyboardInterrupt:
        # Handle cleanup on exit
        handle_exit_signal(None, None)
