import socket
from time import sleep 

# Định nghĩa host và port mà server sẽ chạy và lắng nghe
host = '192.168.1.67'
port = 8000
s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
s.bind((host, port))

s.listen(1) # 1 ở đây có nghĩa chỉ chấp nhận 1 kết nối
print("Server listening on port", port)

c, addr = s.accept()
print("Connect from ", str(addr))

#server sử dụng kết nối gửi dữ liệu tới client dưới dạng binary
while True:
    c.send(b"Hello, how are you")
    buffer = c.recv(1024)
    print(str(buffer))
    sleep(1)