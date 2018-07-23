# client

import socket

if __name__ == '__main__':
    # address = ('127.0.0.1', 20889)
    address = ('127.0.0.1', 3000)


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


    # dubbo version related, not important
    def get_body_str():
        dubbo_service_path = 'com.alibaba.dubbo.performance.demo.provider.IHelloService'
        dubbo_version = '2.0.1'
        # dubbo_version = None
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


    print get_body_str()
    # 1st: encode
    body_bytes = str.encode(get_body_str())
    header_bytes = get_dubbo_header()


    def encode_int(integer):
        array = [(integer >> (8 * i)) & 0xff for i in range(3, -1, -1)]
        return array


    print 'body len:', len(body_bytes)
    print 'bytes:', encode_int(len(body_bytes))

    for my_b in encode_int(len(body_bytes)):
        header_bytes.append(my_b)
    print map(hex, header_bytes)
    print len(header_bytes)

    s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    s.connect(address)
    for i in xrange(100):
        s.send(header_bytes + body_bytes)

        # 2nd: decode
        data = s.recv(512)
        print data
        hex_lst = map(hex, map(ord, data))
        # print ord(data[3])
        # print 'new line flag:', hex_lst[17]
        print 'response value in {0,1,2}:', data[16], ', and result:', data[18:len(data) - 1]
        if i % 100 == 0:
            print i

    s.close()
