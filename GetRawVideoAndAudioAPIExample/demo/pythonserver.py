import asyncio
import websockets
import json
import os
import aiohttp
from dotenv import load_dotenv
from cartesia import Cartesia
import numpy as np
import socket
import sounddevice as sd
import logging
import threading

import socket

# Create a UDP socket
client_socket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

# Target address (C++ server)
SERVER_ADDRESS = ('127.0.0.1', 9090)  # Match the C++ port




logging.basicConfig(level=logging.INFO, format='%(asctime)s - %(levelname)s - %(message)s')

load_dotenv()

# Replace with your API keys
DEEPGRAM_API_KEY = os.getenv("DEEPGRAM_API_KEY", "xxxxxxxxxxxxxxxx")
CEREBRAS_API_KEY = os.getenv("CEREBRAS_API_KEY")
CARTESIA_API_KEY = os.getenv("CARTESIA_API_KEY")

# Audio settings
CHUNK = 640
CHANNELS = 1
RATE = 32000

GANESHA_CONTEXT = """
You are a helpful assistant.
"""

# Initialize Cartesia client
cartesia_client = Cartesia(api_key=CARTESIA_API_KEY)

# Add a global variable to control microphone muting
is_mic_muted = asyncio.Event()
is_mic_muted.set()  # Start with the microphone unmuted

# Global PCM buffer (to receive audio from the UDP server)
pcm_buffer = b''

# Network settings for UDP server
HOST = '127.0.0.1'  # Localhost
PORT = 8080         # Port to listen on (non-privileged ports are > 1023)
BUFFER_SIZE = 640   # Size of the UDP packet

# Initialize the UDP server socket
server_socket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
server_socket.bind((HOST, PORT))

# Function to receive PCM data over UDP and append it to the buffer
def udp_server():
    global pcm_buffer
    print(f"UDP server listening on {HOST}:{PORT}...")
    while True:
        data, addr = server_socket.recvfrom(BUFFER_SIZE)
        pcm_buffer += data  # Append the received PCM data to the buffer

# Start the UDP server in a separate thread
def start_udp_server():
    udp_thread = threading.Thread(target=udp_server, daemon=True)
    udp_thread.start()

# Generate response using Cerebras
async def generate_ganesha_response(prompt: str, max_tokens: int = 100):
    headers = {
        "Authorization": f"Bearer {CEREBRAS_API_KEY}",
        "Content-Type": "application/json"
    }
    data = {
        "model": "llama3.1-8b",
        "messages": [
            {"role": "system", "content": GANESHA_CONTEXT},
            {"role": "user", "content": prompt}
        ],
        "max_tokens": max_tokens,
        "stream": True  # Enable streaming
    }
    try:
        async with aiohttp.ClientSession() as session:
            async with session.post("https://api.cerebras.ai/v1/chat/completions", headers=headers, json=data) as response:
                response.raise_for_status()
                async for line in response.content:
                    if line:
                        try:
                            chunk = json.loads(line.decode('utf-8').strip('data: '))
                            content = chunk['choices'][0]['delta'].get('content', '')
                            if content:
                                yield content
                        except json.JSONDecodeError:
                            continue
    except Exception as e:
        print(f"Error generating response: {e}")
        yield f"Error: Unable to generate response. Please try again later."

async def stream_tts_audio(text: str):
    try:
        voice_id = "39484eb1-2c20-4115-8752-d3f8faeb7739"
        voice = cartesia_client.voices.get(id=voice_id)
        
        #replaced pcm_f32le with pcm_s16le
        output_format = {
            "container": "raw",
            "encoding": "pcm_s16le",
            "sample_rate": 32000,
        }

        for output in cartesia_client.tts.sse(
            model_id="sonic-english",
            transcript=text,
            voice_embedding=voice["embedding"],
            stream=True,
            output_format=output_format,
        ):
            yield output["audio"]

    except Exception as e:
        print(f"Error generating audio: {e}")
        yield None

