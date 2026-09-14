"""
ASTHMA_AIOT - Server nhan PCM tu ESP32-S3 va dong goi thanh file .wav chuan

Cach dung:
    python pc_wav_receiver.py

May tinh chay script nay va ESP32-S3 phai cung mang WiFi.
Ghi dia chi IP cua may tinh nay vao bien server_ip trong file .ino.

Ket qua:
    dataset/
    ├── normal/normal_001.wav, normal_002.wav, ...
    ├── cough/cough_001.wav, ...
    └── wheeze/wheeze_001.wav, ...

Sau khi thu xong, chi can keo tha (hoac sync qua rclone/Google Drive)
ca thu muc dataset/ len Google Drive de doc trong Colab.
"""

import socket
import wave
import os

HOST = "0.0.0.0"
PORT = 5005
DATASET_DIR = "dataset"

SAMPLE_RATE = 16000
CHANNELS = 1
SAMPWIDTH = 2  # 16-bit


def save_wav(label: str, index: int, pcm_data: bytes) -> str:
    folder = os.path.join(DATASET_DIR, label)
    os.makedirs(folder, exist_ok=True)
    filename = os.path.join(folder, f"{label}_{index:03d}.wav")

    with wave.open(filename, "wb") as wf:
        wf.setnchannels(CHANNELS)
        wf.setsampwidth(SAMPWIDTH)
        wf.setframerate(SAMPLE_RATE)
        wf.writeframes(pcm_data)

    return filename


def recv_exact(conn: socket.socket, n: int) -> bytes:
    data = b""
    while len(data) < n:
        chunk = conn.recv(n - len(data))
        if not chunk:
            break
        data += chunk
    return data


def recv_header_line(conn: socket.socket) -> bytes:
    line = b""
    while not line.endswith(b"\n"):
        b = conn.recv(1)
        if not b:
            break
        line += b
    return line


def main():
    srv = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    srv.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    srv.bind((HOST, PORT))
    srv.listen(1)
    print(f"Dang lang nghe tren cong {PORT} ... (Ctrl+C de dung)")

    while True:
        conn, addr = srv.accept()
        print(f"Ket noi tu {addr}")

        try:
            header_line = recv_header_line(conn)
            label, index_str, nbytes_str = header_line.decode().strip().split("|")
            index = int(index_str)
            nbytes = int(nbytes_str)

            pcm_data = recv_exact(conn, nbytes)
            if len(pcm_data) != nbytes:
                print(f"[CANH BAO] Nhan thieu du lieu: {len(pcm_data)}/{nbytes} byte")

            filename = save_wav(label, index, pcm_data)
            print(f"Da luu: {filename} ({len(pcm_data)} byte)")

        except Exception as e:
            print(f"[LOI] Xu ly ket noi that bai: {e}")
        finally:
            conn.close()


if __name__ == "__main__":
    main()
