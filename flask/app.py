from flask import Flask, render_template
from flask_socketio import SocketIO
import serial
import threading
from queue import Queue
import time

app = Flask(__name__)
socketio = SocketIO(app)

@app.route('/', methods=['GET', 'POST'])
def index():
    return render_template('index.html')

# 시리얼 포트 설정 (COM5, 9600 bps)
ser = serial.Serial('COM9', 9600)

# 큐 생성
serial_queue = Queue(maxsize=10)  # 큐 사이즈 설정

# 시리얼 통신을 처리하는 스레드 함수
def serial_thread():
    while True:
        try:
            if ser.in_waiting > 0:      # 시리얼 포트 버퍼에 읽을 수 있는 바이트 > 0 -> 데이터를 읽어옴
                received_data = ser.readline().decode('utf-8').strip()
                serial_queue.put(received_data, block=False)  # 큐에 저장, block=False로 설정하여 예외 발생
        except Exception as e:
            print(f"Serial Thread Exception: {e}")
        time.sleep(0.1)  # 작업 간격을 조절하여 무한 루프에서 너무 빠르게 동작하지 않도록 함

# 소켓 통신을 처리하는 백그라운드 스레드 함수
def socket_thread():
    while True:
        try:
            if not serial_queue.empty():        # 큐가 비어있지 않을 경우
                data = serial_queue.get(block=False)  # 큐에서 데이터 가져오기, block=False로 설정하여 예외 발생
                socketio.emit('serial_data', {'data': data})
                print(f"Sent to socket: {data}")
        except Exception as e:
            print(f"Socket Thread Exception: {e}")
        time.sleep(0.1)  # 작업 간격을 조절하여 무한 루프에서 너무 빠르게 동작하지 않도록 함

# 스레드 생성 및 시작
serial_thread_obj = threading.Thread(target=serial_thread)
socket_thread_obj = threading.Thread(target=socket_thread)


@socketio.on('input_value')
def handle_input_value(data):
    input_value = data['value']
    ser.write(input_value.encode('utf-8'))


@socketio.on('connect')                 # 큐에 저장된 값을 빼서
def handle_connect():
    print('Client connected')
    socketio.start_background_task(target=serial_thread)
    socketio.start_background_task(target=socket_thread)


if __name__ == '__main__':
    socketio.run(app, debug=True)