async def play_audio_stream(audio_stream):
    global is_mic_muted
    try:
        logging.info("Starting audio playback...")
        is_mic_muted.clear()  # Mute the microphone
        logging.info("Microphone muted")
        
        audio_data = b''
        async for chunk in audio_stream:
            if chunk:
                audio_data += chunk
        
        # Convert the accumulated audio data to a numpy array
        np_audio = np.frombuffer(audio_data, dtype=np.float32)
        
        # Play the audio
        sd.play(np_audio, samplerate=32000, blocking=True)
        sd.wait()
        
    except Exception as e:
        logging.error(f"Error in audio playback: {e}")
        logging.error("Traceback:", exc_info=True)
    finally:
        is_mic_muted.set()  # Unmute the microphone
        logging.info("Microphone unmuted")
    
    logging.info("Audio playback completed")

async def process_and_respond(transcript):
    logging.info(f"Processing transcript: {transcript}")
    
    full_response = ""
    async for response_chunk in generate_ganesha_response(transcript):
        print(response_chunk, end='', flush=True)
        full_response += response_chunk
        
    print("\n")
    # full_response is the plain text
    audio_stream = stream_tts_audio(full_response)

    # Send a message to the C++ application
  
    # this section sends the text (in byte format) back to the c++ code
    # Convert the string to bytes using UTF-8 encoding
    full_response_bytes = full_response.encode('utf-8')

   
   

    isChatElseStream = True

    #is Chat
    if isChatElseStream:
         # Send the byte-like object
        client_socket.sendto(full_response_bytes, SERVER_ADDRESS)
        print(f"Sent: {full_response_bytes}")
    #is Audio Stream
    else:
        client_socket.sendto(audio_stream, SERVER_ADDRESS)
        print(f"Sent: {audio_stream}")


    await play_audio_stream(audio_stream)

async def receive_transcription(websocket):
    logging.info("Waiting for transcriptions...")
    try:
        async for message in websocket:
            try:
                response = json.loads(message)
                if response.get("type") == "Results":
                    transcript = response["channel"]["alternatives"][0].get("transcript", "")
                    if transcript:
                        await process_and_respond(transcript)
            except json.JSONDecodeError:
                pass
    except websockets.exceptions.ConnectionClosed:
        logging.info("WebSocket connection closed")
    except Exception as e:
        logging.error(f"Error in receive_transcription: {e}")

SILENCE = b'\x00' * CHUNK  # Silence, a buffer of zeros

async def send_audio(websocket):
    global pcm_buffer
    logging.info("Using UDP stream as microphone. (Press Ctrl+C to stop)")

    try:
        while True:
            if is_mic_muted.is_set():
                if not hasattr(send_audio, 'last_unmuted') or not send_audio.last_unmuted:
                    logging.info("Microphone is active")
                    send_audio.last_unmuted = True
            else:
                if not hasattr(send_audio, 'last_unmuted') or send_audio.last_unmuted:
                    logging.info("Microphone is muted, waiting...")
                    send_audio.last_unmuted = False
                await asyncio.sleep(0.1)
                continue

            await is_mic_muted.wait()  # Wait if the microphone is muted

            # If no audio data is available, send silence to keep the connection alive
            if len(pcm_buffer) >= CHUNK:
                data_to_send = pcm_buffer[:CHUNK]
                pcm_buffer = pcm_buffer[CHUNK:]  # Remove sent data from the buffer
            else:
                data_to_send = SILENCE

            await websocket.send(data_to_send)

            await asyncio.sleep(0.01)  # Small delay to prevent flooding the console
    except KeyboardInterrupt:
        logging.info("\nStopping...")
    except Exception as e:
        logging.error(f"\nError in send_audio: {e}")


async def main():
    uri = "wss://api.deepgram.com/v1/listen?encoding=linear16&sample_rate=32000"
    
    # Start the UDP server in a background thread
    start_udp_server()
    
    try:
        async with websockets.connect(uri, extra_headers={
            "Authorization": f"Token {DEEPGRAM_API_KEY}"
        }) as websocket:
            logging.info("Connected to Deepgram. Starting transcription...")
            await asyncio.gather(
                send_audio(websocket),
                receive_transcription(websocket)
            )
    except websockets.exceptions.InvalidStatusCode as e:
        if e.status_code == 401:
            logging.error("Error: Invalid API key. Please check your Deepgram API key.")
        else:
            logging.error(f"WebSocket connection failed: {e}")
    except Exception as e:
        logging.error(f"An error occurred: {e}")
        logging.error("Traceback:", exc_info=True)

if __name__ == "__main__":
    asyncio.run(main())
