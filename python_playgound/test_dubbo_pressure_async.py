# client
import asyncore
import socket


def get_dubbo_header():
    my_bytes = bytearray()

    # magic
    my_bytes.append(0xda)
    my_bytes.append(0xbb)
    # fastjson | req, 2way, no-event
    my_bytes.append(6 | 0b11000000)
    # status
    my_bytes.append(0x00)
    # long: request id
    for _ in xrange(8):
        my_bytes.append(0x00)
    return my_bytes


def get_body_str():
    dubbo_service_path = 'com.alibaba.dubbo.performance.demo.provider.IHelloService'
    dubbo_version = '2.0.1'
    service_version = None

    # varying parameters
    parameter_type_str = 'Ljava/lang/String;'
    arguments = 'abc'
    method_name = 'hash'
    attachement = {'path': dubbo_service_path}

    def fast_json_serializer(my_str):
        if my_str is None:
            return 'null'
        if isinstance(my_str, str):
            return '"' + my_str + '"'
        if isinstance(my_str, dict):
            ret = '{'
            for k, v in my_str.iteritems():
                if len(ret) > 1:
                    ret += ','
                ret += fast_json_serializer(k) + ':' + fast_json_serializer(v)
            ret += '}'
            return ret

    body_str = '\n'.join(map(fast_json_serializer,
                             [dubbo_version, dubbo_service_path, service_version,
                              method_name, parameter_type_str, arguments, attachement])) + '\n'
    return body_str


def get_dubbo_whole_package():
    def encode_int(integer):
        array = [(integer >> (8 * i)) & 0xff for i in range(3, -1, -1)]
        return array

    # 1st: encode
    body_bytes = str.encode(get_body_str())
    header_bytes = get_dubbo_header()

    # print map(hex, header_bytes)
    # print len(header_bytes)
    # print 'body len:', len(body_bytes)
    # print 'bytes:', encode_int(len(body_bytes))

    for my_b in encode_int(len(body_bytes)):
        header_bytes.append(my_b)
    return header_bytes + body_bytes


class DubboClient(asyncore.dispatcher):
    def __init__(self, host, port):
        asyncore.dispatcher.__init__(self)
        self.create_socket(socket.AF_INET, socket.SOCK_STREAM)
        self.connect((host, port))
        self.i = 0

    def handle_connect(self):
        pass

    def handle_close(self):
        self.close()

    def handle_read(self):
        data = self.recv(512)
        # print self.i, 'response value in {0,1,2}:', data[16], ', and result:', data[18:len(data) - 1]
        print data

    def writable(self):
        print self.i
        return self.i < 1000

    def handle_write(self):
        dubbo_package = get_dubbo_whole_package()
        self.send(dubbo_package)
        self.i += 1


if __name__ == '__main__':
    # address = ('127.0.0.1', 20889)
    # address = ('127.0.0.1', 3000)
    # s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    # s.connect(address)

    # dubbo_package = get_dubbo_whole_package()
    # s.send(dubbo_package)
    # s.close()

    dubbo_client = DubboClient('127.0.0.1', 3001)
    asyncore.loop()
