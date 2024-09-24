from flask import Flask, render_template, Response, request, jsonify
import cv2
from ultralytics import YOLO  # Pastikan model diimpor dari ultralytics

app = Flask(__name__)

# Variabel untuk menyimpan data dari dua sensor kelembapan
sensor_data = {
    "sensor1": None,
    "sensor2": None,
    "isWatering1": "false",
    "isWatering2": "false"
}

# Route untuk menerima dan mengirim data dari kedua sensor
@app.route('/sensor-data', methods=['POST', 'GET'])
def receive_sensor_data():
    global sensor_data
    if request.method == 'POST':
        sensor_data["sensor1"] = request.form.get('sensorValue1')
        sensor_data["sensor2"] = request.form.get('sensorValue2')
        sensor_data["isWatering1"] = request.form.get('isWatering1')
        sensor_data["isWatering2"] = request.form.get('isWatering2')
        print(f"Received sensor 1 data: {sensor_data['sensor1']}, Watering 1: {sensor_data['isWatering1']}")
        print(f"Received sensor 2 data: {sensor_data['sensor2']}, Watering 2: {sensor_data['isWatering2']}")
        return jsonify({"status": "success"}), 200
    elif request.method == 'GET':
        # Mengembalikan data sensor saat ada permintaan GET
        return jsonify(sensor_data), 200

# Fungsi untuk stream video dari webcam dengan YOLOv8
def gen_frames():
    model = YOLO('hidroponik.pt')  # Memuat model YOLOv8 dari file lokal
    cap = cv2.VideoCapture(0)  # Akses webcam
    if not cap.isOpened():
        print("Error: Webcam gagal dibuka.")
        return
    while True:
        success, frame = cap.read()
        if not success:
            print("Error: Tidak ada frame dari webcam.")
            break
        else:
            results = model(frame)  # YOLOv8 inference
            annotated_frame = results[0].plot()  # Menampilkan hasil dengan bounding box
            ret, buffer = cv2.imencode('.jpg', annotated_frame)
            frame = buffer.tobytes()
            yield (b'--frame\r\n'
                   b'Content-Type: image/jpeg\r\n\r\n' + frame + b'\r\n')

# Route utama untuk halaman web
@app.route('/')
def index():
    return render_template('update.html')

# Route untuk streaming video YOLOv8 dari webcam
@app.route('/video_feed')
def video_feed():
    return Response(gen_frames(), mimetype='multipart/x-mixed-replace; boundary=frame')

# Route untuk mendapatkan data sensor secara berkala dari halaman web
@app.route('/get-sensor-data', methods=['GET'])
def get_sensor_data():
    global sensor_data
    return jsonify(sensor_data), 200

if __name__ == "__main__":
    app.run(host='0.0.0.0', port=5000, debug=False)
