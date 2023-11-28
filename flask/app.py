from flask import Flask, render_template
from flask_socketio import SocketIO
from serial import Serial

app = Flask(__name__)
socketio = SocketIO(app)

ser = Serial('COM5', 9600)

@app.route('/', methods=['GET', 'POST'])
def index():
    return render_template('index.html')

# 시리얼 통신 스레드 만들어서 데이터 받음 -> 큐에 저장
# 소켓 IO에서는 큐에 저장된 값을 pop해서 값 가져와서 socket io로 보내기
# 큐 사이즈 이상으로 데이터 보내면 예외처리, 데이터 없을 때 뺴는 경우 예외처리

def background_thread():
    while True:
        if ser.readable():              # sleep -> while문 줄임,  socet io로 전송만
            res = ser.readline()
            socketio.emit('serial_data', {'data': res.decode()[:len(res)-1]})
            print(res.decode()[:len(res)-1])

@socketio.on('connect')                 # 큐에 저장된 값을 빼서
def handle_connect():
    print('Client connected')
    socketio.start_background_task(target=background_thread)



if __name__ == '__main__':
    socketio.run(app, debug=True)
